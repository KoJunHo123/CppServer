#include "pch.h"
#include "Lock.h"
#include "CoreTLS.h"
#include "DeadLockProfiler.h"

void Lock::WriteLock(const char* InName)
{
#if _DEBUG
	// 데드락 프로파일러에 현재 쓰레드가 이 이름의 락을 잡으려 함을 기록합니다.
	GDeadLockProfiler->PushLock(InName);
#endif

	// 1. 재귀 잠금(Recursive Lock) 확인
	// LockFlag의 상위 16비트를 추출하여 현재 소유 중인 쓰레드 ID를 확인합니다.
	const uint32 LockThreadId = (LockFlag.load() & WRITE_THREAD_MASK) >> 16;

	// 이미 내가 이 락을 잡고 있는 상태에서 다시 WriteLock을 호출했다면
	if (LockThreadId == LThreadId)
	{
		// 카운트만 올리고 즉시 리턴합니다. (동일 쓰레드 내 중복 획득 허용)
		++WriteCount;
		return;
	}

	// 2. 잠금 시도 (Spin 구간)
	const int64 BeginTick = ::GetTickCount64();
	// 내가 소유자가 되었을 때의 비트 상태를 미리 계산합니다 (상위 16비트에 내 ID 삽입).
	const uint32 Desired = ((LThreadId << 16) & WRITE_THREAD_MASK);

	while (true)
	{
		for (uint32 SpinCount = 0; SpinCount < MAX_SPIN_COUNT; ++SpinCount)
		{
			// Expected가 EMPTY_FLAG(0)라는 것은 Read/Write 모두 아무도 잡지 않은 클린한 상태임을 의미합니다.
			uint32 Expected = EMPTY_FLAG;

			// CAS 연산: LockFlag가 0이라면 Desired(내 ID)로 단숨에 바꿉니다.
			// 성공 시 true를 반환하며 원자적으로 잠금이 완료됩니다.
			if (LockFlag.compare_exchange_strong(OUT Expected, Desired))
			{
				++WriteCount;
				return;
			}
		}

		// 3. 타임아웃 및 양보
		// 일정 시간 동안 획득에 실패하면 시스템에 심각한 문제가 있다고 판단하여 크래시를 발생시킵니다.
		if (::GetTickCount64() - BeginTick >= ACQUIRE_TIMEOUT_TICK)
		{
			CRASH("LOCK_TIMEOUT");
		}

		// CPU를 너무 점유하지 않도록 잠시 다른 쓰레드에게 실행 우선권을 넘깁니다.
		this_thread::yield();
	}
}

void Lock::WriteUnlock(const char* InName)
{
#if _DEBUG
	GDeadLockProfiler->PopLock(InName);
#endif

	// 논리적 오류 검사: WriteLock을 풀려는데 Read 참여자가 있다면 비정상 상황입니다.
	if (0 != (LockFlag.load() & READ_COUNT_MASK))
	{
		CRASH("INVALID_UNLOCK_ORDER");
	}

	// 재귀적으로 호출된 횟수를 하나 줄입니다.
	const int32 LockCount = --WriteCount;

	// 모든 중복 잠금이 해제되었을 때만 실제로 LockFlag를 비웁니다.
	if (0 == LockCount)
	{
		// 0을 저장하여(EMPTY_FLAG) 다른 Reader나 Writer가 들어올 수 있게 개방합니다.
		LockFlag.store(EMPTY_FLAG);
	}
}

void Lock::ReadLock(const char* InName)
{
#if _DEBUG
	GDeadLockProfiler->PushLock(InName);
#endif

	// 1. Write 소유권 확인
	const uint32 LockThreadId = (LockFlag.load() & WRITE_THREAD_MASK) >> 16;

	// 내가 이미 WriteLock을 잡고 있는 상태라면, ReadLock은 언제든 허용됩니다.
	// 이는 자신이 쓴 데이터를 자신이 읽는 것을 보장하기 위함입니다.
	if (LockThreadId == LThreadId)
	{
		// 하위 16비트의 공유 카운트를 1 올립니다.
		LockFlag.fetch_add(1);
		return;
	}

	// 2. 잠금 시도
	const int64 BeginTick = ::GetTickCount64();

	while (true)
	{
		for (uint32 SpinCount = 0; SpinCount < MAX_SPIN_COUNT; ++SpinCount)
		{
			// 중요: 상위 16비트(Write)가 반드시 0인 상태여야만 들어갈 수 있습니다.
			// 따라서 Expected는 현재의 하위 16비트(ReadCount) 값만 가져옵니다.
			uint32 Expected = (LockFlag.load() & READ_COUNT_MASK);

			// 만약 그 사이 누가 WriteLock을 잡았다면 Expected와 LockFlag가 달라져 CAS가 실패합니다.
			// 아무도 WriteLock을 안 잡았다면 ReadCount를 1 증가시킨 값으로 갱신합니다.
			if (LockFlag.compare_exchange_strong(OUT Expected, Expected + 1))
			{
				return;
			}
		}

		if (::GetTickCount64() - BeginTick >= ACQUIRE_TIMEOUT_TICK)
		{
			CRASH("LOCK_TIMEOUT");
		}

		this_thread::yield();
	}
}

void Lock::ReadUnlock(const char* InName)
{
#if _DEBUG
	GDeadLockProfiler->PopLock(InName);
#endif

	// 원자적으로 1을 감소시킵니다.
	// fetch_sub은 감소하기 '전'의 값을 반환하므로, 반환값이 0이었다면
	// 이미 0인 카운트를 또 줄였다는 뜻이 되어 에러 처리를 합니다.
	if (0 == (LockFlag.fetch_sub(1) & READ_COUNT_MASK))
	{
		CRASH("MULTIPLE_UNLOCK");
	}
}
#pragma once

#include "Types.h"

/*---------------
	RW SpinLock
---------------*/

/*---------------
비트 구조 설계:
[WWWWWWWW][WWWWWWWW] [RRRRRRRR][RRRRRRRR]
- 상위 16비트 (W): Write Flag (단독 소유 중인 쓰레드 ID)
- 하위 16비트 (R): Read Flag (공유 중인 읽기 작업자 수)
---------------*/

// 재귀적 허용 규칙:
// 1. Write 상태에서 다시 Write를 잡는 것 (O) - 가능
// 2. Write 상태에서 Read를 추가로 잡는 것 (O) - 가능 (소유권 유지)
// 3. Read 상태에서 Write로 승격하는 것 (X) - 데드락 위험으로 금지

class Lock
{
	// 락 제어에 사용되는 상수 값 정의
	enum : uint32
	{
		// 무한 루프 방지를 위한 타임아웃 틱 (디버깅 용도)
		ACQUIRE_TIMEOUT_TICK = 10000,
		// CPU 점유를 잠시 양보하기 전 반복 횟수
		MAX_SPIN_COUNT = 5000,
		// 상위 16비트만 추출하기 위한 마스크 (11111111 11111111 00000000 00000000)
		WRITE_THREAD_MASK = 0xFFFF'0000,
		// 하위 16비트만 추출하기 위한 마스크 (00000000 00000000 11111111 11111111)
		READ_COUNT_MASK = 0x0000'FFFF,
		// 아무도 잠금을 잡지 않은 상태
		EMPTY_FLAG = 0x0000'0000
	};

public:
	// 배타적 잠금(WriteLock): 나만 접근하겠다.
	void WriteLock(const char* InName);
	// 배타적 잠금 해제
	void WriteUnlock(const char* InName);
	// 공유 잠금(ReadLock): 다른 읽기 작업자들과 함께 접근하겠다.
	void ReadLock(const char* InName);
	// 공유 잠금 해제
	void ReadUnlock(const char* InName);

private:
	// 실제 락 상태를 저장하는 원자적 변수 (멀티쓰레드 동기화 핵심)
	Atomic<uint32> LockFlag = EMPTY_FLAG;
	// 동일 쓰레드가 WriteLock을 몇 번 호출했는지 기록 (재귀 잠금용)
	uint16 WriteCount = 0;
};

/*---------------------
	Lock Guards (RAII)
---------------------*/

// ReadLockGuard: 스택 객체가 생성될 때 자동으로 ReadLock을 잡고, 소멸될 때 해제합니다.
class ReadLockGuard
{
public:
	ReadLockGuard(Lock& InLock, const char* InName)
		: ReadLock(InLock)
		, Name(InName)
	{
		ReadLock.ReadLock(InName);
	}
	// 함수를 벗어나거나 객체가 파괴되면 자동으로 해제되어 안전합니다.
	~ReadLockGuard() { ReadLock.ReadUnlock(Name); }

private:
	Lock& ReadLock;
	const char* Name;
};

// WriteLockGuard: 스택 객체가 생성될 때 자동으로 WriteLock을 잡고, 소멸될 때 해제합니다.
class WriteLockGuard
{
public:
	WriteLockGuard(Lock& InLock, const char* InName)
		: WriteLock(InLock)
		, Name(InName)
	{
		WriteLock.WriteLock(InName);
	}
	// 쓰기 작업이 끝나면 자동으로 해제되어 누수를 방지합니다.
	~WriteLockGuard() { WriteLock.WriteUnlock(Name); }

private:
	Lock& WriteLock;
	const char* Name;
};
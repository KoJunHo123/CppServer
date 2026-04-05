#pragma once

#define OUT

/*-------------------
		Lock
-------------------*/
#define USE_MANY_LOCKS(count)	Lock Locks[count];
#define USE_LOCK				USE_MANY_LOCKS(1)
#define READ_LOCK_IDX(idx)		ReadLockGuard readLockGuard_##idx(Locks[idx], typeid(this).name());
#define READ_LOCK				READ_LOCK_IDX(0)
#define WRITE_LOCK_IDX(idx)		WriteLockGuard writeLockGuard_##idx(Locks[idx], typeid(this).name());
#define WRITE_LOCK				WRITE_LOCK_IDX(0)

/*-------------------
		Crash
-------------------*/

// __analysis_assume : nullptr이 아니라고 가정해라.
// 인위적으로 크래시 내는 매크로
#define CRASH(cause)						\
{											\
	uint32* crash = nullptr;				\
	__analysis_assume(nullptr != crash);	\
	*crash = 0xDEADBEEF;					\
}

#define ASSERT_CRASH(expr)					\
{											\
	if (!(expr))							\
	{										\
		CRASH("ASSERT_CRASH");				\
		__analysis_assume(expr);			\
	}										\
}
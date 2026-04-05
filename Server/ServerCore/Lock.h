#pragma once

// mutex 직접 구현.
// 1. 표준 mutex는 재귀적으로 Lock을 잡을 수 없음.	- Reculsive Lock 쓰면 되긴 함.
// 2. 상호 배타적인 특성이 필요할 때가 있음.		- ReaderWriter Lock 필요.
// Read일 때는 Lock 없음. Write일 때만 Lock.

#include "Types.h"

/*---------------
	RW SpinLock 
---------------*/

/*---------------
[WWWWWWWW][WWWWWWWW][RRRRRRRR][RRRRRRRR]
W : Write Flag (Exclusive Lock Owner ThreadId)
R : Read Flag (Shared Lock Count)
---------------*/

// W->W(O)
// W->R(O)
// R->W(X)


class Lock
{
	enum : uint32
	{
		ACQUIRE_TIMEOUT_TICK = 10000,
		MAX_SPIN_COUNT = 5000,
		WRITE_THREAD_MASK = 0xFFFF'0000,
		READ_COUNT_MASK = 0x0000'FFFF,
		EMPTY_FLAG = 0x0000'0000
	};

public:
	void WriteLock(const char* name);
	void WriteUnlock(const char* name);
	void ReadLock(const char* name);
	void ReadUnlock(const char* name);

private:
	Atomic<uint32> LockFlag = EMPTY_FLAG;
	uint16 WriteCount = 0;
};

/*---------------------
	Lock Guards
---------------------*/

class ReadLockGuard
{
public:
	ReadLockGuard(Lock& lock, const char* name) 
		: ReadLock(lock)
		, Name(name) 
	{ 
		ReadLock.ReadLock(name); 
	}
	~ReadLockGuard() { ReadLock.ReadUnlock(Name); }

private:
	Lock& ReadLock;
	const char* Name;
};

class WriteLockGuard
{
public:
	WriteLockGuard(Lock& lock, const char* name) 
		: WriteLock(lock)
		, Name(name) 
	{ 
		WriteLock.WriteLock(name); 
	}
	~WriteLockGuard() { WriteLock.WriteUnlock(Name); }

private:
	Lock& WriteLock;
	const char* Name;
};
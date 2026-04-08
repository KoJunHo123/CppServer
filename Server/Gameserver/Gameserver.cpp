#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <windows.h>
#include <future>
#include "ThreadManager.h"
#include "RefCounting.h"

class Player
{
public:
	Player()
	{
	}

	void AddGold(int amount)
	{
		WRITE_LOCK; // Locks[0]에 대한 write lock

		m_gold += amount;
	}

	int GetGold()
	{
		READ_LOCK; // Locks[0]에 대한 read lock

		return m_gold;
	}

private:
	USE_LOCK; // Lock Locks[1];

	int m_gold = 0;
};

int main()
{
	
}

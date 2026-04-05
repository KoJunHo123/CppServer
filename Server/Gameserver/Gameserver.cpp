#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <windows.h>
#include <future>
#include "ThreadManager.h"

// 소수 구하기
bool IsPrime(int number)
{
	if(number <= 1)
	{
		return false;
	}
	if (2 == number || 3 == number)
	{
		return true;
	}

	for (int i = 2; i < number; ++i)
	{
		if (0 == (number % i))
		{
			return false;
		}
	}

	return true;
}

int CountPrime(int start, int end)
{
	int count = 0;
	for (int number = start; number <= end; ++number)
	{
		if(IsPrime(number))
		{
			++count;
		}
	}

	return count;
}

// 1과 자기 자신으로만 나뉘면 그걸 소수라고 함.

int main()
{
	const int MAX_NUMBER = 10000;
	// 1~MAX_NUMBER까지의 소수 개수

	CountPrime(1, MAX_NUMBER);
	vector<thread> threads;

	int coreCount = thread::hardware_concurrency();
	int jobCount = (MAX_NUMBER / coreCount) + 1;

	atomic<int> primeCount = 0;
	for (int i = 0; i < coreCount; ++i)
	{
		int start = i * jobCount + 1;
		int end = min(MAX_NUMBER, (i + 1) * jobCount);

		threads.push_back(thread([start, end, &primeCount]()
			{
				primeCount += CountPrime(start, end);
			}));
	}

	for (thread& t : threads)
	{
		t.join();
	}

	cout << primeCount << endl;
}


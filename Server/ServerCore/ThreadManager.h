#pragma once

#include <thread>
#include <functional>

/*-----------------
	Thread Manager
-------------------*/
class ThreadManager
{
public:
	ThreadManager();
	~ThreadManager();
	
	void Launch(function<void(void)> InCallback);
	void Join();

	static void InitTLS();
	static void DestroyTLS();

private:
	Mutex			Lock;
	vector<thread>	Threads;
};


#pragma once
#include <mutex>

template<typename T>
class LockQueue
{
public:
	LockQueue() {}
	LockQueue(const LockQueue&) = delete;
	LockQueue& operator=(const LockQueue&) = delete;

	void Push(T InValue)
	{
		lock_guard<mutex> lock(Mutex);
		Queue.push(std::move(InValue));
		CondVar.notify_one();
	}

	bool TryPop(T& OutValue)
	{
		lock_guard<mutex> lock(Mutex);
		if (true == Queue.empty())
		{
			return false;
		}

		OutValue = std::move(Queue.front());
		Queue.pop();
		return true;
	}

	void WaitPop(T& OutValue)
	{
		unique_lock<mutex> lock(Mutex);
		CondVar.wait(lock, [this]()
			{
				return false == Queue.empty();
			});
		OutValue = std::move(Queue.front());
		Queue.pop();
	}

private:
	mutex Mutex;
	queue<T> Queue;
	condition_variable CondVar;

};
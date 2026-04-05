#include "pch.h"
#include "DeadLockProfiler.h"

/*-------------------
	DeadLockProfiler
-------------------*/

void DeadLockProfiler::PushLock(const char* name)
{
	LockGuard guard(Lock);

	// 아이디를 찾거나 발급한다.
	int32 lockId = 0;
	auto findIt = NameToId.find(name);
	if (NameToId.end() == findIt)
	{
		lockId = static_cast<int32>(NameToId.size());
		NameToId[name] = lockId;
		IdToName[lockId] = name;
	}
	else
	{
		lockId = findIt->second;
	}

	// 잡고 있는 락이 있었다면
	if (false == LLockStack.empty())
	{
		const int32 prevId = LLockStack.top();
		if (prevId != lockId)
		{
			set<int32>& history = LockHistory[prevId];
			if (history.end() == history.find(lockId))
			{
				history.insert(lockId);
				CheckCycle();
			}
		}
	}

	LLockStack.push(lockId);
}

void DeadLockProfiler::PopLock(const char* name)
{
	LockGuard guard(Lock);

	if (LLockStack.empty())
	{
		CRASH("MULTIPLE_UNLOCK");
	}

	int32 lockId = NameToId[name];
	if (lockId != LLockStack.top())
	{
		CRASH("INVALID_UNLOCK");
	}

	LLockStack.pop();
}

void DeadLockProfiler::CheckCycle()
{
	const int32 lockCount = static_cast<int32>(NameToId.size());
	DiscoveredOrder = vector<int32>(lockCount, -1);
	DiscoveredCount = 0;
	Finished = vector<bool>(lockCount, false);
	Parent = vector<int32>(lockCount, -1);

	for (int32 lockId = 0; lockId < lockCount; ++lockId)
	{
		Dfs(lockId);
	}

	// 연산이 끝났으면 정리.
	DiscoveredOrder.clear();
	Finished.clear();
	Parent.clear();
}

void DeadLockProfiler::Dfs(int32 here)
{
	if (-1 != DiscoveredOrder[here])
	{
		return;
	}

	DiscoveredOrder[here] = DiscoveredCount++;

	// 모든 인접한 정점을 순회한다.
	auto findIt = LockHistory.find(here);
	if (LockHistory.end() == findIt)
	{
		Finished[here] = true;
		return;
	}

	set<int32>& nextSet = findIt->second;
	for (int32 there : nextSet)
	{
		if (-1 == DiscoveredOrder[there])
		{
			Parent[there] = here;
			Dfs(there);
			continue;
		}

		// here가 there보다 먼저 발견되었다면, there는 here의 후손이다. (순방향 간선이다.)
		if (DiscoveredOrder[here] < DiscoveredOrder[there])
		{
			continue;
		}

		// 순방향이 아니고, Dfs(there)가 아직 종료하지 않았다면, there는 here의 선조이다. (역방향 간선)
		if (false == Finished[there])
		{
			printf("%s -> %s\n", IdToName[here], IdToName[there]);

			int32 now = here;
			while (true)
			{
				printf("%s -> %s\n", IdToName[Parent[now]], IdToName[now]);
				now = Parent[now];
				if (now == there)
				{
					break;
				}
			}

			CRASH("DEADLOCK_DETECTED");
		}
	}

	Finished[here] = true;
}

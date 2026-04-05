#pragma once
#include <stack>
#include <map>
#include <vector>

/*-------------------
	DeadLockProfiler
-------------------*/

class DeadLockProfiler
{
public:
	void PushLock(const char* name);
	void PopLock(const char* name);
	void CheckCycle();

private:
	void Dfs(int32 here);

private:
	unordered_map<const char*, int32>	NameToId;
	unordered_map<int32, const char*>	IdToName;
	map<int32, set<int32>>				LockHistory;

	Mutex Lock;

private:
	vector<int32>	DiscoveredOrder;			// 노드가 발견된 순서를 기록하는 배열
	int32			DiscoveredCount = 0;		// 노드가 발견된 순서
	vector<bool>	Finished;					// DFS(i)가 종료되었는지 여부.
	vector<int32>	Parent;						
};


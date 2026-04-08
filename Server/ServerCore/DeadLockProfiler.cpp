#include "pch.h"
#include "DeadLockProfiler.h"

/*-------------------
	DeadLockProfiler
-------------------*/

void DeadLockProfiler::PushLock(const char* InName)
{
	// 프로파일러 내부 자료구조(Map, Stack 등)를 보호하기 위한 동기화입니다.
	LockGuard Guard(Lock);

	// 1. Lock 이름과 ID 매핑
	// 문자열 비교는 비용이 크므로 고유한 정수 ID로 변환하여 관리합니다.
	int32 LockId = 0;
	auto FindIt = NameToId.find(InName);

	if (NameToId.end() == FindIt)
	{
		// 처음 등장한 락이라면 새로운 ID를 발급하고 이름과 매핑 정보를 저장합니다.
		LockId = static_cast<int32>(NameToId.size());
		NameToId[InName] = LockId;
		IdToName[LockId] = InName;
	}
	else
	{
		LockId = FindIt->second;
	}

	// 2. 락 순서 관계(간선) 기록
	// LLockStack은 현재 쓰레드가 잡고 있는 락들의 순서를 담은 TLS 스택입니다.
	if (false == LLockStack.empty())
	{
		// 이전에 이미 잡고 있던 락이 있다면, 그 락이 부모(Source)가 됩니다.
		const int32 prevId = LLockStack.top();

		// 동일한 락을 중복해서 잡는(재진입) 경우가 아니라면 순서 관계를 정의합니다.
		if (prevId != LockId)
		{
			// prevId 다음에 LockId를 잡았다는 관계를 그래프(LockHistory)에 추가합니다.
			set<int32>& history = LockHistory[prevId];

			// 이전에 발견된 적 없는 새로운 선후 관계라면 사이클 검사를 수행합니다.
			if (history.end() == history.find(LockId))
			{
				history.insert(LockId);
				// 새로운 간선이 추가될 때마다 데드락 위험이 있는지 확인합니다.
				CheckCycle();
			}
		}
	}

	// 3. 현재 획득한 락을 스택에 쌓아 현재 쓰레드의 상태를 갱신합니다.
	LLockStack.push(LockId);
}

void DeadLockProfiler::PopLock(const char* InName)
{
	LockGuard guard(Lock);

	// 락을 해제하려는데 스택이 비어있다면, 락을 잡지도 않고 해제하려는 논리적 오류입니다.
	if (LLockStack.empty())
	{
		CRASH("MULTIPLE_UNLOCK");
	}

	int32 lockId = NameToId[InName];

	// 락은 반드시 획득한 역순(LIFO)으로 해제되어야 합니다.
	// 가장 최근에 잡은 락과 지금 해제하려는 락이 다르면 잘못된 프로그래밍입니다.
	if (lockId != LLockStack.top())
	{
		CRASH("INVALID_UNLOCK");
	}

	// 검증이 완료되면 스택에서 제거하여 이전 락 상태로 돌아갑니다.
	LLockStack.pop();
}

void DeadLockProfiler::CheckCycle()
{
	const int32 LockCount = static_cast<int32>(NameToId.size());

	// 탐색을 위한 상태 배열들을 초기화합니다.
	DiscoveredOrder = vector<int32>(LockCount, -1); // 노드 발견 순서
	DiscoveredCount = 0;
	Finished = vector<bool>(LockCount, false);     // DFS 탐색 완료 여부
	Parent = vector<int32>(LockCount, -1);         // 사이클 경로 추적용 부모 노드 기록

	// 그래프가 여러 개의 컴포넌트로 나뉘어 있을 수 있으므로 모든 노드를 기점으로 탐색합니다.
	for (int32 LockId = 0; LockId < LockCount; ++LockId)
	{
		Dfs(LockId);
	}

	// 메모리 절약을 위해 임시 데이터들을 비워줍니다.
	DiscoveredOrder.clear();
	Finished.clear();
	Parent.clear();
}

void DeadLockProfiler::Dfs(int32 InHere)
{
	// 이미 방문한 노드라면 중복 탐색하지 않습니다.
	if (-1 != DiscoveredOrder[InHere]) return;

	// 현재 노드에 방문 순서(ID)를 부여합니다.
	DiscoveredOrder[InHere] = DiscoveredCount++;

	auto FindIt = LockHistory.find(InHere);
	if (LockHistory.end() == FindIt)
	{
		// 자식 노드가 없다면 탐색 종료 처리를 하고 리턴합니다.
		Finished[InHere] = true;
		return;
	}

	set<int32>& NextSet = FindIt->second;
	for (int32 There : NextSet)
	{
		// 1. 아직 방문하지 않은 이웃 노드라면 경로를 기록하며 더 깊이 탐색합니다.
		if (-1 == DiscoveredOrder[There])
		{
			Parent[There] = InHere;
			Dfs(There);
			continue;
		}

		// 2. 이미 방문했지만 나중에 발견된 노드(Forward Edge)라면 무시합니다.
		if (DiscoveredOrder[InHere] < DiscoveredOrder[There]) continue;

		// 3. 핵심: 발견은 되었으나 아직 탐색이 종료되지 않은 노드를 다시 만났다면(Back Edge)
		// 이것은 그래프 내에 순환 고리(사이클)가 형성되었음을 의미합니다.
		if (false == Finished[There])
		{
			// 데드락이 발생할 수 있는 락 호출 순서를 역추적하여 출력합니다.
			printf("%s -> %s\n", IdToName[InHere], IdToName[There]);
			int32 Now = InHere;
			while (true)
			{
				printf("%s -> %s\n", IdToName[Parent[Now]], IdToName[Now]);
				Now = Parent[Now];
				if (Now == There) break;
			}

			// 실제 데드락이 발생하기 전에 프로그램을 종료시켜 버그를 수정하게 유도합니다.
			CRASH("DEADLOCK_DETECTED");
		}
	}

	// 해당 노드와 연결된 모든 자식들의 탐색이 끝났음을 표시합니다.
	Finished[InHere] = true;
}
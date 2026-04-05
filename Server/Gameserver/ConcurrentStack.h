#pragma once
#include <mutex>

template<typename T>
class LockStack
{
public:
	LockStack() {}

	LockStack(const LockStack&) = delete;
	LockStack& operator=(const LockStack&) = delete;

	void Push(T InValue)
	{
		lock_guard<mutex> lock(Mutex);
		Stack.push(std::move(InValue));
		CondVar.notify_one();
	}

	// Empty가 의미 없음. 다른 쓰레드에서 그 사이에 넣으면 어캄?
	bool TryPop(T& OutValue)
	{
		lock_guard<mutex> lock(Mutex);
		if (Stack.empty())
		{
			return false;
		}

		// empty -> top -> pop
		OutValue = std::move(Stack.top());
		Stack.pop();
		return true;
	}

	void WaitPop(T& OutValue)
	{
		unique_lock<mutex> lock(Mutex);
		CondVar.wait(lock, [this]() 
			{ 
				return false == Stack.empty(); 
			});
		OutValue = std::move(Stack.top());
		Stack.pop();
	}

private:
	stack<T> Stack;
	mutex Mutex;
	condition_variable CondVar;
};


template<typename T>
class LockFreeStack
{
	struct Node
	{
		Node(const T& InValue) : Data(InValue), Next(nullptr)
		{

		}

		T Data;
		Node* Next;
	};

public:
	void Push(const T& InValue)
	{
		Node* node = new Node(InValue);
		node->Next = Head;
		while (false == Head.compare_exchange_weak(node->Next, node))
		{
			// node->Next = Head;
		}
		/*
		if (node->Next == Head)
		{
			Head = node;
			return true;
		}
		else
		{
			node->Next = Head;
			return false;
		}
		*/
	}

	bool TryPop(T& OutValue)
	{
		++PopCount;
		Node* oldHead = Head;
		while (oldHead && false == Head.compare_exchange_weak(oldHead, oldHead->Next))
		{

		}

		if (nullptr == oldHead)
		{
			--PopCount;
			return false;
		}

		OutValue = oldHead->Data;

		// 잠시 삭제 보류
		// delete oldHead;
		TryDelete(oldHead);

		return true;
	}

	void TryDelete(Node* InOldHead)
	{
		if (1 == PopCount)
		{
			Node* node = PendingList.exchange(nullptr);

			if (0 == --PopCount)
			{
				DeleteNodes(node);
			}
			else if(node)
			{
				ChainPendingNodeList(node);
			}

			delete InOldHead;
			
		}
		else
		{
			ChainPendingNode(InOldHead);
			--PopCount;
		}
	}

	void ChainPendingNodeList(Node* InFirst, Node* InLast)
	{
		InLast->Next = PendingList;

		while (false == PendingList.compare_exchange_weak(InLast->Next, InFirst))
		{

		}
	}

	void ChainPendingNodeList(Node* InNode)
	{
		Node* last = InNode;
		while (last->Next)
		{
			last = last->Next;
		}

		ChainPendingNodeList(InNode, last);
	}

	void ChainPendingNode(Node* InNode)
	{
		ChainPendingNodeList(InNode, InNode);
	}

	static void DeleteNodes(Node* InNode)
	{
		while (InNode)
		{
			Node* next =InNode->Next;
			delete InNode;
			InNode = next;
		}
	}

private:
	atomic<Node*> Head;
	atomic<uint32> PopCount = 0;	// Pop을 실행 중인 쓰레드 개수.
	atomic<Node*> PendingList;		// 삭제되어야 할 노드들(첫번째 노드)
};

/*
A.compare_exchange_weak(B, C)가 실행될 때 내부적으로 일어나는 일은 다음과 같습니다.

상황 1: A == B 인 경우 (성공)
동작: A의 값을 C로 바꿉니다.

반환: true를 반환합니다.

의미: "내가 예상한 값(B)이 그대로 있네? 계획대로 새로운 값(C)으로 업데이트하자!"

상황 2: A != B 인 경우 (실패)
동작: B에 현재 A의 값을 덮어씌웁니다. (이 부분이 핵심입니다!)

반환: false를 반환합니다.

의미: "누가 그새 A를 바꿔버렸네? 그럼 바뀐 현재의 A 값을 B에 담아줄 테니, 이걸 보고 다시 시도해봐."
*/
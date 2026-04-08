#include "pch.h"
#include "ThreadManager.h"
#include "CoreTLS.h"
#include "CoreGlobal.h"

// 생성자: 객체가 생성되는 시점(주로 메인 쓰레드)에서 실행됩니다.
ThreadManager::ThreadManager()
{
	// 메인 쓰레드 역시 하나의 실행 흐름이므로 고유한 TLS 정보를 가져야 합니다.
	// 이 호출을 통해 메인 쓰레드의 LThreadId가 초기화됩니다.
	InitTLS();
}

// 소멸자: 클래스 인스턴스가 파괴될 때 호출됩니다.
ThreadManager::~ThreadManager()
{
	// 관리 중인 모든 쓰레드가 안전하게 작업을 마칠 때까지 기다립니다.
	// 자원을 해제하기 전 실행 흐름을 정리하는 중요한 단계입니다.
	Join();
}

// Launch 함수: 새로운 쓰레드를 생성하고 실행할 함수를 할당합니다.
void ThreadManager::Launch(function<void(void)> InCallback)
{
	// 여러 쓰레드에서 동시에 Launch를 호출할 경우 Threads 벡터에 접근하는 것이
	// 위험하므로 LockGuard를 통해 상호 배제(Mutual Exclusion)를 보장합니다.
	LockGuard guard(Lock);

	// std::thread 객체를 생성하여 실행 가능한 상태로 만들고 관리 목록에 추가합니다.
	// 람다 캡처 [=]를 통해 전달받은 함수 객체(InCallback)를 복사하여 전달합니다.
	Threads.push_back(thread([=]()
		{
			// [새로 생성된 쓰레드의 진입점]

			// 1. 이 쓰레드만의 고유한 TLS 데이터를 생성합니다 (주로 쓰레드 ID 부여).
			InitTLS();

			// 2. 외부에서 넘겨준 실제 로직(함수)을 실행합니다.
			InCallback();

			// 3. 로직이 끝나면 TLS 자원을 정리합니다. 
			// 현재 구현에서는 비어있지만 나중에 동적 할당 자원 해제 등이 추가될 수 있습니다.
			DestroyTLS();
		}));
}

// Join 함수: 생성된 모든 쓰레드의 작업이 끝날 때까지 현재 쓰레드를 대기시킵니다.
void ThreadManager::Join()
{
	// 관리 목록에 있는 모든 thread 객체를 순회합니다.
	for (thread& t : Threads)
	{
		// joinable은 해당 쓰레드가 실행 중이며 이미 join되지 않았는지 확인하는 함수입니다.
		if (t.joinable())
		{
			// 해당 쓰레드가 완전히 종료될 때까지 호출한 쓰레드(보통 메인)를 멈춥니다.
			t.join();
		}
	}

	// 모든 쓰레드가 종료되었으므로 목록을 비워 메모리를 정리합니다.
	Threads.clear();
}

// InitTLS 함수: 각 쓰레드에 세상을 하나뿐인 고유 번호(ID)를 부여합니다.
void ThreadManager::InitTLS()
{
	// 전역적으로 하나만 존재하는 정적 원자 변수입니다.
	// 1번부터 시작하여 새로운 쓰레드가 생길 때마다 번호를 하나씩 올립니다.
	static Atomic<uint32> SThreadId = 1;

	// fetch_add는 값을 1 증가시키고 '증가하기 전의 값'을 반환하는 원자적 연산입니다.
	// LThreadId는 각 쓰레드의 전용 저장소(TLS)에 정의된 변수여야 합니다.
	// 이를 통해 쓰레드마다 중복되지 않는 고유 번호를 안전하게 가질 수 있습니다.
	LThreadId = SThreadId.fetch_add(1);
}

// DestroyTLS 함수: 쓰레드 종료 시 필요한 정리 작업을 수행합니다.
void ThreadManager::DestroyTLS()
{
	// 현재는 구현 내용이 없으나, TLS 영역에 동적 할당된 메모리가 있다면
	// 메모리 누수 방지를 위해 여기서 해제 로직을 작성합니다.
}
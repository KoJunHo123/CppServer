#pragma once

/*-------------------
	RefCountable
-------------------*/

#pragma once

/*-------------------
	RefCountable
-------------------*/

class RefCountable
{
public:
	// 생성자: 객체가 처음 생성될 때 참조 횟수를 1로 초기화합니다.
	RefCountable() : RefCount(1) {}

	// 가상 소멸자: 상속 구조에서 안전하게 메모리를 해제하기 위해 virtual로 선언합니다.
	virtual ~RefCountable() {}

	// 현재 이 객체를 가리키고 있는 포인터의 개수를 반환합니다.
	int32 GetRefCount() { return RefCount; }

	// 참조 횟수를 1 증가시키고 변경된 값을 반환합니다.
	int32 AddRef() { return ++RefCount; }

	// 참조 횟수를 1 감소시키고 만약 0이 되면 객체 스스로를 메모리에서 삭제합니다.
	int32 ReleaseRef()
	{
		int32 refCount = --RefCount;
		if (0 == refCount)
		{
			// 참조하는 곳이 더 이상 없으므로 메모리 해제
			delete this;
		}
		return refCount;
	}

protected:
	// 멀티쓰레드 환경에서 안전하도록 원자적 연산을 지원하는 atomic 변수를 사용합니다.
	// 여러 쓰레드에서 동시에 AddRef나 ReleaseRef를 호출해도 값이 정확하게 보존됩니다.
	atomic<int32> RefCount;
};


/*-------------------
	SharedPtr
-------------------*/
template<typename T>
class TSharedPtr
{
public:
	// 기본 생성자: 관리할 포인터를 nullptr로 초기화합니다.
	TSharedPtr() {}

	// 인자 생성자: 생으로 전달된 포인터를 받아 참조 횟수를 올리고 관리 시작합니다.
	TSharedPtr(T* InPtr) { Set(InPtr); }

	// 복사 생성자: 다른 스마트 포인터가 가진 객체를 같이 가리키며 참조 횟수를 올립니다.
	TSharedPtr(const TSharedPtr& InRhs) { Set(InRhs.Ptr); }

	// 이동 생성자: 다른 스마트 포인터의 소유권을 뺏어옵니다. 참조 횟수 변화 없이 주소값만 옮깁니다.
	TSharedPtr(TSharedPtr&& InRhs) { Ptr = InRhs.Ptr; InRhs.Ptr = nullptr; }

	// 상속 관계 복사 생성자: 자식 클래스 포인터를 부모 클래스 포인터 타입으로 복사할 때 사용합니다.
	template<typename U>
	TSharedPtr(const TSharedPtr<U>& InRhs) { Set(static_cast<T*>(InRhs.Ptr)); }

	// 소멸자: 스마트 포인터가 사라질 때 관리하던 객체의 참조 횟수를 하나 줄입니다.
	~TSharedPtr() { Release(); }

public:
	// 복사 대입 연산자: 기존에 갖고 있던 객체는 놓아주고 새로운 객체를 가리킵니다.
	TSharedPtr& operator=(const TSharedPtr& InRhs)
	{
		// 같은 객체를 대입하려는 것이 아닐 때만 동작합니다.
		if (Ptr != InRhs.Ptr)
		{
			Release();
			Set(InRhs.Ptr);
		}
		return *this;
	}

	// 이동 대입 연산자: 기존 객체는 해제하고 상대방의 포인터 정보를 완전히 가져옵니다.
	TSharedPtr& operator=(TSharedPtr&& InRhs)
	{
		Release();
		Ptr = InRhs.Ptr;
		InRhs.Ptr = nullptr;
		return *this;
	}

	// 비교 연산자들: 내부의 실제 포인터 주소값을 비교합니다.
	bool		operator==(const TSharedPtr& InRhs) const { return Ptr == InRhs.Ptr; }
	bool		operator==(T* InPtr) const { return Ptr == InPtr; }
	bool		operator!=(const TSharedPtr& InRhs) const { return Ptr != InRhs.Ptr; }
	bool		operator!=(T* InPtr) const { return Ptr == InPtr; }
	bool		operator<(const TSharedPtr& InRhs) const { return Ptr < InRhs.Ptr; }

	// 역참조 연산자: 스마트 포인터를 일반 포인터처럼 사용할 수 있게 해줍니다.
	T* operator*() { return Ptr; }
	const T* operator*() const { return Ptr; }
	operator T* () const { return Ptr; }
	T* operator->() { return Ptr; }
	const T* operator->() const { return Ptr; }

	// 현재 포인터가 비어있는지 확인합니다.
	bool IsNull() { return nullptr == Ptr; }

public:
	// 내부 보조 함수: 새로운 포인터를 설정하고 참조 횟수를 증가시킵니다.
	inline void Set(T* InPtr)
	{
		Ptr = InPtr;
		if (InPtr)
		{
			InPtr->AddRef();
		}
	}

	// 내부 보조 함수: 현재 포인터의 참조 횟수를 줄이고 변수를 비웁니다.
	inline void Release()
	{
		if (nullptr != Ptr)
		{
			Ptr->ReleaseRef();
			Ptr = nullptr;
		}
	}

private:
	// 실제 객체를 가리키는 원시 포인터입니다.
	T* Ptr = nullptr;
};
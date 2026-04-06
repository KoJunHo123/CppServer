#pragma once

/*-------------------
	RefCountable
-------------------*/

class RefCountable
{
public:
	RefCountable() : RefCount(1) {}
	virtual ~RefCountable() {}

	int32 GetRefCount() { return RefCount; }
	int32 AddRef() { return ++RefCount; }
	int32 ReleaseRef()
	{
		int32 refCount = --RefCount;
		if (0 == refCount)
		{
			delete this;
		}
		return refCount;
	}

protected:
	atomic<int32> RefCount;
};


/*-------------------
	SharedPtr
-------------------*/
template<typename T>
class TSharedPtr
{
public:
	TSharedPtr() {}
	TSharedPtr(T* ptr) { Set(ptr); }

	// 복사
	TSharedPtr(const TSharedPtr& rhs) { Set(rhs.Ptr); }
	// 이동
	TSharedPtr(TSharedPtr&& rhs) { Ptr = rhs.Ptr; rhs.Ptr = nullptr; }
	// 상속 관계 복사
	template<typename U>
	TSharedPtr(const TSharedPtr<U>& rhs) { Set(static_cast<T*>(rhs.Ptr)); }

	~TSharedPtr() { Release(); }

public:
	// 복사 연산자
	TSharedPtr& operator=(const TSharedPtr& rhs)
	{
		if (Ptr != rhs.Ptr)
		{
			Release();
			Set(rhs.Ptr);
		}
		return *this;
	}

	// 이동 연산자
	TSharedPtr& operator=(TSharedPtr&& rhs)
	{
		Release();
		Ptr = rhs.Ptr;
		rhs.Ptr = nullptr;
		return *this;
	}

	bool		operator==(const TSharedPtr& rhs) const { return Ptr == rhs.Ptr; }
	bool		operator==(T* ptr) const { return Ptr == ptr; }
	bool		operator!=(const TSharedPtr& rhs) const { return Ptr == rhs.Ptr; }
	bool		operator!=(T* ptr) const { return Ptr == ptr; }
	bool		operator<(const TSharedPtr& rhs) const { return Ptr < rhs.Ptr; }
	T*			operator*() { return Ptr; }
	const T*	operator*() const { return Ptr; }
				operator T*() const { return Ptr; }
	T*			operator->() { return Ptr; }
	const T*	operator->() const { return Ptr; }


	bool IsNull() { return Ptr = nullptr; }

public:
	inline void Set(T* ptr)
	{
		Ptr = ptr;
		if (ptr)
		{
			ptr->AddRef();
		}
	}

	inline void Release()
	{
		if (nullptr != Ptr)
		{
			Ptr->ReleaseRef();
			Ptr = nullptr;
		}
	}

private:
	T* Ptr = nullptr;
};

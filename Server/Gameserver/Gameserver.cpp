#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <windows.h>
#include <future>
#include "ThreadManager.h"
#include "RefCounting.h"


class Wraith : public RefCountable
{
public:
	int Hp = 150;
	int PosX = 0;
	int PosY = 0;
};
using WraithRef = TSharedPtr<Wraith>;

class Missile : public RefCountable
{
public:
	void SetTarget(WraithRef target)
	{
		Target = target;
	}

	bool Update()
	{
		if (nullptr == Target)
		{
			return true;
		}

		int posX = Target->PosX;
		int oisY = Target->PosY;

		if (0 == Target->Hp)
		{
			Target = nullptr;
			return true;
		}
		return false;
	}

	WraithRef Target = nullptr;
};

using MissileRef = TSharedPtr<Missile>;

int main()
{
	WraithRef wraith = new Wraith();
	wraith->ReleaseRef();
	MissileRef missile = new Missile();
	missile->ReleaseRef();

	missile->SetTarget(wraith);

	wraith->Hp = 0;
	wraith = nullptr;

	while (true)
	{
		if (missile)
		{
			if (missile->Update())
			{
				missile = nullptr;
			}
		}
	}

}

#include "pch.h"
#include "ThreadManager.h"
#include "CoreTLS.h"
#include "CoreGlobal.h"
#include "GlobalQueue.h"

/*----------------------
	Threadmanager
-----------------------*/

ThreadManager::ThreadManager()
{
	InitTLS();
}

ThreadManager::~ThreadManager()
{
	Join();
}

void ThreadManager::Launch(function<void(void)> callback)
{
	LockGuard guard(_lock);

	// 스레드 생성 -> 실행 -> 관리
	_threads.push_back(thread([callback]()
		{
			InitTLS();  // TLS 데이터 초기화
			callback();
			DestroyTLS();  // TLS 데이터 정리
		}));
}

void ThreadManager::Join()
{
	for (thread& t : _threads)
	{
		if (t.joinable())
			t.join();
	}
	_threads.clear();
}


void ThreadManager::InitTLS()
{
	// TLS 아이디 지정
	static Atomic<uint32> SThreadId = 1;
	LThreadId = SThreadId.fetch_add(1);
}

/*
* 스레드 TLS 자원 정리
*/
void ThreadManager::DestroyTLS()
{
	// 동적 자원이 있다면 해제
}

void ThreadManager::DoGlobalQueueWork()
{
	while (true)
	{
		uint64 now = ::GetTickCount64();
		if (now > LEndTickCount)
			break;

		JobQueueRef jobQueue = GGlobalQueue->Pop();
		if (jobQueue == nullptr)
			break;

		jobQueue->Execute();
	}
}

void ThreadManager::DoDBQueueWork()
{
	while (true)
	{
		uint64 now = ::GetTickCount64();
		if (now > LEndDBTickCount)
			break;

		JobQueueRef jobQueue = GDBQueue->Pop();
		if (jobQueue == nullptr)
			break;

		jobQueue->Execute();
	}
}

void ThreadManager::DistributeReservedJobs()
{
	const uint64 now = ::GetTickCount64();
	GJobTimer->Distribute(now);
}

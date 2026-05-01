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
    // 메인 스레드도 스레드다 — TLS를 초기화해야 GMemory 기반 할당이 가능
    // ID=0 은 메인 스레드 예약, 실제 Worker 는 1부터 시작
    CoreTLS::OnThreadStart(ThreadType::LOGIC, 0);
}

ThreadManager::~ThreadManager()
{
    Join();
    // 메인 스레드 TLS 해제
    CoreTLS::OnThreadEnd();
}

void ThreadManager::Join()
{
    for (std::thread& t : _threads)
    {
        if (t.joinable())
            t.join();
    }
    _threads.clear();
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

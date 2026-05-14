#include "pch.h"
#include "ThreadManager.h"

ThreadManager::ThreadManager()
{
}

ThreadManager::~ThreadManager()
{
    Join();
}

void ThreadManager::InitMainThread(ThreadType type)
{
    uint32 nextId = _threadIdCounter.fetch_add(1);
    CoreTLS::OnThreadStart(type, nextId);
}

void ThreadManager::Join()
{
    for (Thread& t : _threads)
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
        if (::GetTickCount64() > LEndTickCount)
            break;

        JobQueueRef jobQueue = nullptr;
        if (!GGlobalQueue->TryPop(OUT jobQueue))
            break;

        jobQueue->Execute();
    }
}

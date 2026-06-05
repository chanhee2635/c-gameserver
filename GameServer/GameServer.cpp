#include "pch.h"
#include "Service.h"
#include "GameSession.h"
#include "ThreadManager.h"

enum
{
    WORKER_TICK = 16
};

void DoWorkerJob(ServerServiceRef& service)
{
    while (true)
    {
        LEndTickCount = ::GetTickCount64() + WORKER_TICK;

        service->IocpDispatch();

        GJobTimer->Distribute(::GetTickCount64());

        ThreadManager::DoGlobalQueueWork();

        LFrameAllocator->Clear();
    }
}

int main()
{
    SetCurrentDirectoryW(L"..\\..\\GameServer");
    GameGlobal::Init();

    auto service = GameGlobal::GetService();

    uint32_t workerThreadCount = 8;
    for (int32 i = 0; i < workerThreadCount; i++)
    {
        GThread->Launch(ThreadType::WORKER, [&service]() {
            DoWorkerJob(service);
        });
    }

    GThread->InitMainThread(ThreadType::MONITOR);
    if (GServerStats)
        GServerStats->RunUpdateLoop(); 

    GameGlobal::Clear();  
    return 0;
}

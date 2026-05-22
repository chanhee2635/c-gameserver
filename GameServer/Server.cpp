#include "pch.h"
#include "Service.h"
#include "GameSession.h"
#include "ThreadManager.h"

#include <timeapi.h>
#pragma comment(lib, "winmm.lib")

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

    auto service = MakeShared<ServerService>(
        NetAddress(L"127.0.0.1", 7777),
        MakeShared<IocpCore>(),
        []() { return MakeShared<GameSession>(); },
        2000
    );

    ASSERT_CRASH(service->Start());

    uint32_t workerThreadCount = std::thread::hardware_concurrency() - 4;
    for (int32 i = 0; i < workerThreadCount; i++)
    {
        GThread->Launch(ThreadType::WORKER, [&service]()
        {
            DoWorkerJob(service);
        });
    }

    GThread->InitMainThread(ThreadType::MONITOR);
    if (GServerStats)
        GServerStats->RunUpdateLoop(); 

    GameGlobal::Clear();  

    timeEndPeriod(1);
    return 0;
}

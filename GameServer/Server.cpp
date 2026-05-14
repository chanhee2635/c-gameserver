#include "pch.h"
#include "Service.h"
#include "GameSession.h"
#include "ThreadManager.h"

enum
{
    WORKER_TICK = 64
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
    GameGlobal::Init();

    auto service = MakeShared<ServerService>(
        NetAddress(L"127.0.0.1", 7777),
        MakeShared<IocpCore>(),
        []() { return MakeShared<GameSession>(); },
        2000
    );

    ASSERT_CRASH(service->Start());

    for (int32 i = 0; i < 5; i++)
    {
        GThread->Launch(ThreadType::WORKER, [&service]()
        {
            DoWorkerJob(service);
        });
    }

    GThread->InitMainThread(ThreadType::MONITOR);
    if (GServerStats)
        GServerStats->RunUpdateLoop();  // 메인 스레드가 모니터 루프 담당

    GameGlobal::Clear();  // Join + SocketUtils::Clear + 역순 해제 포함
    return 0;
}

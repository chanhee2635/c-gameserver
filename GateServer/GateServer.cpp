#include "pch.h"
#include "Service.h"
#include "GateSession.h"
#include "GateGlobal.h"
#include "ThreadManager.h"


enum { GATE_TICK = 16 };

void DoGateWorker(ServerServiceRef& service)
{
    while (true)
    {
        LEndTickCount = ::GetTickCount64() + GATE_TICK;
        service->IocpDispatch();
        GJobTimer->Distribute(::GetTickCount64());   // QueueManager Tick 디스패치
        ThreadManager::DoGlobalQueueWork();
        LFrameAllocator->Clear();
    }
}

int main()
{
    GateGlobal::Init();

    auto service = MakeShared<ServerService>(
        NetAddress(L"0.0.0.0", GateGlobal::GetConfig().listenPort),
        MakeShared<IocpCore>(),
        []() { return MakeShared<GateSession>(); },
        10000   
    );
    ASSERT_CRASH(service->Start());

    uint32 workerCount = 2;
    for (uint32 i = 0; i < workerCount; ++i)
        GThread->Launch(ThreadType::WORKER, [&service]() { DoGateWorker(service); });

    GThread->InitMainThread(ThreadType::MONITOR);
    while (true) { ::Sleep(1000); }   

    GateGlobal::Clear();
    return 0;
}
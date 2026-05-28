#include "pch.h"
#include "Service.h"
#include "GateSession.h"
#include "GateGlobal.h"
#include "ThreadManager.h"

#include <timeapi.h>
#pragma comment(lib, "winmm.lib")

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
    timeBeginPeriod(1);

    GateGlobal::Init();

    auto service = MakeShared<ServerService>(
        NetAddress(L"0.0.0.0", GateGlobal::GetConfig().listenPort),
        MakeShared<IocpCore>(),
        []() { return MakeShared<GateSession>(); },
        10000   // 대기자 대량 수용
    );
    ASSERT_CRASH(service->Start());

    uint32 workerCount = std::thread::hardware_concurrency();
    for (uint32 i = 0; i < workerCount; ++i)
        GThread->Launch(ThreadType::WORKER, [&service]() { DoGateWorker(service); });

    GThread->InitMainThread(ThreadType::MONITOR);
    while (true) { ::Sleep(1000); }   // 워커가 IOCP/타이머 처리, 메인은 보류

    GateGlobal::Clear();
    timeEndPeriod(1);
    return 0;
}
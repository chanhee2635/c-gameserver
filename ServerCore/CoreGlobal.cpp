#include "pch.h"
#include "CoreGlobal.h"
#include "Memory.h"
#include "ThreadManager.h"
#include "DeadLockProfiler.h"
#include "SendBuffer.h"
#include "GlobalQueue.h"
#include "JobTimer.h"
#include "DBConnectionPool.h"
#include "ConsoleLog.h"

// ── 선언 순서 = 초기화 순서 (역순으로 소멸) ──────────────────────────────
//
//  GMemory          먼저: ThreadManager 생성자가 CoreTLS::OnThreadStart()를 호출하고,
//                         OnThreadStart() → new ThreadLocalMemory() → GMemory->GetPool() 참조
//  GThreadManager   두 번째: 생성자에서 메인 스레드 CoreTLS::OnThreadStart() 호출
//  이후는 순서 무관
//
unique_ptr<MemoryManager>     GMemory            = make_unique<MemoryManager>();
unique_ptr<ThreadManager>     GThreadManager     = make_unique<ThreadManager>();
unique_ptr<SendBufferManager> GSendBufferManager = make_unique<SendBufferManager>();
unique_ptr<GlobalQueue>       GGlobalQueue       = make_unique<GlobalQueue>();
unique_ptr<GlobalQueue>       GDBQueue           = make_unique<GlobalQueue>();
unique_ptr<JobTimer>          GJobTimer          = make_unique<JobTimer>();
unique_ptr<DeadLockProfiler>  GDeadLockProfiler  = make_unique<DeadLockProfiler>();
unique_ptr<DBConnectionPool>  GDBConnectionPool  = make_unique<DBConnectionPool>();
unique_ptr<ConsoleLog>        GConsoleLogger     = make_unique<ConsoleLog>();

bool          GIsRunning     = true;
Atomic<int32> GLogicJobCount = 0;
Atomic<int32> GDBJobCount    = 0;

// ── CoreGlobal::Clear() ──────────────────────────────────────────────────────

void CoreGlobal::Clear()
{
    // 1. 서버 루프 종료 신호
    GIsRunning = false;

    // 2. 모든 Worker 스레드 완료 대기
    //    (각 스레드는 GIsRunning == false 를 보고 루프 탈출해야 함)
    GThreadManager->Join();

    // 3. 전역 자원 역순 해제 (초기화 역순: 의존성 안전)
    GConsoleLogger.reset();
    GDBConnectionPool.reset();
    GDeadLockProfiler.reset();
    GJobTimer.reset();
    GDBQueue.reset();
    GGlobalQueue.reset();
    GSendBufferManager.reset();

    // GThreadManager 소멸자가 메인 스레드 CoreTLS::OnThreadEnd() 호출
    GThreadManager.reset();

    // GMemory 는 반드시 마지막 (모든 블록 반환 완료 후)
    GMemory.reset();
}

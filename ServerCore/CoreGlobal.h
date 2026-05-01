#pragma once

class ThreadManager;
class MemoryManager;
class DeadLockProfiler;
class SendBufferManager;
class GlobalQueue;
class JobTimer;
class DBConnectionPool;
class ConsoleLog;
class Logger;

extern std::unique_ptr<MemoryManager>       GMemory;
extern std::unique_ptr<ThreadManager>       GThreadManager;
extern std::unique_ptr<SendBufferManager>   GSendBufferManager;
extern std::unique_ptr<GlobalQueue>         GGlobalQueue;
extern std::unique_ptr<GlobalQueue>         GDBQueue;
extern std::unique_ptr<JobTimer>            GJobTimer;
extern std::unique_ptr<DeadLockProfiler>    GDeadLockProfiler;
extern std::unique_ptr<DBConnectionPool>    GDBConnectionPool;
extern std::unique_ptr<ConsoleLog>          GConsoleLogger;  // 하위 호환 유지

extern bool           GIsRunning;
extern Atomic<int32>  GLogicJobCount;
extern Atomic<int32>  GDBJobCount;

/*--------------
    CoreGlobal
    명시적 초기화/종료 인터페이스
    - Init(): 사용 불필요 (unique_ptr 자동 초기화), 의존 순서는 CoreGlobal.cpp 선언 순서로 보장
    - Clear(): 서버 종료 시 반드시 호출 — GIsRunning=false 후 안전한 역순 해제
---------------*/
class CoreGlobal
{
public:
    /*
    * @brief 서버 루프 종료 시 호출
    *        1. GIsRunning = false  → 모든 스레드 루프 탈출 신호
    *        2. GThreadManager->Join() 으로 Worker 스레드 완료 대기
    *        3. 전역 객체를 역순으로 해제 (의존성 역순 보장)
    */
    static void Clear();
};

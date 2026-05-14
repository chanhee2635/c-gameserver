#include "pch.h"
#include "CoreGlobal.h"
#include "ThreadManager.h"
#include "IocpCore.h"

std::unique_ptr<ThreadManager>     CoreGlobal::_thread            = nullptr;
std::unique_ptr<MemoryManager>     CoreGlobal::_memory            = nullptr;
std::unique_ptr<SendBufferManager> CoreGlobal::_sendBufferManager = nullptr;
std::unique_ptr<JobTimer>          CoreGlobal::_jobTimer          = nullptr;
std::unique_ptr<GlobalQueue>       CoreGlobal::_globalQueue       = nullptr;
std::unique_ptr<Logger>            CoreGlobal::_logger            = nullptr;
std::unique_ptr<ServerStats>       CoreGlobal::_serverStats       = nullptr;
std::unique_ptr<DbConnectionPool>  CoreGlobal::_dbConnectionPool  = nullptr;

ThreadManager*     GThread            = nullptr;
MemoryManager*     GMemory            = nullptr;
SendBufferManager* GSendBufferManager = nullptr;
JobTimer*          GJobTimer          = nullptr;
GlobalQueue*       GGlobalQueue       = nullptr;
Logger*            GLogger            = nullptr;
ServerStats*       GServerStats       = nullptr;
DbConnectionPool*  GDbConnectionPool  = nullptr;

void CoreGlobal::Init()
{
    // 1. Memory pool — must come first (ThreadLocalMemory references GMemory)
    _memory = std::make_unique<MemoryManager>();
    GMemory = _memory.get();

    // 2. Socket initialization
    SocketUtils::Init();

    // 3. ThreadManager — must exist before Logger::Init() calls GThread->Launch()
    _thread = std::make_unique<ThreadManager>();
    GThread = _thread.get();

    // 4. Infrastructure
    _sendBufferManager = std::make_unique<SendBufferManager>();
    GSendBufferManager = _sendBufferManager.get();

    _globalQueue = std::make_unique<GlobalQueue>();
    GGlobalQueue = _globalQueue.get();

    _jobTimer = std::make_unique<JobTimer>();
    GJobTimer = _jobTimer.get();

    // 5. Logger — start log thread after GThread is ready
    _logger = std::make_unique<Logger>();
    GLogger = _logger.get();
    GLogger->Init(L"server.log");

    // 6. ServerStats — init after GLogger is ready
    _serverStats = std::make_unique<ServerStats>();
    GServerStats = _serverStats.get();
    GServerStats->Init();

    _dbConnectionPool = std::make_unique<DbConnectionPool>();
    GDbConnectionPool = _dbConnectionPool.get();

    LOG_INFO(L"=== [Server Engine Initialization Complete] ===");
}

void CoreGlobal::Clear()
{
    LOG_INFO(L"=== [Server Engine Shutting Down] ===");

    // Destroy in reverse order: Stats → Logger → Infrastructure → Thread → Memory
    GDbConnectionPool = nullptr;
    _dbConnectionPool = nullptr;

    GServerStats = nullptr;
    _serverStats = nullptr;

    GJobTimer = nullptr;
    _jobTimer = nullptr;

    GGlobalQueue = nullptr;
    _globalQueue = nullptr;

    GSendBufferManager = nullptr;
    _sendBufferManager = nullptr;

    // Logger destructor shuts down the log thread (calls Shutdown())
    GLogger = nullptr;
    _logger = nullptr;

    // Wait for all worker threads then release
    if (GThread)
        GThread->Join();
    GThread = nullptr;
    _thread = nullptr;

    SocketUtils::Clear();

    GMemory = nullptr;
    _memory = nullptr;
}

#pragma once

extern class ThreadManager*     GThread;
extern class MemoryManager*     GMemory;
extern class SendBufferManager* GSendBufferManager;
extern class JobTimer*          GJobTimer;
extern class GlobalQueue*       GGlobalQueue;
extern class Logger*            GLogger;
extern class ServerStats*       GServerStats;
extern class DbConnectionPool*  GDbConnectionPool;

class CoreGlobal
{
public:
    static void Init();
    static void EnableServerInstrumentation();
    static void Clear();

private:
    static std::unique_ptr<class ThreadManager>     _thread;
    static std::unique_ptr<class MemoryManager>     _memory;
    static std::unique_ptr<class SendBufferManager> _sendBufferManager;
    static std::unique_ptr<class JobTimer>          _jobTimer;
    static std::unique_ptr<class GlobalQueue>       _globalQueue;
    static std::unique_ptr<class Logger>            _logger;
    static std::unique_ptr<class ServerStats>       _serverStats;
    static std::unique_ptr<class DbConnectionPool>  _dbConnectionPool;
};

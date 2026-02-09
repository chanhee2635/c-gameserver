#pragma once

class ThreadManager;
class Memory;
class DeadLockProfiler;
class SendBufferManager;
class GlobalQueue;
class JobTimer;
class DBConnectionPool;
class ConsoleLog;

extern std::unique_ptr<ThreadManager>		GThreadManager;
extern std::unique_ptr<Memory>				GMemory;
extern std::unique_ptr<SendBufferManager>	GSendBufferManager;
extern std::unique_ptr<GlobalQueue>			GGlobalQueue;
extern std::unique_ptr<GlobalQueue>			GDBQueue;
extern std::unique_ptr<JobTimer>			GJobTimer;
extern std::unique_ptr<DeadLockProfiler>	GDeadLockProfiler;
extern std::unique_ptr<DBConnectionPool>	GDBConnectionPool;
extern std::unique_ptr<ConsoleLog>			GConsoleLogger;

extern bool GIsRunning;
extern Atomic<int32> GLogicJobCount;
extern Atomic<int32> GDBJobCount;

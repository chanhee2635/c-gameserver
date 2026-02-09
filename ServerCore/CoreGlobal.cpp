#include "pch.h"
#include "CoreGlobal.h"
#include "ThreadManager.h"
#include "Memory.h"
#include "DeadLockProfiler.h"
#include "SendBuffer.h"
#include "GlobalQueue.h"
#include "JobTimer.h"
#include "DBConnectionPool.h"
#include "ConsoleLog.h"

unique_ptr<ThreadManager>		GThreadManager = make_unique<ThreadManager>();
unique_ptr<Memory>				GMemory = make_unique<Memory>();
unique_ptr<SendBufferManager>	GSendBufferManager = make_unique<SendBufferManager>();
unique_ptr<GlobalQueue>			GGlobalQueue = make_unique<GlobalQueue>();
unique_ptr<GlobalQueue>			GDBQueue = make_unique<GlobalQueue>();
unique_ptr<JobTimer>			GJobTimer = make_unique<JobTimer>();
unique_ptr<DeadLockProfiler>	GDeadLockProfiler = make_unique<DeadLockProfiler>();
unique_ptr<DBConnectionPool>	GDBConnectionPool = make_unique<DBConnectionPool>();
unique_ptr<ConsoleLog>			GConsoleLogger = make_unique<ConsoleLog>();

bool GIsRunning = true;
Atomic<int32> GLogicJobCount = 0;
Atomic<int32> GDBJobCount = 0;
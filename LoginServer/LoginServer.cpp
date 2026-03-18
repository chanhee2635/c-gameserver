#include "pch.h"
#include "Service.h"
#include "LoginSession.h"
#include "ThreadManager.h"
#include "ClientPacketHandler.h"
#include "DBConnectionPool.h"
#include "RedisManager.h"
#include "ConfigManager.h"
#include "DBService.h"

void DoWorkerJob(LoginServiceRef& service, uint64 workedTick)
{
	while (true)
	{
		LEndTickCount = ::GetTickCount64() + workedTick;

		service->GetIocpCore()->Dispatch(1);

		// 예약된 일감 처리
		ThreadManager::DistributeReservedJobs();

		// 글로벌 큐
		ThreadManager::DoGlobalQueueWork();
	}
}

int main()
{
    SocketUtils::Init();

	if (GConfigManager->LoadConfig("../../Common/config.json") == false)
	{
		std::wcout << "Config Load Failed!" << std::endl;
		return 0;
	}

	auto dbConfig = GConfigManager->GetDB();
	auto serverConfig = GConfigManager->GetLogin();

	if (GDBConnectionPool->Connect(serverConfig.dbThreadCount + 2, dbConfig.connectionString.c_str()) == false)
	{
		std::wcout << "DB Connect Failed!" << std::endl;
		return 0;
	}

	ClientPacketHandler::Init();

	LoginServiceRef service = MakeShared<LoginService>(
		NetAddress(serverConfig.ip, serverConfig.port),
		MakeShared<IocpCore>(),
		MakeShared<LoginSession>
	);

	GDBService->Start(serverConfig.dbThreadCount, serverConfig.dbWorkedTick);

	bool isStarted = service->Start();
	ASSERT_CRASH(isStarted);

	for (int32 i = 0; i < serverConfig.workerThreadCount; i++)
	{
		GThreadManager->Launch([&service, workedTick = serverConfig.logicWorkedTick]()
			{
				DoWorkerJob(service, workedTick);
			});
	}

	/*uint64 nextMonitorTick = ::GetTickCount64() + 1000;

	while (GIsRunning)
	{
		uint64 currentTick = ::GetTickCount64();

		if (currentTick >= nextMonitorTick)
		{
			Utils::PrintServerStatus(service);
			nextMonitorTick = currentTick + 1000;
		}

		this_thread::sleep_for(10ms);
	}*/

	GThreadManager->Join();
	GDBConnectionPool->Clear();
	SocketUtils::Clear();

	return 0;
}

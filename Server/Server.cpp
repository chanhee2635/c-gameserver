#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "ClientPacketHandler.h"
#include "DBConnectionPool.h"
#include "DataManager.h"
#include "World.h"
#include "NavigationManager.h"
#include "GameSession.h"
#include "ConfigManager.h"
#include "RedisManager.h"
#include "DBService.h"
#include "DBManager.h"

void DoWorkerJob(ServerServiceRef& service, uint64 workedTick)
{
	while (true)
	{
		LEndTickCount = ::GetTickCount64() + workedTick;

		service->GetIocpCore()->Dispatch(1);

		ThreadManager::DistributeReservedJobs();

		ThreadManager::DoGlobalQueueWork();
	}
}

void UpdateServerStatusLoop(ServerServiceRef& service) {

	auto config = GConfigManager->GetGame();
	auto logicConfig = GConfigManager->GetLogic();
	string key = "Server:" + to_string(config.port);

	uint64 nextMonitorTick = 0;
	uint64 nextRedisTick = 0;

	while (GIsRunning)
	{
		uint64 currentTick = ::GetTickCount64();

		//if (currentTick >= nextMonitorTick)
		//{
		//	//Utils::PrintServerStatus(service);
		//	nextMonitorTick = currentTick + logicConfig.monitorTick;
		//}

		if (currentTick >= nextRedisTick)
		{
			int32 currentSessionCount = service->GetCurrentSessionCount();

			string info = std::to_string(config.id) + "|" +
						Utils::ws2s(config.name) + "|" +
						Utils::ws2s(config.ip) + "|" +
						std::to_string(config.port) + "|" + 
						std::to_string(currentSessionCount) + "|" + 
						std::to_string(config.maxSessionCount);

			GRedisManager->SaveServerStatus(key, info, 10);
			nextRedisTick = currentTick + logicConfig.redisTick;
		}

		this_thread::sleep_for(10ms);
	}
}

int main()
{
	GDBManager = make_shared<DBManager>();
	GWorld = make_shared<World>();

	SocketUtils::Init();

	if (GConfigManager->LoadConfig("../../Common/config.json") == false)
	{
		std::wcout << "Config Load Failed!" << std::endl;
		return 0;
	}
	if (GDataManager->LoadData() == false)
	{
		std::wcout << "Data Load Failed!" << std::endl;
		return 0;
	}

	auto& dbConfig = GConfigManager->GetDB();
	auto& serverConfig = GConfigManager->GetGame();
	if (GRedisManager->Connect() == false)
	{
		std::wcout << "Redis Connect Failed!" << std::endl;
		return 0;
	}

	if (GDBConnectionPool->Connect(serverConfig.dbThreadCount+ 2, dbConfig.connectionString.c_str()) == false)
	{
		std::wcout << "DB Connect Failed!" << std::endl;
		return 0;
	}

	if (GNavigationManager->LoadNavMesh("../../NavDataGenerator/SceneNavMesh.nav") == false)
	{
		std::wcout << "NavMesh Load Failed!" << endl;
		return 0;
	}
	
	GWorld->Init();

	ClientPacketHandler::Init();

	ServerServiceRef service = MakeShared<ServerService>(
		NetAddress(serverConfig.ip, serverConfig.port),
		MakeShared<IocpCore>(),
		MakeShared<GameSession>
	);

	GDBService->Start(serverConfig.dbThreadCount, serverConfig.dbWorkedTick);

	bool isStarted = service->Start();
	ASSERT_CRASH(isStarted);

	GWorld->Start();

	for (int32 i = 0; i < serverConfig.workerThreadCount; i++)
	{
		GThreadManager->Launch([&service, workedTick = serverConfig.logicWorkedTick]()
			{
				DoWorkerJob(service, workedTick);
			});
	}

	UpdateServerStatusLoop(service);

	GThreadManager->Join();
	GDBConnectionPool->Clear();
	SocketUtils::Clear();

	return 0;
}

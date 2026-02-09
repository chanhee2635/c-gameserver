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

		service->GetIocpCore()->Dispatch(10);

		// 예약된 일감 처리
		ThreadManager::DistributeReservedJobs();

		// 글로벌 큐
		ThreadManager::DoGlobalQueueWork();
	}
}

int main()
{
    SocketUtils::Init();

	if (GConfigManager->LoadConfig("../Common/config.json") == false)
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

	ASSERT_CRASH(service->Start());

	for (int32 i = 0; i < serverConfig.workerThreadCount; i++)
	{
		GThreadManager->Launch([&service, workedTick = serverConfig.logicWorkedTick]()
			{
				DoWorkerJob(service, workedTick);
			});
	}

	uint64 nextMonitorTick = ::GetTickCount64() + 1000;

	while (GIsRunning)
	{
		uint64 currentTick = ::GetTickCount64();

		if (currentTick >= nextMonitorTick)
		{
			Utils::PrintServerStatus(service);
			nextMonitorTick = currentTick + 1000;
		}

		this_thread::sleep_for(10ms);
	}

	GThreadManager->Join();
	GDBConnectionPool->Clear();
	SocketUtils::Clear();

	return 0;
}

// Drop Table
	/*{
		auto query = L"DROP TABLE IF EXISTS Accounts;";

		DBConnectionRef dbConn = GDBConnectionPool->Pop();
		ASSERT_CRASH(dbConn->Execute(query));
	}*/

	// Create Table
	/*{
		auto query = L"												\
			CREATE TABLE Accounts(									\
				account_id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,			\
				login_id NVARCHAR(20) NOT NULL UNIQUE,				\
				password NVARCHAR(50) NOT NULL,						\
				created_date DATETIME DEFAULT CURRENT_TIMESTAMP);";

		DBConnectionRef dbConn = GDBConnectionPool->Pop();
		ASSERT_CRASH(dbConn->Execute(query));
	}*/

	// Drop Procedure
	/*{
		auto query = L"DROP PROCEDURE IF EXISTS sp_create_account;";

		DBConnectionRef dbConn = GDBConnectionPool->Pop();
		ASSERT_CRASH(dbConn->Execute(query));
	}*/

	// Create Procedure
	/*{
		auto query = L"							\
			CREATE PROCEDURE sp_create_account(	\
				IN p_login_id NVARCHAR(20),		\
				IN p_password NVARCHAR(50)		\
			)									\
			BEGIN								\
				IF EXISTS (SELECT 1 FROM Accounts WHERE login_id = p_login_id) THEN \
					SELECT 1 AS result;			\
				ELSE							\
					INSERT INTO Accounts (login_id, password) VALUES (p_login_id, p_password);	\
					SELECT 0 AS result;			\
				END IF;							\
			END;";

		DBConnectionRef dbConn = GDBConnectionPool->Pop();
		ASSERT_CRASH(dbConn->Execute(query));
	}*/
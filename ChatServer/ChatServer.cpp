#include "pch.h"
#include "ChatSession.h"
#include "ClientPacketHandler.h"
#include "RedisManager.h"
#include "Service.h"
#include "ThreadManager.h"
#include "SessionManager.h"

void DoWorkerJob(ChatServiceRef& service, uint64 workedTick)
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
	GSessionManager = make_unique<SessionManager>();

	SocketUtils::Init();

	if (GConfigManager->LoadConfig("../../Common/config.json") == false)
	{
		std::wcout << "Config Load Failed!" << std::endl;
		return 0;
	}

	auto serverConfig = GConfigManager->GetChat();

	ClientPacketHandler::Init();

	ChatServiceRef service = MakeShared<ChatService>(
		NetAddress(serverConfig.ip, serverConfig.port),
		MakeShared<IocpCore>(),
		MakeShared<ChatSession>
	);

	GSessionManager->Start();

	bool isStarted = service->Start();
	ASSERT_CRASH(isStarted);

	GThreadManager->Launch([]() {
		GRedisManager->SubscribeRedis();
	});

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
	SocketUtils::Clear();

	return 0;
}


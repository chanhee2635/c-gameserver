#include "pch.h"
#include "ChatSession.h"
#include "ClientPacketHandler.h"
#include "RedisManager.h"
#include "Service.h"
#include "ThreadManager.h"

void DoWorkerJob(ChatServiceRef& service, uint64 workedTick)
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

	if (GConfigManager->LoadConfig("../Common/config.json"))
	{
	}

	auto serverConfig = GConfigManager->GetChat();

	ClientPacketHandler::Init();

	ChatServiceRef service = MakeShared<ChatService>(
		NetAddress(serverConfig.ip, serverConfig.port),
		MakeShared<IocpCore>(),
		MakeShared<ChatSession>
	);

	ASSERT_CRASH(service->Start());

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
	SocketUtils::Clear();

	return 0;
}


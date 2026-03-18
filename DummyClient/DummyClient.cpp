#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "ServerPacketHandler.h"
#include "ConfigManager.h"
#include "DummyUser.h"
#include "GameSession.h"
#include "ChatSession.h"

int main()
{
	GIocpCore = MakeShared<IocpCore>();

	SocketUtils::Init();

	if (GConfigManager->LoadConfig("../../Common/config.json") == false)
		return 0;

	auto& gameConfig = GConfigManager->GetGame();
	auto& chatConfig = GConfigManager->GetChat();

	this_thread::sleep_for(10s);

	ServerPacketHandler::Init();

	GGameService = MakeShared<ClientService>(
		NetAddress(gameConfig.ip, gameConfig.port),
		GIocpCore,
		MakeShared<GameSession> 
	);

	GChatService = MakeShared<ClientService>(
		NetAddress(chatConfig.ip, chatConfig.port),
		GIocpCore,
		MakeShared<ChatSession>
	);

	GGameService->Start();
	GChatService->Start();

	for (int32 i = 0; i < 1; i++)
	{
		GThreadManager->Launch([=]()
			{
				while (true)
				{
					GIocpCore->Dispatch(10);
				}
			});
	}

	for (int32 i = 0; i < 200; ++i)
	{
		DummyUserRef user = MakeShared<DummyUser>();
		GUsers.push_back(user);
		user->ConnectToGame();
		this_thread::sleep_for(10ms);
	}

	const int32 UPDATE_THREAD_COUNT = 1;
	int32 totalUsers = (int32)GUsers.size();

	for (int32 t = 0; t < UPDATE_THREAD_COUNT; ++t)
	{
		GThreadManager->Launch([t, totalUsers]() {
			int32 begin = (totalUsers / UPDATE_THREAD_COUNT) * t;
			int32 end = (t == UPDATE_THREAD_COUNT - 1) ? totalUsers : begin + (totalUsers / UPDATE_THREAD_COUNT);

			while (true)
			{
				int64 loopStart = (int64)::GetTickCount64();

				uint64 now = loopStart;
				for (int32 i = begin; i < end; ++i)
					GUsers[i]->Update(now);

				int64 elapsed = (int64)::GetTickCount64() - loopStart;
				int64 sleepMs = 100 - elapsed;
				if (sleepMs > 0)
					this_thread::sleep_for(chrono::milliseconds(sleepMs));
			}
		});
	}

	GThreadManager->Join();
	SocketUtils::Clear();
}

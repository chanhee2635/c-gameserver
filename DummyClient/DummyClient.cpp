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
	SocketUtils::Init();

	if (GConfigManager->LoadConfig("../Common/config.json") == false)
	{
		return 0;
	}

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

	for (int32 i = 0; i < 4; i++)
	{
		GThreadManager->Launch([=]()
			{
				while (true)
				{
					GIocpCore->Dispatch();
				}
			});
	}

	for (int32 i = 0; i < 100; ++i)
	{
		DummyUserRef user = MakeShared<DummyUser>();
		GUsers.push_back(user);

		user->ConnectToGame();

		this_thread::sleep_for(100ms);
	}

	GThreadManager->Launch([=]() {
		while (true)
		{
			uint64 now = ::GetTickCount64();

			for (auto& user : GUsers)
			{
				user->Update(now);
			}

			this_thread::sleep_for(33ms);
		}
	});

	GThreadManager->Join();
	SocketUtils::Clear();
}

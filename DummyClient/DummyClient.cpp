#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "ServerPacketHandler.h"
#include "ConfigManager.h"
#include "DummyUser.h"
#include "GameSession.h"
#include "ChatSession.h"

USE_LOCK;
std::mutex GUserMutex;
vector<DummyUserRef> GUsers;

Atomic<int32> GTargetCount = 0;

void AddUsers(int32 count)
{
	for (int32 i = 0; i < count; ++i)
	{
		DummyUserRef user = MakeShared<DummyUser>();
		{
			std::lock_guard<std::mutex> lock(GUserMutex);  // ✅
			GUsers.push_back(user);
		}
		user->ConnectToGame();

		if (i % 50 == 49)
			this_thread::sleep_for(1000ms);
		else
			this_thread::sleep_for(10ms);
	}
}

void RemoveUsers(int32 count)
{
	for (int32 i = 0; i < count; ++i)
	{
		DummyUserRef user = nullptr;
		{
			std::lock_guard<std::mutex> lock(GUserMutex);  // ✅
			if (GUsers.empty()) break;
			user = GUsers.back();
			GUsers.pop_back();
		}

		if (user)
			user->Disconnect();

		this_thread::sleep_for(10ms);
	}
}

void InputLoop()
{
	while (true)
	{
		int32 input = 0;
		cin >> input;
		if (input < 0) continue;

		int32 current = 0;
		{
			std::lock_guard<std::mutex> lock(GUserMutex);  // ✅
			current = static_cast<int32>(GUsers.size());
		}

		int32 diff = input - current;

		if (diff > 0)
		{
			cout << "▶ " << diff << "명 추가 (" << current << "명 → " << input << "명)" << endl;
			GThreadManager->Launch(ThreadType::LOGIC, [diff]() { AddUsers(diff); });
		}
		else if (diff < 0)
		{
			cout << "▶ " << -diff << "명 제거 (" << current << "명 → " << input << "명)" << endl;
			GThreadManager->Launch(ThreadType::LOGIC, [diff]() { RemoveUsers(-diff); });
		}
		else
		{
			cout << "현재 인원과 동일: " << current << "명" << endl;
		}
	}
}


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
		GThreadManager->Launch(ThreadType::IO, [=]()
			{
				while (true)
					GIocpCore->Dispatch(10);
			});
	}

	GThreadManager->Launch(ThreadType::LOGIC, []()
	{
		while (true)
		{
			int64 loopStart = (int64)::GetTickCount64();
			uint64 now = (uint64)loopStart;

			// snapshot으로 복사 (lock 최소화)
			vector<DummyUserRef> snapshot;
			{
				std::lock_guard<std::mutex> lock(GUserMutex);
				snapshot = GUsers;
			}

			for (auto& user : snapshot)
				user->Update(now);

			int64 elapsed = (int64)::GetTickCount64() - loopStart;
			int64 sleepMs = 100 - elapsed;
			if (sleepMs > 0)
				this_thread::sleep_for(chrono::milliseconds(sleepMs));
		}
	});

	GThreadManager->Launch(ThreadType::MONITOR, []() { InputLoop(); });

	GThreadManager->Join();
	SocketUtils::Clear();
}

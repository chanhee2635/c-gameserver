#pragma once

class DummyUser : public enable_shared_from_this<DummyUser>
{
public:
	void ConnectToGame();
	void ConnectToChat();
	void Update(uint64 now);

	void SendRandomChat();
	void SendRandomMove();

	void ConnectCompleted() { isConnected = true; }

	uint64 GetObjectId();
	ChatSessionRef GetChatSession() { return _chatSession; }

private:
	GameSessionRef _gameSession;
	ChatSessionRef _chatSession;

	bool isConnected = false;

	uint64 _nextChatTick;
	uint64 _nextMoveTick;
};


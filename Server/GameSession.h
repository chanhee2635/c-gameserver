#pragma once
#include "Session.h"

class GameSession : public PacketSession
{
public:
	GameSession() = default;
	~GameSession() = default;

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;

	void Handle_C_AuthToken(string token);
	void Handle_C_CreatePlayer(int32 classType, string name);
	void Handle_C_EnterGame(wstring playerName);
	void Handle_C_LoadCompleted();
	void Handle_C_Move(Protocol::C_Move& packet);

	void SendSummaries(Vector<PlayerSummaryData> summaries);

	uint64 GetAccountDbId() { return _accountDbId; }
	void SetAccountDbId(uint64 accountDbId) { _accountDbId = accountDbId; }
	void AddSummary(PlayerSummaryData summary) { _summaries[summary.name] = std::move(summary); }
	PlayerRef GetPlayer() { return _player; }
	void SetPlayer(PlayerRef player) { _player = player; }

private:
	uint64 _accountDbId = 0;
	Map<wstring, PlayerSummaryData> _summaries;

	PlayerRef _player;
};


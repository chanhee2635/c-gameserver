#pragma once
#include "Session.h"

class GameSession : public PacketSession
{
public:
	GameSession() {}
	~GameSession()
	{
		cout << "~GameSession" << endl;
	}

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;

	void Handle_C_AuthToken(string token);
	void Handle_C_CreatePlayer(int32 classType, string name);
	void Handle_C_EnterGame(wstring playerName);
	void Handle_C_LoadCompleted();

	void SendObjectSummaries(Vector<SummaryDataRef> summaries);
	void SendAddPlayerSummary(SummaryDataRef summary);

	PlayerRef GetPlayer() { return _player; }
	void SetPlayer(PlayerRef player) { _player = player; }

public:
	uint64 accountDbId = 0;
	map<wstring, SummaryDataRef> _summaries;
	PlayerRef _player;
};


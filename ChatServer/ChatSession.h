#pragma once
#include "Session.h"

class ChatSession : public PacketSession
{
public:
	ChatSession() {}
	~ChatSession()
	{
	}

	virtual void OnConnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;
	virtual void OnDisconnected() override;

	void SetPlayerId(uint64 playerId) { _playerId = playerId; }
	uint64 GetPlayerId() { return _playerId; }
	void SetZoneId(int32 zoneId) { _zoneId = zoneId; }
	int32 GetZoneId() { return _zoneId; }
	void SetName(string name) { _name = name; }
	string GetName() { return _name; }

private:
	uint64 _playerId = 0;
	int32 _zoneId = -1;
	string _name = "";
};

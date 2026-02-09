#pragma once
#include "DummyUser.h"
class ChatSession : public PacketSession
{
public:
	ChatSession() { _dummyId = DummyIdGenerator.fetch_add(1); }
	~ChatSession()
	{
	}

	void SetOwner(DummyUserRef owner) { _onwer = owner; }
	DummyUserRef GetOwner() { return _onwer.lock(); }
	void SetPlayerId(uint64 playerId) { _playerId = playerId; }

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;

public:
	static Atomic<int32> DummyIdGenerator;
	int32 _dummyId = 0;
	uint64 _playerId;

private:
	weak_ptr<DummyUser> _onwer;
};


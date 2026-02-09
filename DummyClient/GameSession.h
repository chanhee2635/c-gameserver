#pragma once
#include "DummyUser.h"
class GameSession : public PacketSession
{
public:
	GameSession() { _dummyId = DummyIdGenerator.fetch_add(1); }
	~GameSession()
	{
		cout << "~GameSession" << endl;
	}

	void SetOwner(DummyUserRef owner) { _onwer = owner; }
	DummyUserRef GetOwner() { return _onwer.lock(); }
	void SetObjectInfo(Protocol::ObjectInfo info) 
	{ 
		_info = info; 
		_posInfo = info.posinfo();
	}
	uint64 GetObjectId() { return _info.summary().objectid(); }
	string GetName() { return _info.summary().name(); }
	void SetPosX(float posx) { _posInfo.set_x(_posInfo.x() + posx); }
	void SetPosZ(float posz) { _posInfo.set_z(_posInfo.z() + posz); }
	void SetState(Protocol::CreatureState state) { _posInfo.set_state(state); }
	Protocol::PosInfo GetPosInfo() { return _posInfo; }

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;

public:
	static Atomic<int32> DummyIdGenerator;
	int32 _dummyId = 0;

private:
	weak_ptr<DummyUser> _onwer;

	Protocol::ObjectInfo _info;
	Protocol::PosInfo _posInfo;
};


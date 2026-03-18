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

	void SetOwner(DummyUserRef owner) { _owner = owner; }
	DummyUserRef GetOwner() { return _owner.lock(); }
	void SetObjectInfo(Protocol::ObjectInfo info) 
	{ 
		_info = info; 
		_posInfo = info.pos_info();
	}
	uint64 GetObjectId() { return _info.summary().object_id(); }
	string GetName() { return _info.summary().name(); }
	void SetPos(float x, float y, float z) { _posInfo.mutable_pos()->set_x(x);_posInfo.mutable_pos()->set_y(y);_posInfo.mutable_pos()->set_z(z);}
	void SetPosX(float dx) { _posInfo.mutable_pos()->set_x(_posInfo.pos().x() + dx); }
	float GetPosX() { return _posInfo.pos().x(); }
	void SetPosZ(float dz) { _posInfo.mutable_pos()->set_z(_posInfo.pos().z() + dz); }
	float GetPosZ() { return _posInfo.pos().z(); }
	void SetState(Protocol::CreatureState state) { _posInfo.set_state(state); }
	void SetYaw(float yaw) { _posInfo.set_yaw(yaw); }
	Protocol::PosInfo GetPosInfo() { return _posInfo; }

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;

public:
	static Atomic<int32> DummyIdGenerator;
	int32 _dummyId = 0;

private:
	weak_ptr<DummyUser> _owner;

	Protocol::ObjectInfo _info;
	Protocol::PosInfo _posInfo;
};


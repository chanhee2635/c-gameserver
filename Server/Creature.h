#pragma once
#include "GameObject.h"

class Creature : public GameObject
{
public:
	Creature() {};
	virtual ~Creature() = default;

	int32 GetLevel() { return _summary->level; }
	virtual int32 GetDamage() = 0;
	virtual int32 GetDefense() = 0;
	virtual uint64 GetAttackTick() = 0;
	void SetPosInfo(Vector3 pos, Protocol::PosInfo posInfo);
	CreatureState GetState() const { return _stats.state; }
	void SetState(CreatureState state);
	void SetState(Protocol::CreatureState state) { SetState(static_cast<CreatureState>(state)); }
	void SetPos(Vector3 pos) { _stats.pos = pos; }
	void SetYaw(float yaw) { _stats.yaw = yaw; }
	float GetYaw() { return _stats.yaw; }
	void SetIsRun(bool isRun) { _stats.isRun = isRun; }
	int32 GetHp() { return _stats.hp; }
	void SetHp(int32 hp) { _stats.hp = hp; }
	std::wstring GetName() { return _summary->name; }


	bool CanAttack(int32 comboIndex, uint64 currentTick);
	void SetAttackTick(uint64 tick);
	int32 CalculateDamage(CreatureRef target);
	void OnDamage(int32 damage);
	bool IsDead();

	void MakePosInfo(OUT Protocol::PosInfo& info);
	void MakeObjectInfo(OUT Protocol::ObjectInfo& info);

protected:
	int32 _maxCombo = 0;
	uint64 _lastAttackTick = 0;
};


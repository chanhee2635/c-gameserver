#pragma once
#include "GameObject.h"

class Creature : public GameObject
{
public:
	Creature() = default;
	virtual ~Creature() = default;

	CreatureState	GetState() const { return _state; }
	void			SetState(CreatureState state) { _state = state; }
	bool			IsDead()   const { return _hp <= 0 || _state == CreatureState::Dead; }

	ZoneRef GetZone() { return _zone.lock(); }
	void	SetZone(ZoneRef zone) { _zone = zone; }

	bool IsDirty()	const { return _isDirty; }
	void SetDirty(bool flag) { _isDirty = flag; }

	void MakePosInfo(Protocol::PosInfo& info) const override;
	void MakeStatInfo(Protocol::StatInfo& info) const override;

	virtual float GetAttackSpeed() const { return 1.0f; }
	uint64 GetHitDelay(int32 comboIndex) const;
	virtual int32 GetDefense() const { return 0; }
	int32 GetHp() const { return _hp; }
	int32 TakeDamage(int32 incomingAttack);

protected:
	virtual void OnDead() {}

	CreatureState _state	= CreatureState::Idle;
	int32		  _hp		= 0;
	float		  _speed	= 0.f;

	int32	_maxCombo = 0;
	bool	_isDirty = false;

	weak_ptr<Zone>	_zone;
};


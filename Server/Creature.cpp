#include "pch.h"
#include "Creature.h"
#include "World.h"
#include "DataManager.h"

void Creature::MakePosInfo(Protocol::PosInfo& info) const
{
	info.set_object_id(_objectId);
	info.set_state(static_cast<Protocol::CreatureState>(_state));
	info.mutable_pos()->set_x(_pos.x);
	info.mutable_pos()->set_y(_pos.y);
	info.mutable_pos()->set_z(_pos.z);
	info.set_yaw(_yaw);
}

void Creature::MakeStatInfo(Protocol::StatInfo& info) const
{
	info.set_hp(_hp);
}

uint64 Creature::GetHitDelay(int32 comboIndex) const
{
	const PrefabData* prefab = GDataManager->GetPrefabData(_templateId);
	if (prefab == nullptr) return 0;
	if (comboIndex < 0 || comboIndex >= (int32)prefab->comboHitDelays.size()) return 0;

	float delayMs = prefab->comboHitDelays[comboIndex] / max(GetAttackSpeed(), 0.1f);
	return static_cast<uint64>(delayMs);
}

int32 Creature::TakeDamage(int32 incomingAttack)
{
	if (IsDead()) return 0;

	int32 actualDamage = max(1, incomingAttack - GetDefense());
	_hp = max(0, _hp - actualDamage);

	if (_hp <= 0)
	{
		_hp = 0;
		_state = CreatureState::Dead;
		OnDead();
	}

	return actualDamage;
}


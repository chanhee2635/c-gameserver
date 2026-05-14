#include "pch.h"
#include "Creature.h"
#include "DataManager.h"

void Creature::MakePosInfo(Protocol::PosInfo& info) const
{
    info.set_object_id(_objectId);
    info.set_state(_state);
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
    if (!_prefabData) return 0;
    if (comboIndex < 0 || comboIndex >= (int32)_prefabData->comboHitDelays.size()) return 0;

    float delayMs = _prefabData->comboHitDelays[comboIndex] / std::max(GetAttackSpeed(), 0.1f);
    return static_cast<uint64>(delayMs);
}

int32 Creature::TakeDamage(int32 incomingAttack)
{
    if (IsDead()) return 0;

    int32 damage = std::max(1, incomingAttack - GetDefense());
    _hp = std::max(0, _hp - damage);

    if (_hp <= 0)
    {
        _hp = 0;
        SetState(Protocol::DEAD);
        OnDead();
    }

    return damage;
}
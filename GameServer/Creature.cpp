#include "pch.h"
#include "Creature.h"
#include "DataManager.h"
#include "GameConfig.h"
#include "GameUtil.h"

void Creature::MakePosInfo(Protocol::PosInfo& info) const
{
    info.set_object_id(_objectId);
    info.set_state(_state);
    *info.mutable_pos() = GameUtil::ToProto(_pos);
    info.set_yaw(_yaw);
    *info.mutable_velocity() = GameUtil::ToProto(_velocity);
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

bool Creature::ShouldBroadcastMove(uint64 nowMs) const
{
    if (_state != _lastSentState)                                                  // 애니 상태 변화
        return true;
    if ((_velocity - _lastSentVelocity).LengthSq() > GameConfig::Move::VEL_EPS_SQ) // 속도 변화(출발/정지/회전/sprint)
        return true;
    if ((_pos - PredictPos(nowMs)).LengthSq() > GameConfig::Move::POS_EPS_SQ)       // 위치 발산(벽 등)
        return true;
    if (!_velocity.IsZero() && nowMs - _lastSentTick >= GameConfig::Move::HEARTBEAT_MS) // 이동 중 하트비트
        return true;
    return false;
}

Vector3 Creature::PredictPos(uint64 nowMs) const
{
    float dt = (nowMs - _lastSentTick) / 1000.f;
    return _lastSentPos + _lastSentVelocity * dt;
}

void Creature::CommitMoveAnchor(uint64 nowMs)
{
    _lastSentPos = _pos;
    _lastSentVelocity = _velocity;
    _lastSentState = _state;
    _lastSentTick = nowMs;
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


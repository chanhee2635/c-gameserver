#include <algorithm>
#include "pch.h"
#include "Creature.h"
#include "GameScene.h"
#include "ClientPacketHandler.h"

void Creature::SetPosInfo(Vector3 pos, Protocol::PosInfo posInfo)
{
	SetState(posInfo.state());
	SetPos(pos);
	SetYaw(posInfo.yaw());
	SetIsRun(posInfo.isrun());
}

void Creature::SetState(CreatureState state)
{
	if (_stats.state == state)
		return;

	// 이전 State
	switch (_stats.state)
	{
		case CreatureState::Idle:
			break;
		case CreatureState::Moving:
			break;
		case CreatureState::Attack:
			break;
		case CreatureState::Dead:
			break;
	}

	_stats.state = state;

	// 새로운 State
	switch (_stats.state)
	{
	case CreatureState::Idle:
		break;
	case CreatureState::Moving:
		break;
	case CreatureState::Attack:
		break;
	case CreatureState::Dead:
		break;
	}

	Protocol::S_ChangeState packet;
	packet.set_objectid(_summary->objectId);
	auto posInfo = packet.mutable_posinfo();
	MakePosInfo(*posInfo);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);
	GetGameScene()->BroadcastAround(GetPos(), sendBuffer, GetObjectId());
}

bool Creature::IsDead()
{
	return _stats.hp <= 0 || _stats.state == CreatureState::Dead;
}

void Creature::MakePosInfo(Protocol::PosInfo& info)
{
	_stats.ToPacket(&info);
}

void Creature::MakeObjectInfo(Protocol::ObjectInfo& info)
{
	_summary->ToPacket(info.mutable_summary());
	_stats.ToPacket(info.mutable_posinfo());
	_stats.ToPacket(info.mutable_statinfo());
}

int32 Creature::CalculateDamage(CreatureRef target)
{
	int32 damage =  target->GetDefense() - GetDamage();
	return damage > 0 ? 0 : damage;
}

void Creature::SetAttackTick(uint64 tick)
{
	_lastAttackTick = tick;
}

bool Creature::CanAttack(int32 comboIndex, uint64 currentTick)
{
	if (IsDead()) 
		return false;

	// TODO: animation duration 을 가져와 체크

	if (0 >= comboIndex && _maxCombo < comboIndex) 
		return false;
	return true;
}

void Creature::OnDamage(int32 damage)
{
	if (IsDead()) return;

	int32 hp = _stats.hp + damage;
	if (hp <= 0)
	{
		hp = 0;
		_stats.state = CreatureState::Dead;
	}
	_stats.hp = hp;
}



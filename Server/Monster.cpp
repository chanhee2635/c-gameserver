#include "pch.h"
#include "Monster.h"
#include "IdGenerator.h"
#include "DataManager.h"
#include "Player.h"
#include "World.h"
#include "GameScene.h"
#include "Zone.h"
#include "NavigationManager.h"
#include "ClientPacketHandler.h"


void Monster::Init(const SpawnData& data)
{
	_config = GDataManager->GetMonsterData(data.id);

	_summary = MakeShared<SummaryData>();
	_summary->objectType = GameObjectType::Monster;
	_summary->objectId = IdGenerator::GenerateId(_summary->objectType);
	_summary->level = _config->level;
	_summary->name = _config->name;
	_summary->templateId = _config->id;

	_stats.pos = Vector3(data.pos.x, GNavigationManager->GetHeight(data.pos), data.pos.z);
	_stats.yaw = data.yaw;
	_stats.hp = _config->maxHp;

	_nextSearchTick = ::GetTickCount64();
	_respawnTick = data.respawnTick;
	_spawnPos = _stats.pos;
	_maxCombo = GDataManager->GetMaxCombo(_summary->templateId);
}

void Monster::Reset()
{
	_nextSearchTick = ::GetTickCount64();
	_stats.state = CreatureState::Idle;
	_stats.hp = _config->maxHp;
	_stats.pos = _spawnPos;
	_target.reset();
}

void Monster::Update()
{
	switch (GetState())
	{
		case CreatureState::Idle:
			UpdateIdle(); break;
		case CreatureState::Moving:
			if (_target.lock() != nullptr)
				UpdateMoving();
			else
				UpdateReturn();
			break;
		case CreatureState::Attack:
			UpdateAttack(); break;
		case CreatureState::Dead:
			UpdateDead(); break;
	}
}

void Monster::HandleGatherResult(PlayerRef player, float distSq)
{
	bool shouldChange = (_target.lock() == nullptr || _target.lock()->IsDead());

	if (_target.lock() != nullptr)
		if (distSq < _lastTargetDistSq) shouldChange = true;
	else
		shouldChange = true;

	if (shouldChange)
	{
		_target = player;
		_lastTargetDistSq = distSq;

		if (_stats.state == CreatureState::Idle)
			_stats.state = CreatureState::Moving;
	}
}

void Monster::UpdateIdle()
{
	uint64 now = ::GetTickCount64();
	if (now < _nextSearchTick) return;
	_nextSearchTick = now + SEARCH_TICK;

	auto adjacentZones = GWorld->GetAdjacentZones(_stats.pos);
	Map<GameSceneRef, Vector<ZoneRef>> sceneGroups;
	for (ZoneRef zone : adjacentZones)
	{
		if (GameSceneRef scene = zone->GetScene())
			sceneGroups[scene].push_back(zone);
	}
	
	auto self = static_pointer_cast<Monster>(shared_from_this());
	Vector3 monsterPos = _stats.pos;

	for (auto& item : sceneGroups)
	{
		GameSceneRef scene = item.first;
		Vector<ZoneRef> zones = item.second;
		scene->DoAsync(&GameScene::FindNearestPlayer, zones, self, monsterPos);
	}
}

void Monster::UpdateMoving()
{
	PlayerRef target = _target.lock();
	if (target == nullptr || target->IsDead())
	{
		_target.reset();
		return;
	}

	Vector3 targetPos = target->GetPos();
	// TODO: y축 고려
	float distSq = Vector3::DistanceSquared(_stats.pos, targetPos);

	if (distSq > GetMaxSearchRangeSq())
	{
		_target.reset();
		return;
	}

	if (distSq <= GetAttackRangeSq())
	{
		_stats.state = CreatureState::Attack;
		return;
	}

	TickMoveTo(targetPos);
}

void Monster::UpdateReturn()
{
	float distSq = Vector3::DistanceSquared(_stats.pos, _spawnPos);

	if (distSq < 0.5f * 0.5f)
	{
		_stats.pos = _spawnPos;
		_stats.state = CreatureState::Idle;
		_path.clear();
		auto self = static_pointer_cast<Monster>(shared_from_this());
		GetGameScene()->HandleMove(self, _stats.pos, _spawnPos);
		return;
	}

	TickMoveTo(_spawnPos);
}

void Monster::UpdateAttack()
{
	if (IsDead()) return;
	// 공격 시간이 지났으면 체크
	uint64 currentTick = GetTickCount64();
	if (_lastAttackTick + GetAttackTick() > currentTick)
		return;

	// 타겟이 있는지를 체크
	PlayerRef target = _target.lock();
	if (target == nullptr || target->IsDead())
	{
		_target.reset();
		_stats.state = CreatureState::Moving;
		return;
	}

	// 타겟이 공격 범위에 있는지를 체크
	float distSq = Vector3::DistanceSquared(_stats.pos, target->GetPos());
	if (distSq > GetAttackRangeSq())
	{
		_stats.state = CreatureState::Moving;
		return;
	}

	float dx = target->GetPos().x - _stats.pos.x;
	float dz = target->GetPos().z - _stats.pos.z;
	float degree = atan2(dx, dz) * (180.0f / 3.141592f);
	SetYaw(degree);

	SetAttackTick(currentTick);

	Protocol::S_Attack packet;
	packet.set_objectid(GetObjectId());
	packet.set_comboindex(_maxCombo);
	packet.set_yaw(degree);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);
	GetGameScene()->BroadcastAround(GetPos(), sendBuffer, GetObjectId());

	MonsterRef self = static_pointer_cast<Monster>(shared_from_this());
	// Target의 GameScene 을 호출해서 damage 를 입힌다.
	// 그런데 현재 target 의 GameScene 이 실행된 GameScene 과 다르다면 target의 상태를 변경하면 안된다.
	target->GetGameScene()->DoTimer(DAMAGE_TICK, &GameScene::ApplyDamage, (CreatureRef)self, (CreatureRef)target);
}

void Monster::UpdateDead()
{
}

void Monster::TickMoveTo(Vector3 targetPos)
{
	Vector3 myPos = _stats.pos;

	float distToLastTarget = Vector3::DistanceSquared(_lastTargetPos, targetPos);
	if (_path.empty() || distToLastTarget > 1.0f * 1.0f)
	{
		_path.clear();
		if (GNavigationManager->FindPath(myPos, targetPos, _path))
		{
			_pathIndex = 1;
			_lastTargetPos = targetPos;
		}
		else
			return;
	}

	if (_pathIndex >= _path.size()) return;
	Vector3 nextWaypoint = _path[_pathIndex];

	Vector3 dir = nextWaypoint - myPos;
	float distToPointSq = Vector3::DistanceSquared(nextWaypoint, myPos);
	if (distToPointSq < 0.2f * 0.2f)
	{
		++_pathIndex;
		if (_pathIndex >= _path.size()) return;
		nextWaypoint = _path[_pathIndex];
		dir = nextWaypoint - myPos;
	}

	dir.Normalize();

	float moveDist = GetConfig()->speed * 0.1f;
	Vector3 nextPos = myPos + (dir * moveDist);
	nextPos.y = GNavigationManager->GetHeight(nextPos);

	SetYaw(::atan2(dir.x, dir.z));
	SetPos(nextPos);

	auto self = static_pointer_cast<Monster>(shared_from_this());
	GetGameScene()->HandleMove(self, myPos, nextPos);
}
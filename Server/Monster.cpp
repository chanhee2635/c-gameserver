#include "pch.h"
#include "Monster.h"
#include "IdGenerator.h"
#include "DataManager.h"
#include "Player.h"
#include "GameScene.h"
#include "Zone.h"
#include "NavigationManager.h"
#include "ClientPacketHandler.h"
#include "ConfigManager.h"
#include "World.h"

void Monster::Init(const SpawnData& data)
{
	_config = GDataManager->GetMonsterData(data.id);
	ASSERT_CRASH(_config != nullptr);

	_objectId = IdGenerator::GenerateId(GameObjectType::Monster);
	_objectType = GameObjectType::Monster;
	_templateId = _config->id;
	_name = _config->name;
	_level = _config->level;

	_speed = _config->speed;
	_maxCombo = GDataManager->GetMaxCombo(_templateId);

	_spawnPos = Vector3(data.pos.x, GNavigationManager->GetHeight(data.pos), data.pos.z);
	_yaw = data.yaw;

	_deltaTime = static_cast<float>(GConfigManager->GetLogic().updateTick) / 1000.0f;
	_respawnTick = data.respawnTick;

	Reset();
}

void Monster::Reset()
{
	_state = CreatureState::Idle;
	_hp = _config->maxHp;
	_pos = _spawnPos;
	_isDirty = true;
	_target.reset();
	_path.clear();
	_pathIndex = 0;
	_nextSearchTick = ::GetTickCount64();
	_nextAttackTick = 0;
	_lastTargetDistSq = 0.f;
}

void Monster::Update()
{
	switch (_state)
	{
		case CreatureState::Idle:   UpdateIdle(); break;
		case CreatureState::Moving: (_target.lock() != nullptr) ? UpdateMoving() : UpdateReturn(); break;
		case CreatureState::Attack:	UpdateAttack(); break;
		case CreatureState::Dead:   UpdateDead(); break;
	}
}

void Monster::HandleGatherResult(PlayerRef player, float distSq)
{
	PlayerRef currentTarget = _target.lock();

	bool shouldChange = (currentTarget == nullptr || currentTarget->IsDead() || distSq < _lastTargetDistSq);
	if (!shouldChange) return;

	_target = player;
	_lastTargetDistSq = distSq;

	if (_state == CreatureState::Idle)
	{
		_state = CreatureState::Moving;
		_path.clear();
		_pathIndex = 0;
		_isDirty = true;
	}
}


void Monster::OnDead()
{
	GameSceneRef scene = GetGameScene();
	ZoneRef zone = GetZone();
	if (!scene || !zone) return;

	Protocol::S_Die diePkt;
	diePkt.set_object_id(_objectId);
	scene->BroadcastToAdjacentZones(zone, ClientPacketHandler::MakeSendBuffer(diePkt));

	auto self = static_pointer_cast<Monster>(shared_from_this());

	int32 deadAnimMs = GDataManager->GetDeathDuration(_templateId);
	
	scene->DoTimer(deadAnimMs, [self, respawnTick = _respawnTick]() {
		GWorld->DoAsyncPush([self, respawnTick]() {
			GWorld->LeaveCreature(self);
			GWorld->DoTimer(respawnTick, [self]() {
				self->Reset();
				GWorld->EnterCreature(self);
			});
		});
	});
}

void Monster::UpdateIdle()
{
	int64 now = ::GetTickCount64();
	if (now < _nextSearchTick) return;
	_nextSearchTick = now + 500;

	ZoneRef myZone = GetZone();
	if (myZone == nullptr) return;

	auto self = static_pointer_cast<Monster>(shared_from_this());
	Vector3 monsterPos = _pos;

	Map<GameSceneRef, Vector<ZoneRef>> sceneGroups;
	for (ZoneRef zone : myZone->GetAdjacentZones())
		sceneGroups[zone->GetScene()].push_back(zone);

	for (auto& [scene, zones] : sceneGroups)
	{
		if (scene.get() == GetGameScene().get())
			GetGameScene()->FindNearestPlayer(zones, self, monsterPos);
		else
			scene->DoAsync(&GameScene::FindNearestPlayer, zones, self, monsterPos);
	}
}

void Monster::UpdateMoving()
{
	PlayerRef target = _target.lock();
	if (target == nullptr || target->IsDead())
	{
		_target.reset();
		_path.clear();
		_pathIndex = 0;
		_isDirty = true;
		return;
	}

	Vector3 targetPos = target->GetPos();
	float distSq = Vector3::DistanceSquared(_pos, targetPos);
	if (distSq > GetMaxSearchRangeSq())
	{
		_target.reset();
		_path.clear();
		_pathIndex = 0;
		_isDirty = true;
		return;
	}

	if (distSq <= GetAttackRangeSq())
	{
		_state = CreatureState::Attack;
		_isDirty = true;
		UpdateAttack();
		return;
	}

	TickMoveTo(targetPos);
}

void Monster::UpdateReturn()
{
	if (Vector3::DistanceSquared(_pos, _spawnPos) < 0.1f)
	{
		_pos = _spawnPos;
		_path.clear();
		_pathIndex = 0;
		_state = CreatureState::Idle;
		_nextSearchTick = ::GetTickCount64();
		_isDirty = true;
		return;
	}

	TickMoveTo(_spawnPos);

	if (_state == CreatureState::Idle)
	{
		_pos = _spawnPos;
		_path.clear();
		_pathIndex = 0;
		_nextSearchTick = ::GetTickCount64();
		_isDirty = true;
	}
}

void Monster::UpdateAttack()
{
	PlayerRef target = _target.lock();
	
	if (target == nullptr || target->IsDead())
	{
		_target.reset();
		_state = CreatureState::Moving;
		_path.clear();
		_pathIndex = 0;
		_isDirty = true;
		return;
	}

	float distSq = Vector3::DistanceSquared(_pos, target->GetPos());
	if (distSq > GetAttackRangeSq() * 1.1f)
	{
		_state = CreatureState::Moving;
		_path.clear();
		_pathIndex = 0;
		_isDirty = true;
		return;
	}

	int64 now = ::GetTickCount64();
	if (now < _nextAttackTick) return;

	ExecuteAttack(now, target);
}

void Monster::UpdateDead()
{
}

void Monster::TickMoveTo(Vector3 targetPos)
{
	if (_path.empty() || Vector3::DistanceSquared(_lastTargetPos, targetPos) > 2.0f)
	{
		_path.clear();
		if (GNavigationManager->FindPath(_pos, targetPos, _path))
		{
			_pathIndex = 1;
			_lastTargetPos = targetPos;
		}
		else
			return;
	}

	if (_pathIndex >= (int32)_path.size())
	{
		_path.clear();
		_pathIndex = 0;
		return;
	}

	Vector3 nextWaypoint = _path[_pathIndex];
	Vector3 dir = nextWaypoint - _pos;

	float distToPointSq = Vector3::DistanceSquared(nextWaypoint, _pos);
	if (distToPointSq < 0.2f * 0.2f)
	{
		++_pathIndex;
		if (_pathIndex >= _path.size())
			return;
		nextWaypoint = _path[_pathIndex];
		dir = nextWaypoint - _pos;
	}

	dir.Normalize();

	float moveDist = _speed * _deltaTime;
	Vector3 nextPos = _pos + (dir * moveDist);
	nextPos.y = GNavigationManager->GetHeight(nextPos);

	float degree = ::atan2f(dir.x, dir.z) * (180.0f / 3.141592f);
	if (degree < 0)
		degree += 360.0f;
	_yaw = degree;
	_pos = nextPos;

	_isDirty = true;
}

void Monster::ExecuteAttack(int64 now, PlayerRef target)
{
	uint64 cooldownMs = static_cast<uint64>(1000.0f * max(_config->attackSpeed, 0.1f));
	_nextAttackTick = now + cooldownMs;

	Vector3 toTarget = target->GetPos() - _pos;
	toTarget.y = 0.f;
	if (toTarget.LengthSquared() > 0.001f)
	{
		toTarget.Normalize();
		float degree = ::atan2f(toTarget.x, toTarget.z) * (180.0f / 3.141592f);
		if (degree < 0) degree += 360.0f;
		_yaw = degree;
		_isDirty = true;
	}

	GameSceneRef scene = GetGameScene();
	ZoneRef zone = GetZone();
	if (scene && zone)
	{
		Protocol::S_Attack packet;
		packet.set_object_id(_objectId);
		packet.set_yaw(_yaw);
		packet.set_combo_index(1);
		*packet.mutable_pos() = GameUtil::ToProto(_pos);

		scene->BroadcastToAdjacentZones(zone, ClientPacketHandler::MakeSendBuffer(packet));
	}

	auto self = static_pointer_cast<Monster>(shared_from_this());
	uint64 hitDelay = GetHitDelay(1);

	scene->DoTimer(hitDelay, [self, target]() {
		self->ApplyAttackDamage(target);
	});
}

void Monster::ApplyAttackDamage(PlayerRef target)
{
	if (IsDead()) return;
	if (target == nullptr || target->IsDead()) return;

	float distSq = Vector3::DistanceSquared(_pos, target->GetPos());
	if (distSq > GetAttackRangeSq() * 4.0f)  // 2배 사거리까지 허용 (레이턴시 보정)
		return;

	int32 actualDamage = target->TakeDamage(_config->attack);
	if (actualDamage <= 0) return;

	GameSceneRef scene = GetGameScene();
	ZoneRef zone = GetZone();
	if (scene == nullptr || zone == nullptr) return;

	Protocol::S_ChangeHp pkt;
	pkt.set_object_id(target->GetObjectId());
	pkt.set_hp(target->GetHp());
	pkt.set_damage(-actualDamage);  // 음수로 구분 (피해)

	scene->BroadcastToAdjacentZones(zone, ClientPacketHandler::MakeSendBuffer(pkt));

	if (target->IsDead())
	{
		Protocol::S_Die diePkt;
		diePkt.set_object_id(target->GetObjectId());
		scene->BroadcastToAdjacentZones(zone, ClientPacketHandler::MakeSendBuffer(diePkt));
	}
}


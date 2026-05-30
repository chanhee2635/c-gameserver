#include "pch.h"
#include "Monster.h"
#include "IdGenerator.h"
#include "DataManager.h"
#include "Player.h"
#include "GameScene.h"
#include "Zone.h"
#include "GridMap.h"
#include "GameGlobal.h"
#include "Protocol/PacketUtils.h"
#include "GameUtil.h"

void Monster::Init(const SpawnData& data)
{
    _config = GDataManager->GetMonsterData(data.id);
    ASSERT_CRASH(_config != nullptr);

    _objectId = IdGenerator::Generate();
    _objectType = Protocol::GameObjectType::MONSTER;
    _templateId = _config->id;
    _level = _config->level;
    SetName(_config->name);

    _speed = _config->speed;
    _maxHp = _config->maxHp;
    _spawnPos = data.pos;
    _yaw = data.yaw;
    _respawnDelayMs = data.respawnDelayMs;

    _prefabData  = GDataManager->GetPrefabData(_templateId);
    _searchTick  = GameGlobal::GetConfig().gameplay.monsterSearchTickMs;

    Reset();
}

void Monster::Reset()
{
    _state = Protocol::IDLE;
    _hp = _config->maxHp;
    _pos = _spawnPos;
    _target.reset();
    _path.clear();
    _pathIndex = 0;
    _nextSearchTick = _nowTick;
    _nextAttackTick = 0;
    _lastTargetDistSq = 0.f;
    _velocity = Vector3::Zero();
}

void Monster::Update(uint32 deltaTimeMs)
{
    _deltaTimeMs = deltaTimeMs;
    _nowTick = static_cast<int64>(::GetTickCount64());

    if (_state == Protocol::IDLE)
    {
        if (_nowTick < _nextUpdateTick) return;
        _nextUpdateTick = _nowTick + GameGlobal::GetConfig().gameplay.monsterIdleTickMs;
    }

    switch (_state)
    {
    case Protocol::IDLE:    UpdateIdle();    break;
    case Protocol::MOVING:  if (_target.lock()) UpdateMoving(); else UpdateReturn(); break;
    case Protocol::ATTACK:  UpdateAttack();  break;
    case Protocol::DEAD:    UpdateDead();    break;
    default: break;
    }
}

void Monster::HandleGatherResult(PlayerRef player, float distSq)
{
    PlayerRef current = _target.lock();
    bool shouldChange = (current == nullptr || current->IsDead() || distSq < _lastTargetDistSq);
    if (!shouldChange) return;

    _target = player;
    _lastTargetDistSq = distSq;

    if (_state == Protocol::IDLE)
    {
        _state = Protocol::MOVING;
        ResetPath();
    }
}

void Monster::OnDead()
{
    LOG_INFO(L"Monster dead templateId=" + std::to_wstring(_templateId));
}

void Monster::UpdateIdle()
{
    if (_nowTick < _nextSearchTick) return;
    _nextSearchTick = _nowTick + _searchTick;

    ZoneRef myZone = GetZone();
    if (!myZone) return;

    GameScene* myScene = GetGameSceneRaw();
    if (!myScene) return;

    auto    self = std::static_pointer_cast<Monster>(shared_from_this());
    Vector3 monsterPos = _pos;

    HashMap<GameScene*, Vector<ZoneRef>> sceneGroups;
    for (const ZoneRef& zone : myZone->GetAdjacentZones())
    {
        GameSceneRef scene = zone->GetScene();
        if (scene)
            sceneGroups[scene.get()].push_back(zone);
    }

    for (auto& [scenePtr, zones] : sceneGroups)
    {
        if (scenePtr == myScene)
            myScene->FindNearestPlayer(zones, self, monsterPos);
        else
        {
            GameSceneRef targetScene = zones[0]->GetScene();
            if (!targetScene) continue;
            targetScene->DoAsync(&GameScene::FindNearestPlayer,
                Vector<ZoneRef>(zones.begin(), zones.end()), self, monsterPos);
        }
    }
}

void Monster::UpdateMoving()
{
    PlayerRef target = _target.lock();
    if (!target || target->IsDead())
    {
        _target.reset();
        ResetPath();
        return;
    }

    Vector3 targetPos = target->GetPos();
    float   distSq = _pos.DistanceSq(targetPos);

    if (distSq > GetMaxSearchRangeSq())
    {
        _target.reset();
        ResetPath();
        return;
    }

    if (distSq <= GetAttackRangeSq())
    {
        _state = Protocol::ATTACK;
        _velocity = Vector3::Zero();
        UpdateAttack();
        return;
    }

    TickMoveTo(targetPos);
}

void Monster::UpdateReturn()
{
    if (_pos.DistanceSq(_spawnPos) < 1.0f)
    {
        _pos = _spawnPos;
        _path.clear();
        _pathIndex = 0;
        _state = Protocol::IDLE;
        _nextSearchTick = _nowTick;
        _velocity = Vector3::Zero();
        return;
    }

    TickMoveTo(_spawnPos);
}

void Monster::UpdateAttack()
{
    PlayerRef target = _target.lock();
    if (!target || target->IsDead())
    {
        _target.reset();
        _state = Protocol::MOVING;
        ResetPath();
        return;
    }

    if (_pos.DistanceSq(target->GetPos()) > GetAttackRangeSq() * ATTACK_RANGE_HYSTERESIS)
    {
        _state = Protocol::MOVING;
        ResetPath();
        return;
    }

    if (_nowTick < _nextAttackTick) return;

    ExecuteAttack(_nowTick, target);
}

void Monster::UpdateDead() {}  // 사망 연출/리스폰은 GameScene::HandleMonsterDead 타이머가 처리

void Monster::AddToScene(GameScene* scene)
{
    scene->AddMonster(std::static_pointer_cast<Monster>(shared_from_this()));
}

void Monster::RemoveFromScene(GameScene* scene)
{
    scene->RemoveMonster(_objectId);
}

void Monster::TickMoveTo(Vector3 targetPos)
{
    bool needsRepath = _path.empty() || (_lastTargetPos.DistanceSq(targetPos) > 1.0f && _nowTick >= _nextFindPathTick);

    if (needsRepath)
    {
        _path.clear();
        bool found = false;

        if (GGridMap && GGridMap->IsLoaded())
            found = GGridMap->FindPath(_pos, targetPos, _path);

        if (!found)
        {
            _path = { _pos, targetPos };
            found = true;
            _nextFindPathTick = _nowTick + GameGlobal::GetConfig().gameplay.findPathFailCooldownMs;
        }
        else
        {
            _nextFindPathTick = _nowTick + GameGlobal::GetConfig().gameplay.monsterRepathCooldownMs;
        }

        _pathIndex        = 1;
        _lastTargetPos    = targetPos;
    }

    if (_pathIndex >= static_cast<int32>(_path.size()))
    {
        _path.clear();
        _pathIndex = 0;
        return;
    }

    if (_path.back().DistanceSq(targetPos) > 0.1f)
        _path.back() = targetPos;

    Vector3 nextWaypoint = _path[_pathIndex];

    if (_pos.DistanceSq(nextWaypoint) < ARRIVE_DIST_SQ)
    {
        ++_pathIndex;
        if (_pathIndex >= static_cast<int32>(_path.size())) return;
        nextWaypoint = _path[_pathIndex];
    }

    Vector3 dir = (nextWaypoint - _pos).Normalized();
    _velocity = dir * _speed;
    _pos = _pos + _velocity * (_deltaTimeMs * 0.001f);
    _yaw = GameUtil::YawFromDir(dir);
}

void Monster::ExecuteAttack(int64 now, PlayerRef target)
{
    uint64 cooldownMs = static_cast<uint64>(1000.f / std::max(GetAttackSpeed(), 0.1f));  // ms
    _nextAttackTick = now + static_cast<int64>(cooldownMs);

    Vector3 toTarget = target->GetPos() - _pos;
    toTarget.y = 0.f;
    if (!toTarget.IsZero())
        _yaw = GameUtil::YawFromDir(toTarget);

    GameScene* scene = GetGameSceneRaw();
    if (!scene) return;

    ZoneRef      zone = GetZone();
    if (!zone) return;

    uint64 hitDelay = GetHitDelay(1);

    Protocol::SAttack packet;
    packet.set_object_id(_objectId);
    packet.set_yaw(_yaw);
    packet.set_combo_index(1);
    *packet.mutable_pos() = GameUtil::ToProto(_pos);
    scene->BroadcastToAdjacentZones(zone,
        MakeSendBuffer<Protocol::MsgId::S_ATTACK>(packet));

    auto   self = std::static_pointer_cast<Monster>(shared_from_this());

    scene->DoTimer(hitDelay, [self, target]()
        {
            self->ApplyAttackDamage(target);
        });
}

void Monster::ApplyAttackDamage(PlayerRef target)
{
    if (IsDead()) return;
    if (!target || target->IsDead()) return;

    // 허용 범위 초과 시 무효 (레이턴시 허용)
    if (_pos.DistanceSq(target->GetPos()) > GetAttackRangeSq() * ATTACK_HIT_LENIENCY_MULT) return;

    int32 damage = target->TakeDamage(_config->attack);
    if (damage <= 0) return;

    GameScene*   scene = GetGameSceneRaw();
    ZoneRef      zone = GetZone();
    if (!scene || !zone) return;

    scene->BroadcastHpChange(zone, target->GetObjectId(), target->GetHp(), -damage);

    if (target->IsDead())
        scene->BroadcastDie(zone, target->GetObjectId());
}
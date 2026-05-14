#include "pch.h"
#include "Monster.h"
#include "IdGenerator.h"
#include "DataManager.h"
#include "Player.h"
#include "GameScene.h"
#include "Zone.h"
#include "NavigationManager.h"
#include "Protocol/PacketUtils.h"
#include "GameUtil.h"

void Monster::Init(const SpawnData& data)
{
    _config = GDataManager->GetMonsterData(data.id);
    ASSERT_CRASH(_config != nullptr);

    _objectId = IdGenerator::Generate();
    _objectType = Protocol::MONSTER;
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
    _isDirty = true;
    _target.reset();
    _path.clear();
    _pathIndex = 0;
    _nextSearchTick = _nowTick;
    _nextAttackTick = 0;
    _lastTargetDistSq = 0.f;
}

void Monster::Update(float deltaTime)
{
    _deltaTime = deltaTime;
    _nowTick = static_cast<int64>(::GetTickCount64());

    if (_state == Protocol::IDLE ||
        (_state == Protocol::MOVING && !_target.lock()))
    {
        if (_nowTick < _nextUpdateTick) return;
        _nextUpdateTick = _nowTick + 500;
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
        _isDirty = true;
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

    myScene->FindNearestPlayer(myZone->GetAdjacentZones(), self, monsterPos);
}

void Monster::UpdateMoving()
{
    PlayerRef target = _target.lock();
    if (!target || target->IsDead())
    {
        _target.reset();
        ResetPath();
        _isDirty = true;
        return;
    }

    Vector3 targetPos = target->GetPos();
    float   distSq = _pos.DistanceSq(targetPos);

    if (distSq > GetMaxSearchRangeSq())
    {
        _target.reset();
        ResetPath();
        _isDirty = true;
        return;
    }

    if (distSq <= GetAttackRangeSq())
    {
        _state = Protocol::ATTACK;
        _isDirty = true;
        UpdateAttack();
        return;
    }

    TickMoveTo(targetPos);
}

void Monster::UpdateReturn()
{
    if (_pos.DistanceSq(_spawnPos) < 0.1f)
    {
        _pos = _spawnPos;
        _path.clear();
        _pathIndex = 0;
        _state = Protocol::IDLE;
        _nextSearchTick = _nowTick;
        _isDirty = true;
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
        _isDirty = true;
        return;
    }

    if (_pos.DistanceSq(target->GetPos()) > GetAttackRangeSq() * 1.1f)
    {
        _state = Protocol::MOVING;
        ResetPath();
        _isDirty = true;
        return;
    }

    if (_nowTick < _nextAttackTick) return;

    ExecuteAttack(_nowTick, target);
}

void Monster::UpdateDead() {}

void Monster::TickMoveTo(Vector3 targetPos)
{
    bool needsRepath = _path.empty() || _lastTargetPos.DistanceSq(targetPos) > 2.0f;

    if (needsRepath)
    {
        _path.clear();
        bool found = false;

        if (GNavigationManager)
            found = GNavigationManager->FindPath(_pos, targetPos, _path);

        if (!found)
        {
            // navgrid.bin 미존재 또는 경로 없음 → 직선 이동 폴백
            _path = { _pos, targetPos };
            found = true;
        }

        _pathIndex        = 1;
        _lastTargetPos    = targetPos;
        _nextFindPathTick = 0;
    }

    if (_pathIndex >= static_cast<int32>(_path.size()))
    {
        _path.clear();
        _pathIndex = 0;
        return;
    }

    // 직선 이동 중에는 마지막 웨이포인트를 실시간 타겟 위치로 갱신
    _path.back() = targetPos;

    Vector3 nextWaypoint = _path[_pathIndex];

    if (_pos.DistanceSq(nextWaypoint) < ARRIVE_DIST_SQ)
    {
        ++_pathIndex;
        if (_pathIndex >= static_cast<int32>(_path.size())) return;
        nextWaypoint = _path[_pathIndex];
    }

    Vector3 dir = (nextWaypoint - _pos).Normalized();

    _pos = _pos + dir * (_speed * _deltaTime);
    _yaw = ::atan2f(dir.x, dir.z) * RAD2DEG;
    if (_yaw < 0.f) _yaw += 360.f;
    _isDirty = true;
}

void Monster::ExecuteAttack(int64 now, PlayerRef target)
{
    uint64 cooldownMs = static_cast<uint64>(1000.f / std::max(GetAttackSpeed(), 0.1f));  // ms
    _nextAttackTick = now + static_cast<int64>(cooldownMs);

    Vector3 toTarget = target->GetPos() - _pos;
    toTarget.y = 0.f;
    if (!toTarget.IsZero())
    {
        toTarget = toTarget.Normalized();
        _yaw = ::atan2f(toTarget.x, toTarget.z) * (180.f / 3.141592f);
        if (_yaw < 0.f) _yaw += 360.f;
        _isDirty = true;
    }

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
    if (_pos.DistanceSq(target->GetPos()) > GetAttackRangeSq() * 4.0f) return;

    int32 damage = target->TakeDamage(_config->attack);
    if (damage <= 0) return;

    GameScene*   scene = GetGameSceneRaw();
    ZoneRef      zone = GetZone();
    if (!scene || !zone) return;

    Protocol::SChangeHp hpPkt;
    hpPkt.set_object_id(target->GetObjectId());
    hpPkt.set_hp(target->GetHp());
    hpPkt.set_damage(-damage);
    scene->BroadcastToAdjacentZones(zone,
        MakeSendBuffer<Protocol::MsgId::S_CHANGE_HP>(hpPkt));

    if (target->IsDead())
    {
        Protocol::SDie diePkt;
        diePkt.set_object_id(target->GetObjectId());
        scene->BroadcastToAdjacentZones(zone,
            MakeSendBuffer<Protocol::MsgId::S_DIE>(diePkt));
    }
}
#include "pch.h"
#include "Player.h"
#include "GameSession.h"
#include "DataManager.h"
#include "GameScene.h"
#include "Protocol/PacketUtils.h"
#include "GameUtil.h"
#include "IdGenerator.h"
#include "GameConfig.h"

constexpr float ATTACK_POS_TOLERANCE_MULT = 4.f;

bool Player::Init(const PlayerSummaryData& summary, const PlayerLoadData& loadData)
{
    _playerDbId = summary.dbId;
    _objectId = IdGenerator::Generate();
    _objectType = Protocol::GameObjectType::PLAYER;
    _level = summary.level;
    _templateId = summary.templateId;
    SetName(summary.name);

    _config = GDataManager->GetPlayerData(_templateId, _level);
    if (_config == nullptr)
    {
        LOG_ERROR(L"Player::Init missing data templateId=" + std::to_wstring(_templateId)
                  + L" level=" + std::to_wstring(_level));
        return false;
    }

    _maxCombo = GDataManager->GetMaxCombo(_templateId);
    _maxHp = _config->maxHp;
    _speed = _config->speed;

    _pos = loadData.pos;
    _yaw = loadData.yaw;
    _hp = loadData.hp > 0 ? loadData.hp : _maxHp;
    _mp = loadData.mp;
    _exp = loadData.exp;
    _state = Protocol::IDLE;

    _prefabData = GDataManager->GetPrefabData(_templateId);
    return true;
}

void Player::Send(SendBufferRef sendBuffer)
{
    if (auto session = _session.lock())
        session->Send(sendBuffer);
}

void Player::AddToScene(GameScene* scene)
{
    scene->AddPlayer(std::static_pointer_cast<Player>(shared_from_this()));
}

void Player::RemoveFromScene(GameScene* scene)
{
    scene->RemovePlayer(_objectId);
}

void Player::HandleMoveJob(const MoveJob& job, uint64 nowMs)
{
    _velocity = job.velocity;
    _state    = job.state;
    _yaw      = job.yaw;

    // Dead reckoning: sendServerTimeMs is the send time already in the server-clock domain
    // (via clock sync), so (nowMs - sendServerTimeMs) is the full latency (network + queue).
    // Project the reported position forward by it so _pos reflects the current tick.
    // 0 => client not yet synced; idle packets carry zero velocity => snap to exact pos.
    uint64 latencyMs = 0;
    if (job.sendServerTimeMs != 0 && nowMs > job.sendServerTimeMs)
        latencyMs = nowMs - job.sendServerTimeMs;
    if (latencyMs > GameConfig::Move::DEAD_RECKON_MAX_MS)
        latencyMs = GameConfig::Move::DEAD_RECKON_MAX_MS;

    _pos = job.pos + job.velocity * (latencyMs / 1000.f);
}

void Player::SetPendingMove(const MoveJob& job)
{
    LockGuard lock(_moveLock);
    _pendingMove = job;
    _hasPendingMove = true;
}

bool Player::TakePendingMove(MoveJob& out)
{
    LockGuard lock(_moveLock);
    if (!_hasPendingMove) return false;
    out = _pendingMove;
    _hasPendingMove = false;
    return true;
}

void Player::SendMoveCorrection(uint64 nowMs)
{
    // Throttle: a client shoved into a wall would otherwise be corrected every tick.
    if (nowMs - _lastCorrectionMs < GameConfig::Move::CORRECTION_MIN_INTERVAL_MS) return;
    _lastCorrectionMs = nowMs;

    Protocol::SMoveCorrection pkt;
    *pkt.mutable_pos() = GameUtil::ToProto(_pos);   // last accepted authoritative pos
    pkt.set_yaw(_yaw);
    Send(MakeSendBuffer<Protocol::MsgId::S_MOVE_CORRECTION>(pkt));
}

bool Player::IsMoveAllowed(const Vector3& dst, const Vector3& vel, uint64 nowMs)
{
    const float maxSpeed = _speed * GameConfig::Move::SPRINT_MULT * GameConfig::Move::SPEED_TOLERANCE;

    if (vel.LengthSq() > maxSpeed * maxSpeed)
        return false;

    if (_lastMoveTick != 0)
    {
        uint64 dtMs = nowMs - _lastMoveTick;
        if (dtMs > GameConfig::Move::MAX_MOVE_DT_MS)
            dtMs = GameConfig::Move::MAX_MOVE_DT_MS;

        const float allowed = maxSpeed * (dtMs / 1000.0f) + GameConfig::Move::MOVE_DIST_EPS;
        if (_pos.DistanceSq(dst) > allowed * allowed)
            return false;
    }

    _lastMoveTick = nowMs;
    return true;
}

void Player::HandleAttack(float yaw, int32 comboIndex, Vector3 clientPos)
{
    if (IsDead()) return;
    if (comboIndex < 1 || comboIndex > _maxCombo) return;

    float distSq = clientPos.DistanceSq(_pos);
    float attackPosToleranceSq = GameGlobal::GetConfig().gameplay.attackPosToleranceSq;

    if (distSq > attackPosToleranceSq * ATTACK_POS_TOLERANCE_MULT)
    {
        LOG_WARN(L"[AntiCheat] Attack pos too far: objectId={0}, dist={1:.1f}",
            _objectId, sqrtf(distSq));
        if (GServerStats)
            GServerStats->game.suspiciousPackets.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    Vector3 attackPos = (distSq <= attackPosToleranceSq) ? clientPos : _pos;

    _yaw = yaw;
    _pos = attackPos;
    _state = Protocol::ATTACK;
    _velocity = Vector3::Zero();

    GameScene* scene = GetGameSceneRaw();
    ZoneRef      zone = GetZone();
    if (!scene || !zone) return;

    Protocol::SAttack packet;
    packet.set_object_id(_objectId);
    packet.set_yaw(yaw);
    packet.set_combo_index(comboIndex);
    *packet.mutable_pos() = GameUtil::ToProto(attackPos);
    scene->BroadcastToAdjacentZones(zone,
        MakeSendBuffer<Protocol::MsgId::S_ATTACK>(packet));

    uint64 hitDelay = GetHitDelay(comboIndex);
    auto   self = std::static_pointer_cast<Player>(shared_from_this());

    scene->DoTimer(hitDelay, [scene, self, attackPos, yaw]()
        {
            if (self->IsDead()) return;
            scene->HandleAttackHitDetection(self, attackPos, yaw);
        });
}

void Player::GainExp(int64 rewardExp)
{
    if (IsDead() || rewardExp <= 0) return;

    _exp += rewardExp;

    Protocol::SChangeExp pkt;
    pkt.set_object_id(_objectId);
    pkt.set_exp(_exp);
    Send(MakeSendBuffer<Protocol::MsgId::S_CHANGE_EXP>(pkt));

    TryLevelUp();
}

void Player::Revive(Vector3 pos)
{
    _hp = _config ? _config->maxHp : 1;
    _state = Protocol::IDLE;
    _pos = pos;
    _yaw = 0.f;
    _velocity = Vector3::Zero();
}

void Player::MakeStatInfo(Protocol::StatInfo& info) const
{
    info.set_hp(_hp);
    info.set_mp(_mp);
    info.set_exp(_exp);
}

void Player::TryLevelUp()
{
    while (true)
    {
        int64 reqExp = GDataManager->GetPlayerRequireExp(_templateId, _level + 1);
        if (_exp < reqExp) return;

        const PlayerData* nextConfig = GDataManager->GetPlayerData(_templateId, _level + 1);
        if (nextConfig == nullptr) return;

        _exp -= reqExp;
        _level += 1;
        _config = nextConfig;
        _speed = _config->speed;
        _maxHp = _config->maxHp;
        _hp = _config->maxHp;
        _mp = _config->maxMp;

        GameScene* scene = GetGameSceneRaw();
        ZoneRef      zone = GetZone();
        if (scene && zone)
        {
            Protocol::SChangeLevel broadcastPkt;
            broadcastPkt.set_object_id(_objectId);
            broadcastPkt.set_level(_level);
            broadcastPkt.set_max_hp(_config->maxHp);
            broadcastPkt.set_hp(_hp);
            scene->BroadcastToAdjacentZones(zone,
                MakeSendBuffer<Protocol::MsgId::S_CHANGE_LEVEL>(broadcastPkt));
        }

        Protocol::SChangeLevel lvPkt;
        lvPkt.set_object_id(_objectId);
        lvPkt.set_level(_level);
        lvPkt.set_max_hp(_config->maxHp);
        lvPkt.set_hp(_hp);
        lvPkt.set_exp(_exp);
        Send(MakeSendBuffer<Protocol::MsgId::S_CHANGE_LEVEL>(lvPkt));

        _isSaveDirty = true;
    }
}

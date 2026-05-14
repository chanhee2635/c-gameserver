#include "pch.h"
#include "Player.h"
#include "GameSession.h"
#include "DataManager.h"
#include "GameScene.h"
#include "Protocol/PacketUtils.h"
#include "GameUtil.h"
#include "IdGenerator.h"

void Player::Init(const PlayerSummaryData& summary, const PlayerLoadData& loadData)
{
    _playerDbId = summary.dbId;
    _objectId = IdGenerator::Generate();
    _objectType = Protocol::PLAYER;
    _level = summary.level;
    _templateId = summary.templateId;
    SetName(summary.name);

    _config = GDataManager->GetPlayerData(_templateId, _level);
    ASSERT_CRASH(_config != nullptr);

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
}

void Player::Send(SendBufferRef sendBuffer)
{
    if (auto session = _session.lock())
        session->Send(sendBuffer);
}

void Player::HandleMoveJob(const MoveJob& job)
{
    Vector3                  oldPos   = _pos;
    Protocol::CreatureState  oldState = _state;

    _pos = job.pos;
    _state = job.state;
    _yaw = job.yaw;

    bool posChanged = oldPos.DistanceSq(_pos) > 0.0001f;
    bool stateChanged = oldState != _state;

    if (posChanged || stateChanged)
        _isDirty = true;
}

void Player::HandleAttack(float yaw, int32 comboIndex, Vector3 clientPos)
{
    if (IsDead()) return;
    if (comboIndex < 1 || comboIndex > _maxCombo) return;

    float distSq = clientPos.DistanceSq(_pos);
    float attackPosToleranceSq = GameGlobal::GetConfig().gameplay.attackPosToleranceSq;

    if (distSq > attackPosToleranceSq * 4.f)
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
    _isDirty = true;

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
    _isDirty = true;
}

void Player::MakeStatInfo(Protocol::StatInfo& info) const
{
    info.set_hp(_hp);
    info.set_mp(_mp);
    info.set_exp(_exp);
}

void Player::OnDead()
{
    LOG_INFO(L"Player dead playerDbId=" + std::to_wstring(_playerDbId));
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
        _isDirty = true;

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

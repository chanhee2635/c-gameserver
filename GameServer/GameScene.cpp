#include "pch.h"
#include "GameScene.h"
#include "Player.h"
#include "Monster.h"
#include "Zone.h"
#include "World.h"
#include "DataManager.h"
#include "GameUtil.h"
#include "GridMap.h"
#include "Protocol/PacketUtils.h"
#include "DBManager.h"
#include <chrono>

// 브로드캐스트 전송을 분산할 send-lane 풀. zone 별로 lane 고정(zoneId % N)해 순서 보존
namespace
{
    constexpr int32 SEND_LANE_COUNT = 8;

    Vector<JobQueueRef>& SendLanes()
    {
        static Vector<JobQueueRef> s_lanes = []()
        {
            Vector<JobQueueRef> lanes;
            lanes.reserve(SEND_LANE_COUNT);
            for (int32 i = 0; i < SEND_LANE_COUNT; ++i)
                lanes.push_back(MakeShared<JobQueue>());
            return lanes;
        }();
        return s_lanes;
    }
}

void GameScene::AddZone(ZoneRef zone)
{
    _zones.push_back(zone);
}

void GameScene::AddPlayer(PlayerRef player)
{
    _players[player->GetObjectId()] = player;
}

void GameScene::RemovePlayer(uint64 objectId)
{
    _players.erase(objectId);
}

void GameScene::AddMonster(MonsterRef monster)
{
    _monsters[monster->GetObjectId()] = monster;
}

void GameScene::RemoveMonster(uint64 objectId)
{
    _monsters.erase(objectId);
}

void GameScene::Update()
{
    uint64 now = ::GetTickCount64();
    uint32 updateTickMs = GameGlobal::GetConfig().gameplay.updateTickMs;
    if (_lastUpdateTick > 0 && GServerStats)
    {
        uint64 expectedThisTick = _lastUpdateTick + updateTickMs;
        if (now > expectedThisTick)
        {
            uint64 lagMs = now - expectedThisTick;
            GServerStats->game.tickLagTotalMs.fetch_add(lagMs, std::memory_order_relaxed);
            GServerStats->game.tickLagCount.fetch_add(1, std::memory_order_relaxed);

            uint64 prevMax = GServerStats->game.tickLagMaxMs.load(std::memory_order_relaxed);
            while (lagMs > prevMax &&
                !GServerStats->game.tickLagMaxMs.compare_exchange_weak(prevMax, lagMs, std::memory_order_relaxed));
        }
    }

    if (GServerStats) GServerStats->game.tickCount.fetch_add(1, std::memory_order_relaxed);

    uint32 deltaTimeMs = (_lastUpdateTick > 0) ? now - _lastUpdateTick : updateTickMs;
    _lastUpdateTick = now;

    DoTimer(GameGlobal::GetConfig().gameplay.updateTickMs, &GameScene::Update);

    FrameVector<CreatureRef> movingCreatures;
    FrameHashMap<GameScene*, Vector<MoveNotice>> crossNotices;

    using PhaseClock = std::chrono::steady_clock;
    auto p0 = PhaseClock::now();
    UpdateObjects(now, deltaTimeMs, movingCreatures);
    auto p1 = PhaseClock::now();
    CollectMoveNotices(movingCreatures, crossNotices);
    DispatchNotices(crossNotices);
    auto p2 = PhaseClock::now();
    BroadcastScene();
    auto p3 = PhaseClock::now();

    if (GServerStats)
    {
        auto us = [](auto a, auto b) {
            return (uint64)std::chrono::duration_cast<std::chrono::microseconds>(b - a).count();
        };
        GServerStats->game.tickPhaseUpdateUs.fetch_add(us(p0, p1), std::memory_order_relaxed);
        GServerStats->game.tickPhaseNoticeUs.fetch_add(us(p1, p2), std::memory_order_relaxed);
        GServerStats->game.tickPhaseBroadcastUs.fetch_add(us(p2, p3), std::memory_order_relaxed);
    }
}

void GameScene::UpdateObjects(uint64 nowMs, uint32 deltaTimeMs, FrameVector<CreatureRef>& movingCreatures)
{
    for (auto& [id, player] : _players)
    {
        MoveJob job;
        if (player->TakePendingMove(job))
        {
            bool blocked = (GGridMap && GGridMap->IsLoaded() && !GGridMap->IsWalkable(job.pos));
            bool allowed = player->IsMoveAllowed(job.pos, job.velocity, nowMs);

            if (!blocked && allowed)
            {
                player->HandleMoveJob(job);
            }
            else if (!allowed && GServerStats)
            {
                GServerStats->game.suspiciousPackets.fetch_add(1, std::memory_order_relaxed);
            }
            // Rejected moves are ignored so the server stays authoritative (no teleport/speedhack).
            // TODO: send a position-correction packet so client/server positions cannot drift.
        }

        if (player->ShouldBroadcastMove(nowMs))
            CollectMovingCreature(player, nowMs, movingCreatures);

        if (player->IsSaveDirty())
        {
            player->ClearSaveDirty();

            uint64  dbId = player->GetPlayerDbId();
            int32   level = player->GetLevel();
            int32   hp = player->GetHp();
            int32   mp = player->GetMp();
            int64   exp = player->GetExp();
            Vector3 pos = player->GetPos();
            float   yaw = player->GetYaw();

            GDBManager->DoAsync([dbId, level, hp, mp, exp, pos, yaw]()
            {
                GDBManager->SavePlayerLevelUp(dbId, level, hp, mp, exp, pos, yaw);
            });
        }
    }

    for (auto& [id, monster] : _monsters)
    {
        if (monster->IsDead()) continue;
        monster->Update(deltaTimeMs);

        if (monster->ShouldBroadcastMove(nowMs))
            CollectMovingCreature(monster, nowMs, movingCreatures);
    }
}

void GameScene::CollectMovingCreature(const CreatureRef& creature, uint64 nowMs, FrameVector<CreatureRef>& movingCreatures)
{
    movingCreatures.push_back(creature);
    creature->CommitMoveAnchor(nowMs);
}

void GameScene::CollectMoveNotices(FrameVector<CreatureRef>& movingCreatures, FrameHashMap<GameScene*, Vector<MoveNotice>>& crossNotices)
{
    for (CreatureRef& creature : movingCreatures)
    {
        ZoneRef oldZone = creature->GetZone();
        if (!oldZone) continue;   

        ZoneRef newZone = GWorld->GetZoneByPos(creature->GetPos());
        if (!newZone) continue;

        if (oldZone == newZone)
        {
            for (const ZoneRef& zone : oldZone->GetAdjacentZones())
            {
                if (!zone->IsActive()) continue;
                EmitVision(creature, zone, VisionType::Move, crossNotices);
            }
        }
        else
        {
            HandleZoneChange(creature, oldZone, newZone, crossNotices);
        }
    }
}

void GameScene::HandleZoneChange(CreatureRef creature, ZoneRef oldZone, ZoneRef newZone,
    FrameHashMap<GameScene*, Vector<MoveNotice>>& crossNotices)
{
    GameSceneRef oldScene = oldZone->GetScene();
    GameSceneRef newScene = newZone->GetScene();
    bool sceneChanged = (oldScene != newScene);

    oldZone->Leave(creature);
    creature->SetZone(newZone);

    if (sceneChanged)
    {
        creature->RemoveFromScene(this);

        newScene->DoAsync(MakeJob([newScene, newZone, creature]()
        {
            newZone->Enter(creature);
            creature->SetGameScene(newScene);
            creature->AddToScene(newScene.get());
        }));
    }
    else
    {
        newZone->Enter(creature);
    }

    MoveResultRef moveResult = GWorld->GetMoveResult(oldZone, newZone);
    if (!moveResult) return;

    for (const ZoneRef& zone : moveResult->enterZones)
        EmitVision(creature, zone, VisionType::Spawn, crossNotices);
    for (const ZoneRef& zone : moveResult->leaveZones)
        EmitVision(creature, zone, VisionType::Despawn, crossNotices);
    for (const ZoneRef& zone : moveResult->keepZones)
        EmitVision(creature, zone, VisionType::Move, crossNotices);
}

void GameScene::EmitVision(const CreatureRef& creature, const ZoneRef& zone, VisionType type,
    FrameHashMap<GameScene*, Vector<MoveNotice>>& crossNotices)
{
    GameScene* scenePtr = zone->GetSceneRaw();
    if (scenePtr == this)
        ApplyVision(creature, zone, type);                    
    else if (scenePtr)
        crossNotices[scenePtr].push_back({ creature, zone, type }); 
}

void GameScene::ApplyVision(const CreatureRef& creature, const ZoneRef& zone, VisionType type)
{
    switch (type)
    {
    case VisionType::Spawn:
        zone->AddPendingSpawn(creature);
        if (creature->GetObjectType() == Protocol::GameObjectType::PLAYER)
            World::SendSpawnPacketsToPlayer(std::static_pointer_cast<Player>(creature), { zone });
        break;
    case VisionType::Despawn:
        zone->AddPendingDespawn(creature->GetObjectId());
        if (creature->GetObjectType() == Protocol::GameObjectType::PLAYER)
            World::SendDespawnPacketsToPlayer(std::static_pointer_cast<Player>(creature), { zone });
        break;
    case VisionType::Move:
        zone->AddPendingMove(creature);
        break;
    }
}

void GameScene::DispatchNotices(FrameHashMap<GameScene*, Vector<MoveNotice>>& crossNotices)
{
    for (auto& [scenePtr, notices] : crossNotices)
    {
        if (!scenePtr || notices.empty()) continue;

        GameSceneRef targetScene = notices[0].targetZone->GetScene();
        RunOnScene(targetScene, [targetScene, n = std::move(notices)]() mutable
        {
            targetScene->ProcessNotices(n);
        });
    }
}

void GameScene::ProcessNotices(const Vector<MoveNotice>& notices)
{
    for (const MoveNotice& notice : notices)
        ApplyVision(notice.mover, notice.targetZone, notice.type);
}

void GameScene::BroadcastScene()
{
    for (ZoneRef& zone : _zones)
    {
        if (zone->IsEmpty()) continue;     

        if (zone->IsActive())          
        {
            _broadcastPacket.Clear();

            auto flush = [&]()
            {
                if (_broadcastPacket.spawns_size()   == 0 &&
                    _broadcastPacket.despawns_size() == 0 &&
                    _broadcastPacket.moves_size()    == 0) return;
                OffloadBroadcast(zone, MakeSendBuffer<Protocol::MsgId::S_UPDATE_SCENE>(_broadcastPacket));
                _broadcastPacket.Clear();
            };

            zone->FillUpdatePacket(_broadcastPacket, flush);
        }

        zone->ClearPending();   
    }
}

void GameScene::BroadcastToZone(ZoneRef zone, SendBufferRef sendBuffer, uint64 exceptId)
{
    if (!zone || !sendBuffer) return;

    for (auto& [id, player] : zone->GetPlayers())
    {
        if (id == exceptId) continue;
        player->Send(sendBuffer);
    }
}

// zone 플레이어를 스냅샷한 뒤 실제 전송은 send-lane으로 넘겨 워커가 병렬 처리한다
void GameScene::OffloadBroadcast(const ZoneRef& zone, SendBufferRef sendBuffer)
{
    if (!sendBuffer) return;

    const auto& players = zone->GetPlayers();
    if (players.empty()) return;

    auto targets = MakeShared<Vector<PlayerRef>>();
    targets->reserve(players.size());
    for (auto& [id, player] : players)
        targets->push_back(player);

    JobQueueRef lane = SendLanes()[zone->GetId() % SEND_LANE_COUNT];
    lane->DoAsync([targets = std::move(targets), sendBuffer = std::move(sendBuffer)]()
    {
        for (PlayerRef& p : *targets)
            p->Send(sendBuffer);
    });
}

void GameScene::BroadcastToAdjacentZones(ZoneRef zone, SendBufferRef sendBuffer, uint64 exceptId)
{
    if (!zone || !sendBuffer) return;

    FrameHashMap<GameScene*, FrameVector<ZoneRef>> adjacentSceneGroups;

    for (const ZoneRef& adjZone : zone->GetAdjacentZones())
    {
        GameSceneRef scene = adjZone->GetScene();
        if (scene)
            adjacentSceneGroups[scene.get()].push_back(adjZone);
    }

    for (auto& [scenePtr, zones] : adjacentSceneGroups)
    {
        if (scenePtr == this)
        {
            for (const ZoneRef& z : zones)
                BroadcastToZone(z, sendBuffer, exceptId);
        }
        else
        {
            GameSceneRef targetScene = zones[0]->GetScene();
            if (!targetScene) continue;
            targetScene->DoAsync(MakeJob([targetScene, zones = Vector<ZoneRef>(zones.begin(), zones.end()), sendBuffer, exceptId]() {
                for (const ZoneRef& z : zones)
                    targetScene->BroadcastToZone(z, sendBuffer, exceptId);
            }));
        }
    }
}

void GameScene::BroadcastToAllPlayers(SendBufferRef buffer)
{
    for (auto& [id, player] : _players)
        player->Send(buffer);
}

void GameScene::BroadcastHpChange(const ZoneRef& zone, uint64 objectId, int32 hp, int32 damage)
{
    if (!zone) return;

    Protocol::SChangeHp pkt;
    pkt.set_object_id(objectId);
    pkt.set_hp(hp);
    pkt.set_damage(damage);
    BroadcastToAdjacentZones(zone, MakeSendBuffer<Protocol::MsgId::S_CHANGE_HP>(pkt));
}

void GameScene::BroadcastDie(const ZoneRef& zone, uint64 objectId)
{
    if (!zone) return;

    Protocol::SDie pkt;
    pkt.set_object_id(objectId);
    BroadcastToAdjacentZones(zone, MakeSendBuffer<Protocol::MsgId::S_DIE>(pkt));
}

void GameScene::FindNearestPlayer(Vector<ZoneRef> zones, MonsterRef monster, Vector3 monsterPos)
{
    if (!monster) return;

    PlayerRef bestPlayer = nullptr;
    float     minDistSq  = monster->GetSearchRangeSq();

    for (const ZoneRef& zone : zones)
    {
        if (!zone->IsActive()) continue;
        for (auto& [id, player] : zone->GetPlayers())
        {
            if (player->IsDead()) continue;

            float distSq = (player->GetPos() - monsterPos).LengthSq();
            if (distSq < minDistSq)
            {
                minDistSq  = distSq;
                bestPlayer = player;
            }
        }
    }

    if (!bestPlayer) return;

    GameSceneRef monsterScene = monster->GetGameScene();
    RunOnScene(monsterScene, [monster, bestPlayer, minDistSq]()
    {
        monster->HandleGatherResult(bestPlayer, minDistSq);
    });
}

void GameScene::HandleAttackHitDetection(PlayerRef attacker, Vector3 attackPos, float yaw)
{
    ZoneRef myZone = attacker->GetZone();
    if (!myZone) return;

    float attackRangeSq = attacker->GetAttackRange() * attacker->GetAttackRange();
    float halfAngleRad  = attacker->GetAttackAngle() * 0.5f * DEG2RAD;
    float cosHalfAngle  = cosf(halfAngleRad);

    float   yawRad  = yaw * DEG2RAD;
    Vector3 forward = { sinf(yawRad), 0.f, cosf(yawRad) };

    for (const ZoneRef& zone : myZone->GetAdjacentZones())
    {
        for (auto& [monsterId, monster] : zone->GetMonsters())
        {
            if (monster->IsDead()) continue;

            Vector3 toMonster = monster->GetPos() - attackPos;
            toMonster.y = 0.f;

            float distSq = toMonster.LengthSq();
            if (distSq > attackRangeSq) continue;

            if (distSq > 0.001f)
            {
                if (forward.Dot(toMonster.Normalized()) < cosHalfAngle) continue;
            }

            ApplyHitToMonster(attacker, monster, attackPos, myZone);
        }
    }
}

void GameScene::ApplyHitToMonster(PlayerRef attacker, MonsterRef monster,
                                  Vector3 attackPos, ZoneRef myZone)
{
    int32 actualDamage = monster->TakeDamage(attacker->GetAttack());
    if (actualDamage == 0)
        return;

    BroadcastHpChange(myZone, monster->GetObjectId(), monster->GetHp(), -actualDamage);

    if (monster->IsDead())
    {
        attacker->GainExp(monster->GetRewardExp());
        HandleMonsterDead(monster);
    }
}

void GameScene::HandleMonsterDead(MonsterRef monster)
{
    ZoneRef zone = monster->GetZone();
    if (!zone) return;

    BroadcastDie(zone, monster->GetObjectId());

    int32 deadAnimMs = GDataManager->GetDeathDurationMs(monster->GetTemplateId());
    DoTimer(deadAnimMs, [monster]()
        {
            GWorld->DoAsync([monster]() {
                GWorld->LeaveCreature(monster);
                });
            GWorld->DoTimer(monster->GetRespawnDelayMs(), [monster]() {
                monster->Reset();
                GWorld->DoAsync([monster]() {
                    GWorld->EnterCreature(monster);
                    });
                });
        });
}

void GameScene::HandleRevive(PlayerRef player, bool isCurrentPos)
{
    if (!player || !player->IsDead()) return;

    Vector3 revivePos;
    if (isCurrentPos)
    {
        revivePos = player->GetPos();
    }
    else
    {
        const SpawnData* nearest = GDataManager->GetNearestPlayerSpawn(player->GetPos());
        if (!nearest) return;
        revivePos = nearest->pos;
    }

    player->Revive(revivePos);

    Protocol::SRevive pkt;
    *pkt.mutable_pos() = GameUtil::ToProto(revivePos);
    pkt.set_hp(player->GetHp());
    pkt.set_max_hp(player->GetMaxHp());
    player->Send(MakeSendBuffer<Protocol::MsgId::S_REVIVE>(pkt));

    GWorld->DoAsync([player]()  { GWorld->LeaveCreature(player); });
    GWorld->DoTimer(1, [player]() { GWorld->EnterCreature(player); });
}

#include "pch.h"
#include "GameScene.h"
#include "Player.h"
#include "Monster.h"
#include "Zone.h"
#include "World.h"
#include "DataManager.h"
#include "GameUtil.h"
#include "Protocol/PacketUtils.h"
#include "DBManager.h"

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

void GameScene::PushMoveJob(MoveJob job)
{
    LockGuard guard(_moveLock);
    _pendingMoveJobs.push_back(job);
}

void GameScene::Update()
{
    uint64 now = ::GetTickCount64();
    uint32 updateTickMs = GameGlobal::GetConfig().gameplay.updateTickMs;
    if (_lastUpdateTick > 0)
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

    FrameVector<MoveJob>    jobsCache;
    FrameVector<CreatureRef> movingCreatures;
    FrameVector<CreatureRef> movingCreaturesSnapshot;
    FrameHashMap<GameScene*, Vector<MoveNotice>> sceneNotices;

    UpdateObjects(deltaTimeMs, jobsCache, movingCreatures);
    CollectMoveNotices(movingCreatures, movingCreaturesSnapshot, sceneNotices);
    DispatchNotices(sceneNotices);
    BroadcastScene(deltaTimeMs);

    DoTimer(GameGlobal::GetConfig().gameplay.updateTickMs, &GameScene::Update);
}

void GameScene::UpdateObjects(uint32 deltaTimeMs, FrameVector<MoveJob>& jobsCache, FrameVector<CreatureRef>& movingCreatures)
{
    {
        LockGuard guard(_moveLock);
        jobsCache.reserve(_pendingMoveJobs.size());
        for (auto& job : _pendingMoveJobs)
            jobsCache.push_back(std::move(job));
        _pendingMoveJobs.clear();
    }

    for (MoveJob& job : jobsCache)
    {
        auto it = _players.find(job.objectId);
        if (it == _players.end()) continue;

        PlayerRef& player = it->second;
        player->HandleMoveJob(job);
        if (player->IsDirty())
            CollectMovingCreature(player, movingCreatures);
    }

    for (auto& [id, monster] : _monsters)
    {
        if (monster->IsDead()) continue;
        monster->Update(deltaTimeMs);

        if (monster->IsDirty())
            CollectMovingCreature(monster, movingCreatures);
    }

    for (auto& [id, player] : _players)
    {
        if (!player->IsSaveDirty()) continue;
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

void GameScene::CollectMovingCreature(const CreatureRef& creature, FrameVector<CreatureRef>& movingCreatures)
{
    movingCreatures.push_back(creature);
    creature->SetDirty(false);
}

void GameScene::CollectMoveNotices(FrameVector<CreatureRef>& movingCreatures, FrameVector<CreatureRef>& snapshot, FrameHashMap<GameScene*, Vector<MoveNotice>>& sceneNotices)
{
    snapshot.swap(movingCreatures);

    for (CreatureRef& creature : snapshot)
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
                GameScene* scenePtr = zone->GetSceneRaw();
                if (scenePtr)
                    sceneNotices[scenePtr].push_back({ creature, zone, VisionType::Move });
            }
        }
        else
        {
            HandleZoneChange(creature, oldZone, newZone, sceneNotices);
        }
    }
}

void GameScene::HandleZoneChange(CreatureRef creature, ZoneRef oldZone, ZoneRef newZone,
    FrameHashMap<GameScene*, Vector<MoveNotice>>& sceneNotices)
{
    GameSceneRef oldScene = oldZone->GetScene();
    GameSceneRef newScene = newZone->GetScene();
    bool sceneChanged = (oldScene != newScene);

    oldZone->Leave(creature);
    creature->SetZone(newZone);

    if (sceneChanged)
    {
        if (creature->GetObjectType() == Protocol::GameObjectType::PLAYER)
            RemovePlayer(creature->GetObjectId());
        else if (creature->GetObjectType() == Protocol::GameObjectType::MONSTER)
            RemoveMonster(creature->GetObjectId());

        newScene->DoAsync(MakeJob([newScene, newZone, creature]()
        {
            newZone->Enter(creature);
            creature->SetGameScene(newScene);

            if (creature->GetObjectType() == Protocol::GameObjectType::PLAYER)
                newScene->AddPlayer(std::static_pointer_cast<Player>(creature));
            else if (creature->GetObjectType() == Protocol::GameObjectType::MONSTER)
                newScene->AddMonster(std::static_pointer_cast<Monster>(creature));
        }));
    }
    else
    {
        newZone->Enter(creature);
    }

    MoveResultRef moveResult = GWorld->GetMoveResult(oldZone, newZone);
    if (!moveResult) return;

    for (const ZoneRef& zone : moveResult->enterZones)
    {
        GameScene* scenePtr = zone->GetScene().get();
        if (scenePtr) sceneNotices[scenePtr].push_back({ creature, zone, VisionType::Spawn });
    }
    for (const ZoneRef& zone : moveResult->leaveZones)
    {
        GameScene* scenePtr = zone->GetScene().get();
        if (scenePtr) sceneNotices[scenePtr].push_back({ creature, zone, VisionType::Despawn });
    }
    for (const ZoneRef& zone : moveResult->keepZones)
    {
        GameScene* scenePtr = zone->GetScene().get();
        if (scenePtr) sceneNotices[scenePtr].push_back({ creature, zone, VisionType::Move });
    }
}

void GameScene::DispatchNotices(FrameHashMap<GameScene*, Vector<MoveNotice>>& sceneNotices)
{
    for (auto& [scenePtr, notices] : sceneNotices)
    {
        if (!scenePtr || notices.empty()) continue;

        if (scenePtr == this)
        {
            ProcessNotices(notices);
        }
        else
        {
            GameSceneRef targetScene = notices[0].targetZone->GetScene();
            if (!targetScene) continue;

            targetScene->DoAsync(MakeJob([targetScene, n = std::move(notices)]() mutable {
                targetScene->ProcessNotices(n);
            }));
        }
    }
}

void GameScene::ProcessNotices(const Vector<MoveNotice>& notices)
{
    for (const MoveNotice& notice : notices)
    {
        ZoneRef zone         = notice.targetZone;
        CreatureRef creature = notice.mover;

        switch (notice.type)
        {
        case VisionType::Spawn:
            zone->AddPendingSpawn(creature);
            if (creature->GetObjectType() == Protocol::GameObjectType::PLAYER)
            {
                PlayerRef player = std::static_pointer_cast<Player>(creature);
                World::SendSpawnPacketsToPlayer(player, { zone });
            }
            break;

        case VisionType::Despawn:
            zone->AddPendingDespawn(creature->GetObjectId());
            if (creature->GetObjectType() == Protocol::GameObjectType::PLAYER)
            {
                PlayerRef player = std::static_pointer_cast<Player>(creature);
                World::SendDespawnPacketsToPlayer(player, { zone });
            }
            break;

        case VisionType::Move:
            zone->AddPendingMove(creature);
            break;
        }
    }
}

void GameScene::BroadcastScene(uint32 deltaTimeMs)
{
    for (ZoneRef& zone : _zones)
    {
        if (!zone->IsActive()) continue;
        if (zone->IsEmpty())   continue;

        _broadcastPacket.Clear();

        _broadcastPacket.set_deltatimems(deltaTimeMs);

        auto flush = [&]()
        {
            if (_broadcastPacket.spawns_size()   == 0 &&
                _broadcastPacket.despawns_size() == 0 &&
                _broadcastPacket.moves_size()    == 0) return;

            BroadcastToZone(zone, MakeSendBuffer<Protocol::MsgId::S_UPDATE_SCENE>(_broadcastPacket));
            _broadcastPacket.Clear();
        };

        zone->FillUpdatePacket(_broadcastPacket, flush);
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
    if (!monsterScene) return;

    if (monsterScene.get() == this)
        monster->HandleGatherResult(bestPlayer, minDistSq);
    else
    {
        monsterScene->DoAsync(MakeJob([monster, bestPlayer, minDistSq]() {
            monster->HandleGatherResult(bestPlayer, minDistSq);
        }));
    }
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

    Protocol::SChangeHp hpPkt;
    hpPkt.set_object_id(monster->GetObjectId());
    hpPkt.set_hp(monster->GetHp());
    hpPkt.set_damage(-actualDamage);
    BroadcastToAdjacentZones(myZone, MakeSendBuffer<Protocol::MsgId::S_CHANGE_HP>(hpPkt));

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

    Protocol::SDie diePkt;
    diePkt.set_object_id(monster->GetObjectId());
    BroadcastToAdjacentZones(zone,
        MakeSendBuffer<Protocol::MsgId::S_DIE>(diePkt));

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

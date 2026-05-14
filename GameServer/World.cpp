#include "pch.h"
#include "World.h"
#include "GameSession.h"
#include "GameScene.h"
#include "Zone.h"
#include "Player.h"
#include "Monster.h"
#include "DataManager.h"
#include "Protocol/PacketUtils.h"

void World::Init(const WorldConfig& config)
{
    _config = config;
    CreateScene();
    CreateZone();
    CreateMoveTable();
}

void World::Start()
{
    SpawnMonsters();
    for (auto& scene : _scenes)
        scene->DoAsync(&GameScene::Update);
}

void World::CreateScene()
{
    int32 count = _config.sceneCount;
    _scenes.resize(count);
    for (int32 i = 0; i < count; ++i)
        _scenes[i] = MakeShared<GameScene>();
}

void World::CreateZone()
{
    int32 zoneCount      = _config.mapSize / _config.zoneSize;
    int32 totalZoneCount = zoneCount * zoneCount;
    _zones.reserve(totalZoneCount);

    for (int32 z = 0; z < zoneCount; ++z)
    {
        for (int32 x = 0; x < zoneCount; ++x)
        {
            int32   linearIdx = z * zoneCount + x;
            ZoneRef zone = MakeShared<Zone>(x, z, _config.zoneSize);
            zone->SetId(linearIdx);
            _zones.push_back(zone);

            int32 sceneIdx = 0;
            if (_config.sceneCount > 1)
            {
                int32 zonesPerScene = totalZoneCount / _config.sceneCount;
                sceneIdx = std::min(linearIdx / zonesPerScene, _config.sceneCount - 1);
            }

            _scenes[sceneIdx]->AddZone(zone);
            zone->SetScene(_scenes[sceneIdx]);
        }
    }

    for (ZoneRef& zone : _zones)
    {
        Vector<ZoneRef> adjacent = GetAdjacentZones(zone->GetPos());
        std::sort(adjacent.begin(), adjacent.end(),
            [](const ZoneRef& a, const ZoneRef& b) { return a->GetId() < b->GetId(); });
        zone->SetAdjacentZones(std::move(adjacent));
    }
}

void World::CreateMoveTable()
{
    for (const ZoneRef& oldZone : _zones)
    {
        int32 oldId = oldZone->GetId();
        for (const ZoneRef& newZone : oldZone->GetAdjacentZones())
        {
            int32 newId = newZone->GetId();
            if (oldId == newId) continue;
            _moveTable[oldId][newId] = CalculateMoveResult(oldZone, newZone);
        }
    }
}

void World::PlayerEnterToGame(GameSessionRef session,
    PlayerSummaryData summary, PlayerLoadData loadData)
{
    if (!session) return;

    PlayerRef player = MakeShared<Player>();
    player->Init(summary, loadData);
    player->SetSession(session);
    session->SetPlayer(player);

    EnterCreature(player);

    Protocol::SEnterGame enterPacket;
    enterPacket.set_success(true);
    player->MakeObjectInfo(*enterPacket.mutable_my_player());
    session->Send(MakeSendBuffer<Protocol::MsgId::S_ENTER_GAME>(enterPacket));

    Protocol::SReadyToEnter readyPkt;
    player->Send(MakeSendBuffer<Protocol::MsgId::S_READY_TO_ENTER>(readyPkt));
}

void World::EnterCreature(CreatureRef creature)
{
    if (!creature) return;

    ZoneRef mainZone = GetZoneByPos(creature->GetPos());
    if (!mainZone) return;
    GameSceneRef mainScene = mainZone->GetScene();
    if (!mainScene) return;

    creature->SetZone(mainZone);
    creature->SetGameScene(mainScene);

    if (GServerStats)
    {
        if (creature->GetObjectType() == Protocol::PLAYER)
        {
            int32 cur  = GServerStats->game.activePlayers.fetch_add(1, std::memory_order_relaxed) + 1;
            int32 peak = GServerStats->game.peakPlayers.load(std::memory_order_relaxed);
            while (cur > peak &&
                   !GServerStats->game.peakPlayers.compare_exchange_weak(
                       peak, cur, std::memory_order_relaxed))
            {}
        }
        else if (creature->GetObjectType() == Protocol::MONSTER)
            GServerStats->game.activeMonsters.fetch_add(1, std::memory_order_relaxed);
    }

    const Vector<ZoneRef>& adjacentZones = mainZone->GetAdjacentZones();

    _enterSceneGroups.clear();
    for (const ZoneRef& zone : adjacentZones)
        _enterSceneGroups[zone->GetSceneRaw()].push_back(zone);

    for (auto& [scene, zones] : _enterSceneGroups)
    {
        scene->DoAsync(MakeJob([scene, zones, creature, mainZone]()
        {
            for (const ZoneRef& zone : zones)
            {
                if (zone == mainZone)
                    zone->Enter(creature);       
                zone->AddPendingSpawn(creature);  
            }

            if (creature->GetObjectType() == Protocol::PLAYER)
            {
                PlayerRef player = std::static_pointer_cast<Player>(creature);
                scene->AddPlayer(player);
                World::SendSpawnPacketsToPlayer(player, zones);
            }
            else if (creature->GetObjectType() == Protocol::MONSTER)
            {
                scene->AddMonster(std::static_pointer_cast<Monster>(creature));
            }
        }));
    }
}

void World::LeaveCreature(CreatureRef creature)
{
    if (!creature) return;

    ZoneRef mainZone = creature->GetZone();
    if (!mainZone) return;

    creature->SetZone(nullptr);
    creature->SetGameScene(nullptr);

    if (GServerStats)
    {
        if (creature->GetObjectType() == Protocol::PLAYER)
            GServerStats->game.activePlayers.fetch_sub(1, std::memory_order_relaxed);
        else if (creature->GetObjectType() == Protocol::MONSTER)
            GServerStats->game.activeMonsters.fetch_sub(1, std::memory_order_relaxed);
    }

    const Vector<ZoneRef>& adjacentZones = mainZone->GetAdjacentZones();
    uint64 objectId = creature->GetObjectId();

    _leaveSceneGroups.clear();
    for (const ZoneRef& zone : adjacentZones)
    {
        GameSceneRef zoneScene = zone->GetScene();
        if (zoneScene)
            _leaveSceneGroups[zone->GetSceneRaw()].push_back(zone);
    }

    for (auto& [scene, zones] : _leaveSceneGroups)
    {
        scene->DoAsync(MakeJob([scene, zones, creature, mainZone, objectId]()
        {
            for (const ZoneRef& zone : zones)
            {
                if (zone == mainZone)
                    zone->Leave(creature);            
                zone->AddPendingDespawn(objectId);  
            }

            if (creature->GetObjectType() == Protocol::PLAYER)
                scene->RemovePlayer(objectId);
            else if (creature->GetObjectType() == Protocol::MONSTER)
                scene->RemoveMonster(objectId);
        }));
    }
}

ZoneRef World::GetZoneByPos(Vector3 pos) const
{
    int32 zoneCount = _config.mapSize / _config.zoneSize;

    int32 x = static_cast<int32>(pos.x / _config.zoneSize);
    int32 z = static_cast<int32>(pos.z / _config.zoneSize);

    if (x < 0 || x >= zoneCount || z < 0 || z >= zoneCount)
        return nullptr;

    return _zones[z * zoneCount + x];
}

ZoneRef World::GetZoneById(int32 id) const
{
    if (id < 0 || id >= static_cast<int32>(_zones.size()))
        return nullptr;
    return _zones[id];
}

Vector<ZoneRef> World::GetAdjacentZones(Vector3 pos) const
{
    int32 zoneCount = _config.mapSize / _config.zoneSize;
    int32 centerX   = static_cast<int32>(pos.x / _config.zoneSize);
    int32 centerZ   = static_cast<int32>(pos.z / _config.zoneSize);

    Vector<ZoneRef> adjacent;
    adjacent.reserve(9);

    for (int32 dz = -1; dz <= 1; ++dz)
    {
        for (int32 dx = -1; dx <= 1; ++dx)
        {
            int32 nx = centerX + dx;
            int32 nz = centerZ + dz;
            if (nx >= 0 && nx < zoneCount && nz >= 0 && nz < zoneCount)
                adjacent.push_back(_zones[nz * zoneCount + nx]);
        }
    }
    return adjacent;
}

MoveResultRef World::GetMoveResult(ZoneRef oldZone, ZoneRef newZone) const
{
    if (!oldZone || !newZone) return nullptr;

    int32 oldId = oldZone->GetId();
    int32 newId = newZone->GetId();

    auto outerIt = _moveTable.find(oldId);
    if (outerIt != _moveTable.end())
    {
        auto innerIt = outerIt->second.find(newId);
        if (innerIt != outerIt->second.end())
            return innerIt->second;
    }

    return CalculateMoveResult(oldZone, newZone);
}

MoveResultRef World::CalculateMoveResult(ZoneRef oldZone, ZoneRef newZone) const
{
    const Vector<ZoneRef>& oldAdj = oldZone->GetAdjacentZones();
    const Vector<ZoneRef>& newAdj = newZone->GetAdjacentZones();

    MoveResultRef result = MakeShared<MoveResult>();
    auto itOld = oldAdj.begin();
    auto itNew = newAdj.begin();

    while (itOld != oldAdj.end() && itNew != newAdj.end())
    {
        int32 oldAdjId = (*itOld)->GetId();
        int32 newAdjId = (*itNew)->GetId();

        if (oldAdjId < newAdjId)
            result->leaveZones.push_back(*itOld++);
        else if (oldAdjId > newAdjId)
            result->enterZones.push_back(*itNew++);
        else
        {
            result->keepZones.push_back(*itOld);
            ++itOld; ++itNew;
        }
    }

    while (itOld != oldAdj.end()) result->leaveZones.push_back(*itOld++);
    while (itNew != newAdj.end()) result->enterZones.push_back(*itNew++);

    return result;
}

void World::SpawnMonsters()
{
    const Vector<SpawnData>& spawnList = GDataManager->GetMonsterSpawnList();
    for (const SpawnData& data : spawnList)
    {
        MonsterRef monster = MakeShared<Monster>();
        monster->Init(data);
        EnterCreature(monster);
    }
}

void World::SendSpawnPacketsToPlayer(PlayerRef player, const Vector<ZoneRef>& zones)
{
    if (!player) return;

    Protocol::SUpdateScene packet;
    auto flush = [&]()
    {
        if (packet.spawns_size() == 0) return;
        player->Send(MakeSendBuffer<Protocol::MsgId::S_UPDATE_SCENE>(packet));
        packet.Clear();
    };

    for (const ZoneRef& zone : zones)
        zone->MakeSpawnPacket(player, packet, flush);
}

void World::SendDespawnPacketsToPlayer(PlayerRef player, const Vector<ZoneRef>& zones)
{
    if (!player) return;

    Protocol::SUpdateScene packet;
    auto flush = [&]()
    {
        if (packet.despawns_size() == 0) return;
        player->Send(MakeSendBuffer<Protocol::MsgId::S_UPDATE_SCENE>(packet));
        packet.Clear();
    };

    for (const ZoneRef& zone : zones)
        zone->MakeDespawnPacket(player->GetObjectId(), packet, flush);
}

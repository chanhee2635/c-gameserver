#pragma once
#include "GameTypes.h"
#include "Protocol.pb.h"
#include <functional>

class Zone : public std::enable_shared_from_this<Zone>
{
public:
    Zone(int32 x, int32 z, int32 zoneSize);
    ~Zone() = default;

    bool IsActive() const  { return _playerCount.load(std::memory_order_relaxed) > 0; }
    void  AddPlayerCount() { _playerCount.fetch_add(1, std::memory_order_relaxed); }
    void  SubPlayerCount() { _playerCount.fetch_sub(1, std::memory_order_relaxed); }

    void         SetScene(GameSceneRef scene) { _scene = scene; _sceneRaw = scene.get(); }
    GameSceneRef GetScene()    const { return _scene.lock(); }
    GameScene*   GetSceneRaw() const { return _sceneRaw; }

    void    SetId(int32 id)  { _id = id; }
    int32   GetId()    const { return _id; }
    Vector3 GetPos()   const { return { (float)_x * _zoneSize, 0.f, (float)_z * _zoneSize }; }

    void                   SetAdjacentZones(Vector<ZoneRef> adjacent) { _adjacentZones = std::move(adjacent); }
    const Vector<ZoneRef>& GetAdjacentZones() const                   { return _adjacentZones; }

    const HashMap<uint64, PlayerRef>&  GetPlayers()  const { return _players; }
    const HashMap<uint64, MonsterRef>& GetMonsters() const { return _monsters; }

    void AddPendingDespawn(uint64 objectId)    { _pendingDespawns.push_back(objectId); }
    void AddPendingSpawn(CreatureRef creature);
    void AddPendingMove(CreatureRef creature);
    void ClearPending();

    bool IsEmpty() const
    {
        return _pendingDespawns.empty()
            && _pendingSpawns.empty()
            && _pendingMoves.empty();
    }

    void Enter(CreatureRef creature);
    void Leave(CreatureRef creature);

    template<typename Flush>
    void FillUpdatePacket(Protocol::SUpdateScene& packet, Flush&& flush);

    template<typename Flush>
    void MakeSpawnPacket(PlayerRef self, Protocol::SUpdateScene& packet, Flush&& flush);

    template<typename Flush>
    void MakeDespawnPacket(uint64 selfId, Protocol::SUpdateScene& packet, Flush&& flush);


    int32 GetPendingMoveCount() const { return static_cast<int32>(_pendingMoves.size()); }

private:
    template<typename Container, typename AddFunc, typename Flush>
    static void FillList(const Container& container, int32 flushCount,
        AddFunc&& addFunc, Flush&& flush)
    {
        int32 count = 0;
        for (const auto& item : container)
        {
            if (addFunc(item))
                if (++count >= flushCount) { flush(); count = 0; }
        }
        if (count > 0) flush();
    }

    Atomic<int32>    _playerCount = 0;
    GameSceneWeakRef _scene;
    GameScene*       _sceneRaw = nullptr;
    int32            _id;
    int32            _x;
    int32            _z;
    int32            _zoneSize;

    HashMap<uint64, PlayerRef>   _players;
    HashMap<uint64, MonsterRef>  _monsters;

    Vector<ZoneRef>     _adjacentZones;
    Vector<uint64>      _pendingDespawns;
    Vector<CreatureRef> _pendingSpawns;
    Vector<CreatureRef> _pendingMoves;
};

template<typename Flush>
void Zone::FillUpdatePacket(Protocol::SUpdateScene& packet, Flush&& flush)
{
    FillList(_pendingDespawns, GameConfig::Zone::DESPAWN_FLUSH_COUNT,
        [&](uint64 id) { packet.add_despawns(id); return true; }, flush);

    FillList(_pendingSpawns, GameConfig::Zone::SPAWN_FLUSH_COUNT,
        [&](const auto& c) { c->MakeObjectInfo(*packet.add_spawns()); return true; }, flush);

    FillList(_pendingMoves, GameConfig::Zone::MOVE_FLUSH_COUNT,
        [&](const auto& c) { c->MakePosInfo(*packet.add_moves()); return true; }, flush);
}

template<typename Flush>
void Zone::MakeSpawnPacket(PlayerRef self, Protocol::SUpdateScene& packet, Flush&& flush)
{
    FillList(_players, GameConfig::Zone::SPAWN_FLUSH_COUNT,
        [&](const auto& kv) {
            if (kv.second == self) return false;
            kv.second->MakeObjectInfo(*packet.add_spawns());
            return true;
        }, flush);

    FillList(_monsters, GameConfig::Zone::SPAWN_FLUSH_COUNT,
        [&](const auto& kv) { kv.second->MakeObjectInfo(*packet.add_spawns()); return true; }, flush);
}

template<typename Flush>
void Zone::MakeDespawnPacket(uint64 selfId, Protocol::SUpdateScene& packet, Flush&& flush)
{
    FillList(_players, GameConfig::Zone::DESPAWN_FLUSH_COUNT,
        [&](const auto& kv) {
            if (kv.first == selfId) return false;
            packet.add_despawns(kv.first);
            return true;
        }, flush);

    FillList(_monsters, GameConfig::Zone::DESPAWN_FLUSH_COUNT,
        [&](const auto& kv) { packet.add_despawns(kv.first); return true; }, flush);
}
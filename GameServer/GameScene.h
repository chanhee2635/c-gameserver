#pragma once
#include "GameTypes.h"
#include "Struct.h"

enum class VisionType { Spawn, Despawn, Move };

struct MoveNotice
{
    CreatureRef mover;
    ZoneRef     targetZone;
    VisionType  type;
};

class GameScene : public JobQueue
{
public:
    GameScene() = default;

    void  SetId(int32 id) { _id = id; }
    int32 GetId() const { return _id; }

    void AddZone(ZoneRef zone);

    void AddPlayer(PlayerRef player);
    void RemovePlayer(uint64 objectId);
    void AddMonster(MonsterRef monster);
    void RemoveMonster(uint64 objectId);

    void BroadcastToZone(ZoneRef zone, SendBufferRef buffer, uint64 exceptId = 0);
    void BroadcastToAdjacentZones(ZoneRef zone, SendBufferRef buffer, uint64 exceptId = 0);
    void BroadcastToAllPlayers(SendBufferRef buffer);

    // Broadcast HP-change / death packets to adjacent zones (shared packet build).
    void BroadcastHpChange(const ZoneRef& zone, uint64 objectId, int32 hp, int32 damage);
    void BroadcastDie(const ZoneRef& zone, uint64 objectId);

    void FindNearestPlayer(Vector<ZoneRef> zones, MonsterRef monster, Vector3 monsterPos);
    void HandleAttackHitDetection(PlayerRef attacker, Vector3 attackPos, float yaw);
    void HandleRevive(PlayerRef player, bool isCurrentPos);
    void HandleMonsterDead(MonsterRef monster);

    void Update();

private:
    void UpdateObjects(uint64 nowMs, uint32 deltaTimeMs, FrameVector<CreatureRef>& movingCreatures);
    void CollectMoveNotices(FrameVector<CreatureRef>& movingCreatures, FrameHashMap<GameScene*, Vector<MoveNotice>>& crossNotices);
    void HandleZoneChange(CreatureRef creature, ZoneRef oldZone, ZoneRef newZone, FrameHashMap<GameScene*, Vector<MoveNotice>>& crossNotices);
    void CollectMovingCreature(const CreatureRef& creature, uint64 nowMs, FrameVector<CreatureRef>& movingCreatures);
    void ApplyVision(const CreatureRef& creature, const ZoneRef& zone, VisionType type);
    void EmitVision(const CreatureRef& creature, const ZoneRef& zone, VisionType type, FrameHashMap<GameScene*, Vector<MoveNotice>>& crossNotices); 
    void DispatchNotices(FrameHashMap<GameScene*, Vector<MoveNotice>>& crossNotices);
    void ProcessNotices(const Vector<MoveNotice>& notices);
    void BroadcastScene();
    // Hands the filled packet to a send-lane which serializes it off the scene thread.
    void OffloadBroadcast(const ZoneRef& zone, std::shared_ptr<Protocol::SUpdateScene> packet);
    void ApplyHitToMonster(PlayerRef attacker, MonsterRef monster, Vector3 attackPos, ZoneRef myZone);

    template<typename Fn>
    void RunOnScene(const GameSceneRef& target, Fn&& fn)
    {
        if (!target) return;
        if (target.get() == this)
            fn();
        else
            target->DoAsync(MakeJob(std::forward<Fn>(fn)));
    }

private:
    int32  _id = 0;
    uint64 _lastUpdateTick = 0;

    Vector<ZoneRef>             _zones;
    HashMap<uint64, PlayerRef>  _players;
    HashMap<uint64, MonsterRef> _monsters;

    Protocol::SUpdateScene _broadcastPacket;
};

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

    void AddZone(ZoneRef zone);

    void AddPlayer(PlayerRef player);
    void RemovePlayer(uint64 objectId);
    void AddMonster(MonsterRef monster);
    void RemoveMonster(uint64 objectId);

    void PushMoveJob(MoveJob job);

    void BroadcastToZone(ZoneRef zone, SendBufferRef buffer, uint64 exceptId = 0);
    void BroadcastToAdjacentZones(ZoneRef zone, SendBufferRef buffer, uint64 exceptId = 0);

    void FindNearestPlayer(const Vector<ZoneRef>& zones, MonsterRef monster, Vector3 monsterPos);
    void HandleAttackHitDetection(PlayerRef attacker, Vector3 attackPos, float yaw);
    void HandleRevive(PlayerRef player, bool isCurrentPos);
    void HandleMonsterDead(MonsterRef monster);

    void Update();

private:
    void UpdateObjects(float deltaTime,
        FrameVector<MoveJob>& jobsCache,
        FrameVector<CreatureRef>& movingCreatures);

    void CollectMoveNotices(FrameVector<CreatureRef>& movingCreatures,
        FrameVector<CreatureRef>& snapshot,
        FrameHashMap<GameScene*, Vector<MoveNotice>>& sceneNotices);

    void HandleZoneChange(CreatureRef creature, ZoneRef oldZone, ZoneRef newZone,
        FrameHashMap<GameScene*, Vector<MoveNotice>>& sceneNotices);

    void CollectMovingCreature(const CreatureRef& creature,
        FrameVector<CreatureRef>& movingCreatures);

    void DispatchNotices(FrameHashMap<GameScene*, Vector<MoveNotice>>& sceneNotices);
    void ProcessNotices(const Vector<MoveNotice>& notices);
    void BroadcastScene();

    void ApplyHitToMonster(PlayerRef attacker, MonsterRef monster,
        Vector3 attackPos, ZoneRef myZone);

private:
    int64 _lastUpdateTick = 0;

    Vector<ZoneRef>            _zones;
    HashMap<uint64, PlayerRef>  _players;
    HashMap<uint64, MonsterRef> _monsters;

    Mutex           _moveLock;
    Vector<MoveJob> _pendingMoveJobs;
    /*Vector<MoveJob> _jobsCache;

    Vector<CreatureRef>                  _movingCreatures;
    Vector<CreatureRef>                  _movingCreaturesSnapshot;
    HashMap<GameScene*, Vector<MoveNotice>>  _sceneNotices;

    HashMap<GameScene*, Vector<ZoneRef>> _adjacentSceneGroups;*/

    Protocol::SUpdateScene _broadcastPacket;
};

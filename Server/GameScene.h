#pragma once

enum class VisionType
{
	Spawn,
	Despawn,
	Move
};

struct MoveJob
{
	uint64 objectId;
	Vector3 pos;
	float yaw;
	CreatureState state;
};

struct MoveNotice {
	CreatureRef mover;
	ZoneRef targetZone;
	VisionType type;
};

class GameScene : public JobQueue
{
public:
	GameScene();

	void AddZone(ZoneRef zone);

	void Update();
	void UpdateObjects();
	void CollectMovingCreature(const CreatureRef& creature);
	void ProcessNotices(const vector<MoveNotice>& notices);
	void FindNearestPlayer(Vector<ZoneRef> zones, MonsterRef monster, Vector3 monsterPos);

	void AddMonster(MonsterRef monster);
	void RemoveMonster(uint64 objectId);
	void AddPlayer(PlayerRef player);
	void RemovePlayer(uint64 objectId);

	void PushMoveJob(MoveJobRef job);

	void CollectMoveNotices();
	void HandleZoneChange(CreatureRef creature, ZoneRef oldZone, ZoneRef newZone);
	void DispatchNotices();
	void BroadcastScene();
	void BroadcastToZone(ZoneRef zone, SendBufferRef sendBuffer, uint64 exceptId = 0);
	void BroadcastToAdjacentZones(ZoneRef zone, SendBufferRef sendBuffer, uint64 exceptId = 0);

	void HandleAttackHitDetection(PlayerRef attacker, Vector3 attackPos, float yaw);
	void HandleRevive(PlayerRef player, bool isCurrentPos);

private:
	int64 _updateTick;

	Map<uint64, MonsterRef> _monsters;
	Vector<MonsterRef> snapshotMonsters;

	vector<CreatureRef> _movingCreatures;
	vector<CreatureRef> _movingCreaturesSnapshot;

	Vector<ZoneRef> _zones;
	uint64 _lastTick = 0;

	LockQueue<MoveJobRef> _moveJobs;
	Vector<MoveJobRef> _jobsCache;
	unordered_map<GameSceneRef, vector<MoveNotice>> _sceneNotices;

	unordered_map<uint64, PlayerRef> _players;

};


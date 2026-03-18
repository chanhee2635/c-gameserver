#pragma once
#include "Creature.h"

class Zone : public enable_shared_from_this<Zone>
{
friend class GameScene;
friend class World;

public:
	Zone(int32 x, int32 z);
	~Zone() 
	{
		_players.clear();
		_monsters.clear();
		_scene.reset();
	}

	bool IsActive() { return _playerCount > 0; }
	void AddPlayerCount() { _playerCount.fetch_add(1); }
	void SubPlayerCount() { _playerCount.fetch_sub(1); }
	void AddPendingDespawn(uint64 objectId) { _pendingDespawns.insert(objectId); }
	void AddPendingSpawn(CreatureRef creature) { _pendingSpawns[creature->GetObjectId()] = creature; }
	void AddPendingMove(CreatureRef creature) { _pendingMoves[creature->GetObjectId()] = creature; }
	void FillUpdatePacket(Protocol::S_UpdateScene& packet);
	void ClearPending();

	bool IsEmpty() { return _pendingDespawns.empty() && _pendingSpawns.empty() && _pendingMoves.empty(); }

	void SetScene(GameSceneRef scene) { _scene = scene; }
	GameSceneRef GetScene() { return _scene.lock(); }
	Vector3 GetPos();
	int32 GetId() { return _id; }
	void SetAdjacentZones(Vector<ZoneRef> adjacent) { _adjacentZones = adjacent; }
	const Vector<ZoneRef>& GetAdjacentZones() { return _adjacentZones; }

	void MakeSpawnPacket(PlayerRef self, Protocol::S_UpdateScene& packet);
	void MakeDespawnPacket(uint64 selfId, Protocol::S_UpdateScene& packet);

	void Enter(CreatureRef creature, bool sceneChanged = true);
	void Leave(CreatureRef creature, bool sceneChanged = true);

private:
	USE_LOCK;
	atomic<int32> _playerCount = 0;
	weak_ptr<GameScene> _scene;
	int32 _id;
	int32 _x;
	int32 _z;

	// zone에 위치했던 creature
	unordered_map<uint64, PlayerRef> _players;
	unordered_map<uint64, MonsterRef> _monsters;

	Vector<ZoneRef> _adjacentZones;

	// adjacentZones 의 변화가 있는 creature
	unordered_set<uint64> _pendingDespawns;
	unordered_map<uint64, CreatureRef> _pendingSpawns;
	unordered_map<uint64, CreatureRef> _pendingMoves;
};


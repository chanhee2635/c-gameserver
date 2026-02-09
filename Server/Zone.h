#pragma once

class Zone
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

	void SetScene(GameSceneRef scene) { _scene = scene; }
	GameSceneRef GetScene() { return _scene.lock(); }
	Vector3 GetPos();
	int32 GetId() { return _id; }

	void MakeSpawnPacket(Protocol::S_Spawn& packet);
	void MakeDespawnPacket(OUT Protocol::S_Despawn& packet);

	void Enter(CreatureRef creature);
	void Leave(CreatureRef creature);

private:
	USE_LOCK;
	weak_ptr<GameScene> _scene;
	int32 _id;
	int32 _x;
	int32 _z;
	Map<uint64, PlayerRef> _players;
	Map<uint64, MonsterRef> _monsters;
};


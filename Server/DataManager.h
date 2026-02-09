#pragma once

class DataManager
{
public:
	/* 게임 서비스에 필요한 데이터 로드 */
	bool LoadData();

private:
	bool LoadPrefabData();
	bool LoadPlayerData();
	bool LoadMonsterData();
	bool LoadSpawnData();
	bool LoadMonsterSpawnData();

public:
	const PrefabData* GetPrefabData(int32 templateId);
	const PlayerData* GetPlayerData(int32 templateId, int32 level = 1);
	const MonsterData* GetMonsterData(int32 templateId);
	const SpawnData* GetSpawnData(SpawnPoint spawnPoint);
	const vector<SpawnData>& GetMonsterSpawnList() { return _monsterSpawnList; };

	int64 GetPlayerRequireExp(int32 templateId, int32 level);
	int32 GetMaxCombo(int32 templateId);
	SpawnData* GetNearestSpawnData(Vector3 playerPos);

private:
	map<int32, PrefabData>					_prefabData;
	map<pair<int32, int32>, PlayerData>		_playerData;
	map<int32, MonsterData>					_monsterData;
	map<int32, SpawnData>					_spawnData;
	vector<SpawnData>						_monsterSpawnList;
};

extern std::unique_ptr<DataManager>	GDataManager;
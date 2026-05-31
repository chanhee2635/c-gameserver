#pragma once
#include "Struct.h"

class DataManager
{
public:
    bool LoadData();

    // 조회
    const PrefabData*  GetPrefabData(int32 templateId) const;
    const PlayerData*  GetPlayerData(int32 templateId, int32 level) const;
    bool               IsValidPlayerTemplate(int32 templateId) const;
    const MonsterData* GetMonsterData(int32 templateId) const;

    int64 GetPlayerRequireExp(int32 templateId, int32 level) const;
    int32 GetMaxCombo(int32 templateId) const;
    int32 GetDeathDurationMs(int32 templateId) const;

    const SpawnData*         GetPlayerSpawnData(int32 id) const;
    const SpawnData*         GetNearestPlayerSpawn(Vector3 pos) const;
    const Vector<SpawnData>& GetMonsterSpawnList() const { return _monsterSpawnList; }

private:
    bool LoadPrefabData();
    bool LoadPlayerData();
    bool LoadMonsterData();
    bool LoadPlayerSpawnData();
    bool LoadMonsterSpawnData();

    Map<int32, PrefabData>              _prefabData;
    Map<std::pair<int32, int32>, PlayerData> _playerData;      // (templateId, level)
    Map<int32, MonsterData>             _monsterData;
    Map<int32, SpawnData>               _playerSpawnData;
    Vector<SpawnData>                   _monsterSpawnList;
};

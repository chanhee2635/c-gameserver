#include "pch.h"
#include "DataManager.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;


bool DataManager::LoadData()
{
    if (!LoadPrefabData())       return false;
    if (!LoadPlayerData())       return false;
    if (!LoadMonsterData())      return false;
    if (!LoadPlayerSpawnData())  return false;
    if (!LoadMonsterSpawnData()) return false;

    LOG_INFO(L"DataManager::LoadData() complete");
    return true;
}

bool DataManager::LoadPrefabData()
{
    _prefabData.clear();

    std::ifstream file("../Client/Assets/Resources/Data/PrefabData.json");
    if (!file.is_open())
    {
        LOG_ERROR(L"PrefabData.json open failed");
        return false;
    }

    json j = json::parse(file);
    for (const auto& item : j["data"])
    {
        PrefabData data;
        data.id              = item["id"];
        data.name            = Utils::ToWString(item["name"].get<string>());
        data.maxCombo        = item["maxCombo"];
        data.deathDurationMs = item["deathDurationMs"];
        data.poolSize        = item["poolSize"];

        const auto& delays = item["comboHitDelays"];
        data.comboHitDelays.resize(delays.size());
        for (int32 i = 0; i < static_cast<int32>(delays.size()); i++)
            data.comboHitDelays[i] = delays[i];

        _prefabData[data.id] = std::move(data);
    }
    return true;
}

bool DataManager::LoadPlayerData()
{
    _playerData.clear();

    std::ifstream file("../Client/Assets/Resources/Data/PlayerData.json");
    if (!file.is_open())
    {
        LOG_ERROR(L"PlayerData.json open failed");
        return false;
    }

    json j = json::parse(file);
    for (const auto& item : j["data"])
    {
        PlayerData data;
        data.id          = item["id"];
        data.level       = item["level"];
        data.maxHp       = item["maxHp"];
        data.maxMp       = item["maxMp"];
        data.reqExp      = item["reqExp"];
        data.attack      = item["attack"];
        data.defense     = item["defense"];
        data.speed       = item["speed"];
        data.attackRange = item["attackRange"];
        data.attackSpeed = item["attackSpeed"];
        data.attackAngle = item["attackAngle"];

        _playerData[{data.id, data.level}] = std::move(data);
    }
    return true;
}

bool DataManager::LoadMonsterData()
{
    _monsterData.clear();

    std::ifstream file("../Client/Assets/Resources/Data/MonsterData.json");
    if (!file.is_open())
    {
        LOG_ERROR(L"MonsterData.json open failed");
        return false;
    }

    json j = json::parse(file);
    for (const auto& item : j["data"])
    {
        MonsterData data;
        data.id             = item["id"];
        data.level          = item["level"];
        data.name           = Utils::ToWString(item["name"].get<string>());
        data.maxHp          = item["maxHp"];
        data.attack         = item["attack"];
        data.defense        = item["defense"];
        data.rewardExp      = item["rewardExp"];
        data.speed          = item["speed"];
        data.searchRange    = item["searchRange"];
        data.maxSearchRange = item["maxSearchRange"];
        data.attackRange    = item["attackRange"];
        data.attackSpeed    = item["attackSpeed"];

        _monsterData[data.id] = std::move(data);
    }
    return true;
}

bool DataManager::LoadPlayerSpawnData()
{
    _playerSpawnData.clear();

    std::ifstream file("../Client/Assets/Resources/Data/PlayerSpawnData.json");
    if (!file.is_open())
    {
        LOG_ERROR(L"PlayerSpawnData.json open failed");
        return false;
    }

    json j = json::parse(file);
    for (const auto& item : j["data"])
    {
        SpawnData data;
        data.id    = item["id"];
        data.pos.x = item["x"];
        data.pos.y = item["y"];
        data.pos.z = item["z"];
        data.yaw   = item["yaw"];

        _playerSpawnData[data.id] = std::move(data);
    }
    return true;
}

bool DataManager::LoadMonsterSpawnData()
{
    _monsterSpawnList.clear();

    std::ifstream file("../Client/Assets/Resources/Data/MonsterSpawnData.json");
    if (!file.is_open())
    {
        LOG_ERROR(L"MonsterSpawnData.json open failed");
        return false;
    }

    json j = json::parse(file);
    for (const auto& item : j["data"])
    {
        SpawnData data;
        data.id             = item["id"];
        data.pos.x          = item["x"];
        data.pos.y          = item["y"];
        data.pos.z          = item["z"];
        data.yaw            = item["yaw"];
        data.respawnDelayMs = item["respawnDelayMs"];

        _monsterSpawnList.push_back(std::move(data));
    }
    return true;
}

const PrefabData* DataManager::GetPrefabData(int32 templateId) const
{
    auto it = _prefabData.find(templateId);
    return it != _prefabData.end() ? &it->second : nullptr;
}

const PlayerData* DataManager::GetPlayerData(int32 templateId, int32 level) const
{
    auto it = _playerData.find({templateId, level});
    return it != _playerData.end() ? &it->second : nullptr;
}

const MonsterData* DataManager::GetMonsterData(int32 templateId) const
{
    auto it = _monsterData.find(templateId);
    return it != _monsterData.end() ? &it->second : nullptr;
}

int64 DataManager::GetPlayerRequireExp(int32 templateId, int32 level) const
{
    const PlayerData* data = GetPlayerData(templateId, level);
    return data ? data->reqExp : LLONG_MAX;
}

int32 DataManager::GetMaxCombo(int32 templateId) const
{
    const PrefabData* data = GetPrefabData(templateId);
    return data ? data->maxCombo : 0;
}

int32 DataManager::GetDeathDurationMs(int32 templateId) const
{
    const PrefabData* data = GetPrefabData(templateId);
    return data ? data->deathDurationMs : 0;
}

const SpawnData* DataManager::GetPlayerSpawnData(int32 id) const
{
    auto it = _playerSpawnData.find(id);
    return it != _playerSpawnData.end() ? &it->second : nullptr;
}

const SpawnData* DataManager::GetNearestPlayerSpawn(Vector3 pos) const
{
    const SpawnData* nearest = nullptr;
    float minDistSq = FLT_MAX;

    for (const auto& [id, data] : _playerSpawnData)
    {
        float distSq = pos.DistanceSq(data.pos);
        if (distSq < minDistSq)
        {
            minDistSq = distSq;
            nearest   = &data;
        }
    }
    return nearest;
}

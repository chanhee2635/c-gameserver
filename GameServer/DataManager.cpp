#include "pch.h"
#include "DataManager.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace
{
    // Open and parse a data file, returning its "data" array node via outArray.
    // Logs an error and returns false on failure.
    bool LoadJsonArray(const char* path, json& outArray)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            LOG_ERROR(Utils::ToWString(path) + L" open failed");
            return false;
        }

        json j = json::parse(file, nullptr, false);
        if (j.is_discarded() || !j.contains("data"))
        {
            LOG_ERROR(Utils::ToWString(path) + L" parse failed");
            return false;
        }

        outArray = std::move(j["data"]);
        return true;
    }
}

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

    json data;
    if (!LoadJsonArray("../Client/Assets/Resources/Data/PrefabData.json", data))
        return false;

    for (const auto& item : data)
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

    json data;
    if (!LoadJsonArray("../Client/Assets/Resources/Data/PlayerData.json", data))
        return false;

    for (const auto& item : data)
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

    json data;
    if (!LoadJsonArray("../Client/Assets/Resources/Data/MonsterData.json", data))
        return false;

    for (const auto& item : data)
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

    json data;
    if (!LoadJsonArray("../Client/Assets/Resources/Data/PlayerSpawnData.json", data))
        return false;

    for (const auto& item : data)
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

    json data;
    if (!LoadJsonArray("../Client/Assets/Resources/Data/MonsterSpawnData.json", data))
        return false;

    for (const auto& item : data)
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

bool DataManager::IsValidPlayerTemplate(int32 templateId) const
{
    // A creatable class must have level-1 base data.
    return GetPlayerData(templateId, 1) != nullptr;
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

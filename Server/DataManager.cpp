#include "pch.h"
#include "DataManager.h"
#include "DBConnectionPool.h"
#include "DBBind.h"
#include <limits>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

unique_ptr<DataManager>	GDataManager = make_unique<DataManager>();

bool DataManager::LoadData()
{
	if(!LoadPrefabData()) return false;
	if(!LoadPlayerData()) return false;
	if(!LoadMonsterData()) return false;
	if(!LoadSpawnData()) return false;
	if(!LoadMonsterSpawnData()) return false;

	return true;
}

bool DataManager::LoadPrefabData()
{
	_prefabData.clear();

	std::ifstream file("../../Client/Assets/Resources/Data/PrefabData.json");
	if (!file.is_open()) return false;

	json prefabData;
	file >> prefabData;

	auto& configs = prefabData["data"];

	for (auto& item : configs)
	{
		PrefabData data;
		data.id = item["id"];
		string s1 = item["name"];
		data.name = Utils::s2ws(s1);
		string s2 = item["prefabPath"];
		data.prefabPath = Utils::s2ws(s2);
		data.maxCombo = item["maxCombo"];

		data.comboHitDelays.resize(data.maxCombo + 1, 0);
		if (item.contains("comboHitDelays"))
		{
			const auto& delays = item["comboHitDelays"];
			for (int i = 0; i <= data.maxCombo && i < (int)delays.size(); ++i)
				data.comboHitDelays[i] = delays[i];
		}

		data.deathDuration = item["deathDuration"];

		_prefabData[data.id] = data;
	}

	return true;
}

bool DataManager::LoadPlayerData()
{
	_playerData.clear();

	std::ifstream file("../../Client/Assets/Resources/Data/PlayerData.json");
	if (!file.is_open()) return false;

	json playerData;
	file >> playerData;

	auto& configs = playerData["data"];

	for (auto& item : configs)
	{
		PlayerData data;
		data.id = item["id"];
		data.level = item["level"];
		data.maxHp = item["maxHp"];
		data.maxMp = item["maxMp"];
		data.reqExp = item["reqExp"];
		data.attack = item["attack"];
		data.defense = item["defense"];
		data.speed = item["speed"];
		data.attackRange = item["attackRange"];
		data.attackSpeed = item["attackSpeed"];
		data.attackAngle = item["attackAngle"];
		_playerData[make_pair(data.id, data.level)] = data;
	}

	return true;
}

bool DataManager::LoadMonsterData()
{
	_monsterData.clear();

	std::ifstream file("../../Client/Assets/Resources/Data/MonsterData.json");
	if (!file.is_open()) return false;

	json monsterData;
	file >> monsterData;

	auto& configs = monsterData["data"];

	for (auto& item : configs)
	{
		MonsterData data;
		data.id = item["id"];
		data.level = item["level"];
		data.name = Utils::s2ws(item["name"]);
		data.maxHp = item["maxHp"];
		data.attack = item["attack"];
		data.defense = item["defense"];
		data.rewardExp = item["rewardExp"];
		data.speed = item["speed"];
		data.searchRange = item["searchRange"];
		data.maxSearchRange = item["maxSearchRange"];
		data.attackRange = item["attackRange"];
		data.attackSpeed = item["attackSpeed"];
		_monsterData[data.id] = data;
	}

	return true;
}

bool DataManager::LoadSpawnData()
{
	_spawnData.clear();

	std::ifstream file("../../Client/Assets/Resources/Data/PlayerSpawnData.json");
	if (!file.is_open()) return false;

	json spawnJson;
	file >> spawnJson;

	for (auto& item : spawnJson)
	{
		SpawnData data;
		data.id = item["id"];
		data.pos.x = item["x"];
		data.pos.y = item["y"];
		data.pos.z = item["z"];
		data.yaw = item["yaw"];

		_spawnData[data.id] = data;
	}

	return true;
}

bool DataManager::LoadMonsterSpawnData()
{
	_monsterSpawnList.clear();

	std::ifstream file("../../Client/Assets/Resources/Data/MonsterSpawnData.json");
	if (!file.is_open()) return false;

	json spawnJson;
	file >> spawnJson;

	for (auto& item : spawnJson)
	{
		SpawnData data;
		data.id = item["id"];
		data.pos.x = item["x"];
		data.pos.y = item["y"];
		data.pos.z = item["z"];
		data.yaw = item["yaw"];
		data.respawnTick = item["respawnTime"];
		_monsterSpawnList.push_back(data);
	}

	return true;
}

const PrefabData* DataManager::GetPrefabData(int32 templateId)
{
	auto it = _prefabData.find(templateId);
	if (it != _prefabData.end())
		return &it->second;

	return nullptr;
}

const PlayerData* DataManager::GetPlayerData(int32 templateId, int32 level)
{
	auto it = _playerData.find(make_pair(templateId, level));
	if (it != _playerData.end())
		return &it->second;

	return nullptr;
}

const MonsterData* DataManager::GetMonsterData(int32 templateId)
{
	auto it = _monsterData.find(templateId);
	if (it != _monsterData.end())
		return &it->second;

	return nullptr;
}

const SpawnData* DataManager::GetSpawnData(SpawnPoint spawnPoint)
{
	auto it = _spawnData.find((int32)spawnPoint);
	if (it != _spawnData.end())
		return &it->second;

	return nullptr;
}

int64 DataManager::GetPlayerRequireExp(int32 templateId, int32 level)
{
	const PlayerData* data = GetPlayerData(templateId, level);
	if (data == nullptr)
		return LONG_MAX;

	return data->reqExp;
}

int32 DataManager::GetMaxCombo(int32 templateId)
{
	const PrefabData* data = GetPrefabData(templateId);
	if (data == nullptr)
		return 0;

	return data->maxCombo;
}

int32 DataManager::GetDeathDuration(int32 templateId)
{
	const PrefabData* data = GetPrefabData(templateId);
	if (data == nullptr)
		return 0;
	return data->deathDuration;
}

SpawnData* DataManager::GetNearestSpawnData(Vector3 playerPos)
{
	SpawnData* nearest = nullptr;
	float minDistanceSq = (std::numeric_limits<float>::max)();

	for (auto& item : _spawnData)
	{
		SpawnData& data = item.second;

		float distSq = Vector3::DistanceSquared(playerPos, data.pos);

		if (distSq < minDistanceSq)
		{
			minDistanceSq = distSq;
			nearest = &data;
		}
	}
	return nearest;
}

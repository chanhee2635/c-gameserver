#include "pch.h"
#include "Zone.h"
#include "GameScene.h"
#include "Player.h"
#include "Monster.h"
#include "RedisManager.h"
#include "ConfigManager.h"

Zone::Zone(int32 x, int32 z) : _x(x), _z(z)
{
	int32 zoneCount = GConfigManager->GetWorld().mapSize / GConfigManager->GetWorld().zoneSize;
	_id = (z * zoneCount) + x;

	_players.reserve(100);
	_monsters.reserve(50);
}

void Zone::FillUpdatePacket(Protocol::S_UpdateScene& packet)
{
	for (uint64 id : _pendingDespawns)
	{
		packet.add_despawns(id);
	}
	for (auto& [id, creature] : _pendingSpawns)
	{
		auto* info = packet.add_spawns();
		creature->MakeObjectInfo(*info);
	}
	for (auto& [id, creature] : _pendingMoves)
	{
		auto* info = packet.add_moves();
		creature->MakePosInfo(*info);
	}
}

void Zone::ClearPending()
{
	_pendingDespawns.clear();
	_pendingSpawns.clear();
	_pendingMoves.clear();
}

Vector3 Zone::GetPos()
{
	int32 zoneSize = GConfigManager->GetWorld().zoneSize;
	return Vector3((float)_x * zoneSize, 0.0f, (float)_z * zoneSize);
}

void Zone::MakeSpawnPacket(PlayerRef self, OUT Protocol::S_UpdateScene& packet)
{
	if (_players.empty() && _monsters.empty()) return;

	for (auto& item : _players)
	{
		if (PlayerRef player = item.second)
		{
			if (player == self) continue;
			auto info = packet.add_spawns();
			player->MakeObjectInfo(*info);
		}
	}

	for (auto& item : _monsters)
	{
		if (MonsterRef monster = item.second) 
		{
			auto info = packet.add_spawns();
			monster->MakeObjectInfo(*info);
		}
	}
}

void Zone::MakeDespawnPacket(uint64 selfId, OUT Protocol::S_UpdateScene& packet)
{
	if (_players.empty() && _monsters.empty()) return;

	for (auto& item : _players)
	{
		if (item.first == selfId) continue;
		packet.add_despawns(item.first);
	}

	for (auto& item : _monsters)
	{
		packet.add_despawns(item.first);
	}
}

void Zone::Enter(CreatureRef creature, bool sceneChanged)
{
	if (creature == nullptr) return;

	uint64 objectId = creature->GetObjectId();
	switch (creature->GetObjectType())
	{
		case GameObjectType::Player:
		{
			PlayerRef player = static_pointer_cast<Player>(creature);
			_players[objectId] = player;
			AddPlayerCount();
			GRedisManager->PublishZoneChange(objectId, _id);
			if (sceneChanged) GetScene()->AddPlayer(player);
			break;
		}
		case GameObjectType::Monster:
		{
			MonsterRef monster = static_pointer_cast<Monster>(creature);
			_monsters[objectId] = monster;
			if (sceneChanged) GetScene()->AddMonster(monster);
			break;
		}
	}
}

void Zone::Leave(CreatureRef creature, bool sceneChanged)
{
	if (creature == nullptr) return;

	uint64 objectId = creature->GetObjectId();
	switch (creature->GetObjectType())
	{
		case GameObjectType::Player:
		{
			_players.erase(objectId);
			SubPlayerCount();
			if (sceneChanged) GetScene()->RemovePlayer(objectId);
			break;
		}
		case GameObjectType::Monster:
		{
			_monsters.erase(objectId);
			if (sceneChanged) GetScene()->RemoveMonster(objectId);
			break;
		}
	}
}
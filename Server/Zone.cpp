#include "pch.h"
#include "Zone.h"
#include "Creature.h"
#include "Player.h"
#include "Monster.h"
#include "ClientPacketHandler.h"
#include "RedisManager.h"
#include "ConfigManager.h"

Zone::Zone(int32 x, int32 z) : _x(x), _z(z)
{
	_id = (z * GConfigManager->GetWorld().zoneSize) + x;
}

Vector3 Zone::GetPos()
{
	int32 zoneSize = GConfigManager->GetWorld().zoneSize;
	return Vector3((float)_x * zoneSize, 0.0f, (float)_z * zoneSize);
}

void Zone::MakeSpawnPacket(OUT Protocol::S_Spawn& packet)
{
	if (_players.empty() && _monsters.empty()) return;

	for (auto& item : _players)
	{
		if (item.second != nullptr)
		{
			auto info = packet.add_infos();
			item.second->MakeObjectInfo(*info);
		}
	}

	for (auto& item : _monsters)
	{
		if (item.second != nullptr) 
		{
			auto info = packet.add_infos();
			item.second->MakeObjectInfo(*info);
		}
	}
}

void Zone::MakeDespawnPacket(OUT Protocol::S_Despawn& packet)
{
	if (_players.empty() && _monsters.empty()) return;

	for (auto& item : _players)
		packet.add_objectids(item.first);

	for (auto& item : _monsters)
		packet.add_objectids(item.first);
}

void Zone::Enter(CreatureRef creature)
{
	if (creature == nullptr) return;

	uint64 objectId = creature->GetObjectId();
	GameObjectType type = creature->GetObjectType();

	switch (type)
	{
		case GameObjectType::Player:
		{
			PlayerRef player = static_pointer_cast<Player>(creature);
			_players[objectId] = player;
			GRedisManager->PublishZoneChange(objectId, _id);
			break;
		}
		case GameObjectType::Monster:
		{
			MonsterRef monster = static_pointer_cast<Monster>(creature);
			_monsters[objectId] = monster;
			break;
		}
	}
}

void Zone::Leave(CreatureRef creature)
{
	if (creature == nullptr) return;

	uint64 objectId = creature->GetObjectId();
	GameObjectType type = creature->GetObjectType();

	switch (type)
	{
		case GameObjectType::Player:
		{
			_players.erase(objectId);
			break;
		}
		case GameObjectType::Monster:
		{
			_monsters.erase(objectId);
			break;
		}
	}
}
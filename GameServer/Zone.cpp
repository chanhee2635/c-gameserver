#include "pch.h"
#include "Zone.h"
#include "Creature.h"
#include "Player.h"
#include "Monster.h"

void Zone::AddPendingSpawn(CreatureRef creature) { _pendingSpawns[creature->GetObjectId()] = creature; }
void Zone::AddPendingMove(CreatureRef creature)  { _pendingMoves[creature->GetObjectId()] = creature; }

Zone::Zone(int32 x, int32 z, int32 zoneSize)
    : _x(x), _z(z), _zoneSize(zoneSize), _id(0)
{
    _players.reserve(64);
    _monsters.reserve(32);
}

void Zone::Enter(CreatureRef creature)
{
    if (!creature) return;

    uint64 objectId = creature->GetObjectId();

    switch (creature->GetObjectType())
    {
    case Protocol::PLAYER:
    {
        PlayerRef player = std::static_pointer_cast<Player>(creature);
        _players[objectId] = player;
        AddPlayerCount();
        break;
    }
    case Protocol::MONSTER:
    {
        MonsterRef monster = std::static_pointer_cast<Monster>(creature);
        _monsters[objectId] = monster;
        break;
    }
    default:
        break;
    }
}

void Zone::Leave(CreatureRef creature)
{
    if (!creature) return;

    uint64 objectId = creature->GetObjectId();

    switch (creature->GetObjectType())
    {
    case Protocol::PLAYER:
        _players.erase(objectId);
        SubPlayerCount();
        break;
    case Protocol::MONSTER:
        _monsters.erase(objectId);
        break;
    default:
        break;
    }
}

void Zone::ClearPending()
{
    _pendingDespawns.clear();
    _pendingSpawns.clear();
    _pendingMoves.clear();
}


#pragma once

enum class CreatureState : uint8
{
	Idle = 0,
	Moving = 1,
	Sprinting = 2,
	Attack = 3,
	Dead = 4
};

enum class GameObjectType : uint8
{
	UnKnown = 0,
	Player = 1,
	Monster = 2
};

enum
{
	DAMAGE_TICK = 100,
	DEAD_TICK = 3000,
	SEARCH_TICK = 1000,
};

enum class SpawnPoint
{
	BEGIN = 1,
	CENTER = 3
};


#pragma once

enum class CreatureState : uint8
{
	None = 0,
	Idle = 1,
	Moving = 2,
	Attack = 3,
	Dead = 4
};

enum class GameObjectType : uint8
{
	None = 0,
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
};


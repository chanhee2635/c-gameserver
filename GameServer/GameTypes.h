#pragma once
#include <memory>
#include "Container.h"
#include "Utils.h"
#include "GameMath.h"

class GameSession;
class GameScene;
class Zone;
class GameObject;
class Creature;
class Player;
class Monster;

using GameSessionRef = std::shared_ptr<GameSession>;
using GameSessionWeakRef = std::weak_ptr<GameSession>;
using GameSceneRef = std::shared_ptr<GameScene>;
using GameSceneWeakRef = std::weak_ptr<GameScene>;
using ZoneRef = std::shared_ptr<Zone>;
using ZoneWeakRef = std::weak_ptr<Zone>;
using GameObjectRef = std::shared_ptr<GameObject>;
using CreatureRef = std::shared_ptr<Creature>;
using CreatureWeakRef = std::weak_ptr<Creature>;
using PlayerRef = std::shared_ptr<Player>;
using PlayerWeakRef = std::weak_ptr<Player>;
using MonsterRef = std::shared_ptr<Monster>;
using MonsterWeakRef = std::weak_ptr<Monster>;

struct MoveResult;
using MoveResultRef = std::shared_ptr<MoveResult>;
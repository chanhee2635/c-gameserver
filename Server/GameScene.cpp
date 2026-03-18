#include "pch.h"
#include "GameScene.h"
#include "Player.h"
#include "Monster.h"
#include "Zone.h"
#include "World.h"
#include "ClientPacketHandler.h"
#include "ConfigManager.h"
#include "DataManager.h"
#include "RedisManager.h"

GameScene::GameScene()
{
	_updateTick = GConfigManager->GetLogic().updateTick;
}

void GameScene::AddZone(ZoneRef zone)
{
	_zones.push_back(zone);
}

void GameScene::Update()
{
	UpdateObjects();
	CollectMoveNotices();
	DispatchNotices();
	BroadcastScene();

	DoTimer(_updateTick, &GameScene::Update);
}


void GameScene::UpdateObjects()
{
	_movingCreatures.clear();
	_jobsCache.clear();
	_moveJobs.PopAll(OUT _jobsCache);

	for (MoveJobRef job : _jobsCache)
	{
		auto it = _players.find(job->objectId);
		if (it != _players.end())
		{
			PlayerRef& player = it->second;
			player->HandleMoveJob(job);
			if (player->IsDirty())
			{
				CollectMovingCreature(player);
			}
		}
	}

	for (auto& [id, monster] : _monsters)
	{
		if (monster->IsDead()) continue;
		monster->Update();
		if (monster->IsDirty())
			CollectMovingCreature(monster);
	}
}

void GameScene::CollectMovingCreature(const CreatureRef& creature)
{
	_movingCreatures.push_back(creature);
	creature->SetDirty(false);
}

void GameScene::CollectMoveNotices()
{
	_movingCreaturesSnapshot.clear();
	_movingCreaturesSnapshot.swap(_movingCreatures);
	_sceneNotices.clear();

	for (CreatureRef& creature : _movingCreaturesSnapshot)
	{
		ZoneRef oldZone = creature->GetZone();
		if (oldZone == nullptr) 
			continue;

		Vector3 pos = creature->GetPos();
		ZoneRef newZone = GWorld->GetZoneByPos(pos);
		if (newZone == nullptr) 
			continue;

		if (oldZone == newZone)
		{
			const auto& adjacentZones = oldZone->GetAdjacentZones();
			for (ZoneRef zone : adjacentZones)
				_sceneNotices[zone->GetScene()].push_back({ creature, zone, VisionType::Move });
		}
		else
		{
			HandleZoneChange(creature, oldZone, newZone);
		}
	}
}

void GameScene::HandleZoneChange(CreatureRef creature, ZoneRef oldZone, ZoneRef newZone)
{
	GameSceneRef oldScene = oldZone->GetScene();
	GameSceneRef newScene = newZone->GetScene();
	bool isSceneChanged = (oldScene != newScene);

	oldZone->Leave(creature, isSceneChanged);

	if (isSceneChanged)
	{
		creature->SetZone(newZone);
		newScene->DoAsyncPush([newZone, newScene, creature]() {
			newZone->Enter(creature, true);
			creature->SetGameScene(newScene);
		});
	}
	else
	{
		creature->SetZone(newZone);
		newZone->Enter(creature, false);
	}

	MoveResultRef moveTable = GWorld->GetMoveResult(oldZone, newZone);
	if (moveTable == nullptr) return;

	for (ZoneRef zone : moveTable->enterZones)
		_sceneNotices[zone->GetScene()].push_back({ creature, zone, VisionType::Spawn });
	for (ZoneRef zone : moveTable->leaveZones)
		_sceneNotices[zone->GetScene()].push_back({ creature, zone, VisionType::Despawn });
	for (ZoneRef zone : moveTable->keepZones)
		_sceneNotices[zone->GetScene()].push_back({ creature, zone, VisionType::Move });
}

void GameScene::DispatchNotices()
{
	for (auto& [scene, notices] : _sceneNotices)
	{
		if (!scene || notices.empty()) continue;

		if (scene.get() == this)
		{
			ProcessNotices(notices);
		}
		else
		{
			scene->DoAsyncPush([scene, moveNotices = std::move(notices)]() mutable {
				scene->ProcessNotices(moveNotices);
			});
		}
	}

	_sceneNotices.clear();
}

void GameScene::ProcessNotices(const vector<MoveNotice>& notices)
{
	for (const auto& notice : notices)
	{
		ZoneRef zone = notice.targetZone;
		CreatureRef creature = notice.mover;

		switch (notice.type)
		{
		case VisionType::Spawn:
			zone->AddPendingSpawn(creature);
			if (creature->GetObjectType() == GameObjectType::Player)
			{
				PlayerRef player = static_pointer_cast<Player>(creature);
				Protocol::S_UpdateScene packet;
				zone->MakeSpawnPacket(player, packet);
				if (packet.spawns_size() > 0)
					player->Send(ClientPacketHandler::MakeSendBuffer(packet));
			}
			break;
		case VisionType::Despawn:
			zone->AddPendingDespawn(creature->GetObjectId());
			if (creature->GetObjectType() == GameObjectType::Player)
			{
				PlayerRef player = static_pointer_cast<Player>(creature);
				Protocol::S_UpdateScene packet;
				zone->MakeDespawnPacket(player->GetObjectId(), packet);
				if (packet.despawns_size() > 0)
					player->Send(ClientPacketHandler::MakeSendBuffer(packet));
			}
			break;
		case VisionType::Move:
			zone->AddPendingMove(notice.mover);
			break;
		}
	}
}

void GameScene::BroadcastScene()
{
	for (ZoneRef& zone : _zones)
	{
		if (!zone->IsEmpty())
		{
			if (zone->IsActive())
			{
				Protocol::S_UpdateScene packet;
				zone->FillUpdatePacket(packet);

				const int32 MAX_MOVES_PER_PACKET = 30;
				if (packet.moves_size() > MAX_MOVES_PER_PACKET)
				{
					Protocol::S_UpdateScene splitPacket;
					splitPacket.mutable_spawns()->CopyFrom(packet.spawns());
					splitPacket.mutable_despawns()->CopyFrom(packet.despawns());

					for (int32 i = 0; i < packet.moves_size(); i++)
					{
						splitPacket.add_moves()->CopyFrom(packet.moves(i));
						if (splitPacket.moves_size() >= MAX_MOVES_PER_PACKET)
						{
							BroadcastToZone(zone, ClientPacketHandler::MakeSendBuffer(splitPacket));
							splitPacket.clear_moves();
							splitPacket.clear_spawns();
							splitPacket.clear_despawns();
						}
					}
					if (splitPacket.moves_size() > 0 || splitPacket.spawns_size() > 0)
						BroadcastToZone(zone, ClientPacketHandler::MakeSendBuffer(splitPacket));
				}
				else if (packet.moves_size() > 0 || packet.spawns_size() > 0 || packet.despawns_size() > 0)
				{
					BroadcastToZone(zone, ClientPacketHandler::MakeSendBuffer(packet));
				}
			}
			zone->ClearPending();
		}
	}
}


void GameScene::BroadcastToZone(ZoneRef zone, SendBufferRef sendBuffer, uint64 exceptId)
{
	if (zone == nullptr || sendBuffer == nullptr) return;

	for (auto& [id, player] : zone->_players)
	{
		if (id == exceptId) continue;
		player->Send(sendBuffer);
	}
}

void GameScene::BroadcastToAdjacentZones(ZoneRef zone, SendBufferRef sendBuffer, uint64 exceptId)
{
	if (zone == nullptr || sendBuffer == nullptr) return;

	const Vector<ZoneRef>& adjacentZones = zone->GetAdjacentZones();

	unordered_map<GameScene*, Vector<ZoneRef>> sceneGroups;
	for (const ZoneRef& zone : adjacentZones)
	{
		GameSceneRef scene = zone->GetScene();
		if (scene == nullptr) continue;
		sceneGroups[scene.get()].push_back(zone);
	}

	for (auto& [scene, zones] : sceneGroups)
	{
		if (scene == this)
		{
			for (const ZoneRef& zone : zones)
				BroadcastToZone(zone, sendBuffer, exceptId);
		}
		else
		{
			GameSceneRef targetScene = zones[0]->GetScene();
			if (targetScene == nullptr) continue;

			targetScene->DoAsyncPush([targetScene, zones, sendBuffer, exceptId]() {
				for (const ZoneRef& zone : zones)
					targetScene->BroadcastToZone(zone, sendBuffer, exceptId);
			});
		}
	}
}

void GameScene::HandleAttackHitDetection(PlayerRef attacker, Vector3 attackPos, float yaw)
{
	ZoneRef myZone = attacker->GetZone();
	if (myZone == nullptr) return;

	float attackRange = attacker->GetAttackRange();
	float halfAngleRad = (attacker->GetAttackAngle() * 0.5f) * (3.14159f / 180.f);
	float cosHalfAngle = cosf(halfAngleRad);
	float attackRangeSq = attackRange * attackRange;

	float yawRad = yaw * (3.14159f / 180.f);
	Vector3 forward = { sinf(yawRad), 0.f, cosf(yawRad) };

	const Vector<ZoneRef>& adjacentZones = myZone->GetAdjacentZones();
	for (const ZoneRef& zone : adjacentZones)
	{
		for (auto& [monsterId, monster] : zone->_monsters)
		{
			if (monster->IsDead()) continue;

			Vector3 monsterPos = monster->GetPos();

			Vector3 toMonster = { monsterPos.x - attackPos.x, 0.f, monsterPos.z - attackPos.z };
			float distSq = toMonster.LengthSquared();
			if (distSq > attackRangeSq) continue;

			if (distSq > 0.001f)
			{
				Vector3 dir = toMonster;
				dir.Normalize();
				float dot = forward.Dot(dir);
				if (dot < cosHalfAngle) continue;  // 공격 각도 밖
			}

			int32 rawDamage = attacker->GetAttack();
			int32 actualDamage = monster->TakeDamage(rawDamage);
			int32 remainHp = monster->GetHp();

			Protocol::S_ChangeHp hpPkt;
			hpPkt.set_object_id(monsterId);
			hpPkt.set_hp(remainHp);
			hpPkt.set_damage(-actualDamage);
			BroadcastToAdjacentZones(myZone, ClientPacketHandler::MakeSendBuffer(hpPkt));

			if (monster->IsDead())
				attacker->GainExp(monster->GetRewardExp());
		}
	}
}

void GameScene::HandleRevive(PlayerRef player, bool isCurrentPos)
{
	if (player == nullptr || !player->IsDead()) return;

	Vector3 revivePos;
	if (isCurrentPos)
		revivePos = player->GetPos();
	else
	{
		SpawnData* nearest = GDataManager->GetNearestSpawnData(player->GetPos());
		if (nearest == nullptr) return;
		revivePos = nearest->pos;
	}

	player->Revive(revivePos);

	Protocol::S_Revive pkt;
	pkt.mutable_pos()->set_x(revivePos.x);
	pkt.mutable_pos()->set_y(revivePos.y);
	pkt.mutable_pos()->set_z(revivePos.z);
	pkt.set_hp(player->GetHp());
	pkt.set_max_hp(player->GetMaxHp());
	player->Send(ClientPacketHandler::MakeSendBuffer(pkt));

	GWorld->DoAsyncPush([player]() {
		GWorld->LeaveCreature(player);
	});
	GWorld->DoTimer(1, [player]() {
		GWorld->EnterCreature(player);
	});
}

void GameScene::FindNearestPlayer(Vector<ZoneRef> zones, MonsterRef monster, Vector3 monsterPos)
{
	if (monster == nullptr) return;

	PlayerRef bestPlayer = nullptr;
	float minDistSq = monster->GetSearchRangeSq();
	for (const ZoneRef& zone : zones)
	{
		for (auto& [id, player] : zone->_players)
		{
			if (player->IsDead()) continue;

			float distSq = (player->GetPos() - monsterPos).LengthSquared();
			if (distSq < minDistSq)
			{
				minDistSq = distSq;
				bestPlayer = player;
			}
		}
	}

	if (bestPlayer != nullptr)
	{
		if (monster->GetGameScene().get() == this)
			monster->HandleGatherResult(bestPlayer, minDistSq);
		else
			monster->GetGameScene()->DoAsync([monster, bestPlayer, minDistSq]() {
				monster->HandleGatherResult(bestPlayer, minDistSq);
			}); 
	}
}

void GameScene::AddMonster(MonsterRef monster)
{
	_monsters[monster->GetObjectId()] = monster;
}

void GameScene::RemoveMonster(uint64 objectId)
{
	_monsters.erase(objectId);
}

void GameScene::AddPlayer(PlayerRef player)
{
	_players[player->GetObjectId()] = player;
}

void GameScene::RemovePlayer(uint64 objectId)
{
	_players.erase(objectId);
}

void GameScene::PushMoveJob(MoveJobRef job)
{
	_moveJobs.Push(job);
}


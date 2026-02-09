#include "pch.h"
#include "GameScene.h"
#include "Player.h"
#include "Monster.h"
#include "Zone.h"
#include "World.h"
#include "ClientPacketHandler.h"
#include "DataManager.h"
#include "NavigationManager.h"
#include "ConfigManager.h"

void GameScene::AddZone(ZoneRef zone)
{
	_zones.push_back(zone);
}

void GameScene::Update()
{
	for (ZoneRef zone : _zones)
	{
		for (auto it = zone->_monsters.begin(); it != zone->_monsters.end();)
		{
			auto curIt = it++;
			MonsterRef monster = curIt->second;
			if (monster)
				monster->Update();
		}
	}

	DoTimer(GConfigManager->GetLogic().updateTick, &GameScene::Update);
}

void GameScene::HandlePlayerMove(PlayerRef player, Protocol::PosInfo posInfo)
{
	Vector3 oldPos = player->GetPos();
	Vector3 newPos = Vector3(posInfo.x(), posInfo.y(), posInfo.z());

	// NavMesh 위인지 확인
	Vector3 correctedPos;
	if (GNavigationManager->IsValidLocation(newPos, correctedPos) == false)
	{
		return;
	}

	// 속도 핵 감지
	float actualDeltaTime = (::GetTickCount64() / 1000.0f) - player->GetLastMoveTime();
	float maxDist = player->GetSpeed() * actualDeltaTime * 1.5f;
	float distance = Vector3::DistanceSquared(oldPos, newPos);
	if (distance > maxDist * maxDist)
	{
		return;
	}
	
	player->SetPosInfo(correctedPos, posInfo);

	HandleMove(player, oldPos, newPos);

}

void GameScene::HandleMove(CreatureRef creature, Vector3 oldPos, Vector3 newPos)
{
	ZoneRef oldZone = GWorld->GetZoneByPos(oldPos);
	ZoneRef newZone = GWorld->GetZoneByPos(newPos);
	if (oldZone == nullptr || newZone == nullptr) return;

	if (oldZone != newZone)
	{
		HandleMoveZone(creature, oldZone, newZone);
	}
	else
	{
		Protocol::S_Move packet;
		packet.set_objectid(creature->GetObjectId());
		auto pos = packet.mutable_pos();
		creature->MakePosInfo(*pos);
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);
		BroadcastAround(newPos, sendBuffer, creature->GetObjectId());
	}
}

void GameScene::HandleMoveZone(CreatureRef creature, ZoneRef oldZone, ZoneRef newZone)
{
	if (creature == nullptr || oldZone == nullptr || newZone == nullptr) return;

	MoveResultRef result = GWorld->GetMoveResult(oldZone, newZone);
	if (result == nullptr) return;

	HandleVisionLeave(creature, result->leaveZones);
	oldZone->Leave(creature);

	auto newScene = newZone->GetScene();
	newScene->DoAsync([newScene, creature, newZone, result]() 
	{
		creature->SetGameScene(newZone->GetScene());
		newScene->HandleVisionEnter(creature, result->enterZones);
		newZone->Enter(creature);

		Protocol::S_Move packet;
		packet.set_objectid(creature->GetObjectId());
		auto pos = packet.mutable_pos();
		creature->MakePosInfo(*pos);
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);
		newScene->BroadcastToZones(result->keepZones, sendBuffer, creature->GetObjectId());
	});
}

void GameScene::HandleVisionEnter(CreatureRef creature, Vector<ZoneRef> zones)
{
	if (creature == nullptr) return;

	if (creature->GetObjectType() == GameObjectType::Player)
	{
		// 나에게 먼저 주변 정보를 보낸다.
		PlayerRef player = static_pointer_cast<Player>(creature);

		Protocol::S_Spawn packet;
		for (ZoneRef zone : zones)
			zone->MakeSpawnPacket(packet);

		if (packet.infos_size() != 0)
		{
			auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);
			player->Send(sendBuffer);
		}
	}

	HandleSpawn(creature, zones);
}

void GameScene::HandleSpawn(CreatureRef creature, Vector<ZoneRef> zones)
{
	if (creature == nullptr) return;

	Protocol::S_Spawn packet;
	auto info = packet.add_infos();
	creature->MakeObjectInfo(*info);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);
	BroadcastToZones(zones, sendBuffer, creature->GetObjectId());
}

void GameScene::HandleVisionLeave(CreatureRef creature, Vector<ZoneRef> zones)
{
	if (creature == nullptr) return;

	HandleDespawn(creature, zones);

	if (creature->GetObjectType() == GameObjectType::Player)
	{
		// 나에게 주변 정보를 보낸다.
		PlayerRef player = static_pointer_cast<Player>(creature);
		Protocol::S_Despawn packet;
		for (ZoneRef zone : zones)
			zone->MakeDespawnPacket(packet);

		if (packet.objectids_size() != 0)
		{
			auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);
			player->Send(sendBuffer);
		}
	}
}

void GameScene::HandleDespawn(CreatureRef creature, Vector<ZoneRef> zones)
{
	if (creature == nullptr) return;

	Protocol::S_Despawn packet;
	packet.add_objectids(creature->GetObjectId());
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);
	BroadcastToZones(zones, sendBuffer, creature->GetObjectId());
}

void GameScene::HandleRevive(PlayerRef player, bool currentPosRevive)
{
	if (player == nullptr || player->IsDead() == false) return;

	ZoneRef oldZone = GWorld->GetZoneByPos(player->GetPos());  // 지속적인 계산은 조금이라도 무리가 될 수 있기에 플레이어도 Zone을 들고 있어야할 듯

	// 플레이어의 정보를 저장
	if (!currentPosRevive)
	{
		const SpawnData* spawnData = GDataManager->GetNearestSpawnData(player->GetPos());
		if (spawnData)
		{
			player->SetPos(spawnData->pos);
			player->SetYaw(spawnData->yaw);
		}
	}
	player->SetRevive();
	
	ZoneRef newZone = GWorld->GetZoneByPos(player->GetPos());

	if (oldZone == nullptr || newZone == nullptr) return;

	if (oldZone != newZone)
	{
		HandleReviveZone(player, oldZone, newZone);
	}
	else
	{
		Protocol::S_Revive packet;
		packet.set_objectid(player->GetObjectId());
		packet.set_hp(player->GetHp());
		auto info = packet.mutable_posinfo();
		player->MakePosInfo(*info);
		BroadcastAround(player->GetPos(), ClientPacketHandler::MakeSendBuffer(packet));
	}
}

void GameScene::HandleReviveZone(PlayerRef player, ZoneRef oldZone, ZoneRef newZone)
{
	if (player == nullptr || oldZone == nullptr || newZone == nullptr) return;

	MoveResultRef result = GWorld->GetMoveResult(oldZone, newZone);
	if (result == nullptr) return;

	HandleVisionLeave(player, result->leaveZones);
	oldZone->Leave(player);

	auto newScene = newZone->GetScene();
	newScene->DoAsync([newScene, player, newZone, result]()
	{
		player->SetGameScene(newZone->GetScene());
		newScene->HandleVisionEnter(player, result->enterZones);
		newZone->Enter(player);

		Protocol::S_Revive packet;
		packet.set_objectid(player->GetObjectId());
		packet.set_hp(player->GetHp());
		auto info = packet.mutable_posinfo();
		player->MakePosInfo(*info);
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);
		newScene->BroadcastToZones(result->keepZones, sendBuffer);
	});
}

void GameScene::PlayerAttack(PlayerRef player, Vector<ZoneRef> zones)
{
	if (player == nullptr) return;

	Vector<MonsterRef> targets;
	for (ZoneRef zone : zones)
	{
		for (auto& item : zone->_monsters)
		{
			MonsterRef monster = item.second;
			if (player->InAttackRange(monster))
				monster->GetGameScene()->DoTimer(DAMAGE_TICK, &GameScene::ApplyDamage, (CreatureRef)player, (CreatureRef)monster);
		}
	}
}

void GameScene::ApplyDamage(CreatureRef attacker, CreatureRef target)
{
	if (attacker == nullptr || target == nullptr || target->IsDead()) return;

	if (target->GetGameScene().get() != this)
	{
		target->GetGameScene()->DoAsync(&GameScene::ApplyDamage, attacker, target);
		return;
	}

	int32 damage = attacker->CalculateDamage(target);
	target->OnDamage(damage);

	Protocol::S_ChangeHp packet;
	packet.set_objectid(target->GetObjectId());
	packet.set_hp(target->GetHp());
	packet.set_damage(damage);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);
	BroadcastAround(target->GetPos(), sendBuffer);

	if (target->IsDead())
		HandleDead(attacker, target);
}

void GameScene::HandleDead(CreatureRef attacker, CreatureRef target)
{
	if (target == nullptr || target->IsDead() == false) return;

	Protocol::S_Die packet;
	packet.set_objectid(target->GetObjectId());
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);
	BroadcastAround(target->GetPos(), sendBuffer);

	target->SetState(CreatureState::Dead);

	if (target->GetObjectType() == GameObjectType::Player)
	{
		// TODO: 패널티
		// 경험치 하락?
	}
	else if (target->GetObjectType() == GameObjectType::Monster)
	{
		MonsterRef monster = static_pointer_cast<Monster>(target);

		if (attacker->GetObjectType() == GameObjectType::Player)
		{
			PlayerRef player = static_pointer_cast<Player>(attacker);
			int32 rewardExp = monster->GetConfig()->rewardExp;
			player->GetGameScene()->DoAsync([player, rewardExp]() {
				player->AddExp(rewardExp);
			});
		}

		ZoneRef zone = GWorld->GetZoneByPos(target->GetPos());
		DoTimer(DEAD_TICK, [monster]() {
			GWorld->LeaveCreature(monster);
		}); 
		DoTimer(monster->GetRespawnTick(), [monster]() {
			monster->Reset();
			GWorld->EnterCreature(monster);
		});
	}
}

void GameScene::FindNearestPlayer(Vector<ZoneRef> zones, MonsterRef monster, Vector3 monsterPos)
{
	if (zones.size() == 0 || monster == nullptr) return;

	PlayerRef bestPlayer = nullptr;
	float minDistSq = monster->GetSearchRangeSq();

	for (ZoneRef zone : zones)
	{
		for (auto& item : zone->_players)
		{
			PlayerRef player = item.second;
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
		monster->GetGameScene()->DoAsync([monster, bestPlayer, minDistSq]() {
			monster->HandleGatherResult(bestPlayer, minDistSq);
		}); 
	}
}

void GameScene::BroadcastToZone(ZoneRef zone, SendBufferRef sendBuffer, uint64 exceptId)
{
	if (zone == nullptr || sendBuffer == nullptr) return;

	for (auto& item : zone->_players)
	{
		PlayerRef player = item.second;
		if (player->GetObjectId() == exceptId) continue;

		player->Send(sendBuffer);
	}
}

void GameScene::BroadcastToZones(Vector<ZoneRef> zones, SendBufferRef sendBuffer, uint64 exceptId)
{
	Map<GameSceneRef, Vector<ZoneRef>> sceneGroups;
	for (ZoneRef zone : zones)
	{
		if (GameSceneRef scene = zone->GetScene())
			sceneGroups[scene].push_back(zone);
	}

	for (auto& item : sceneGroups)
	{
		GameSceneRef scene = item.first;
		Vector<ZoneRef> groupZones = item.second;
		if (scene.get() == this)
		{
			for (ZoneRef zone : groupZones)
				BroadcastToZone(zone, sendBuffer, exceptId);
		}
		else
		{
			scene->DoAsync([scene, groupZones, sendBuffer, exceptId]() {
				for (ZoneRef zone : groupZones)
					scene->BroadcastToZone(zone, sendBuffer, exceptId);
			});
		}
	}
}

void GameScene::BroadcastAround(Vector3 pos, SendBufferRef sendBuffer, uint64 exceptId)
{
	auto adjacentZones = GWorld->GetAdjacentZones(pos);
	BroadcastToZones(adjacentZones, sendBuffer, exceptId);
}
#include "pch.h"
#include "World.h"
#include "GameScene.h"
#include "Zone.h"
#include "GameSession.h"
#include "Player.h"
#include "Monster.h"
#include "DataManager.h"
#include "ClientPacketHandler.h"
#include "RedisManager.h"
#include "ConfigManager.h"

shared_ptr<World> GWorld = make_shared<World>();

void World::Init()
{
	_config = GConfigManager->GetWorld();
	CreateScene();
	CreateZone();
	CreateMoveTable();
}

void World::Start()
{
	SpawnMonsters();
	for (auto& scene : _scenes)
		scene->DoAsync(&GameScene::Update);
}

void World::CreateScene()
{
	int32 totalSceneCount = _config.sceneCount * _config.sceneCount;

	// 컨테이너 크기를 미리 확보하여 재할당 오버헤드 방지
	_scenes.resize(totalSceneCount);
	for (int32 i = 0; i < totalSceneCount; ++i)
		_scenes[i] = MakeShared<GameScene>();
}

void World::CreateZone()
{
	int32 zoneCount = _config.mapSize / _config.zoneSize;
	int32 totalZoneCount = zoneCount * zoneCount;
	int32 sceneCount = _config.sceneCount;

	// 컨테이너 크기를 미리 확보하여 재할당 오버헤드 방지
	_zones.reserve(totalZoneCount);
	for (int32 z = 0; z < zoneCount; ++z)
	{
		for (int32 x = 0; x < zoneCount; ++x)
		{
			ZoneRef zone = MakeShared<Zone>(x, z);
			_zones.push_back(zone);

			int32 xIdx = x / (zoneCount / sceneCount);  // 0 or 1
			int32 zIdx = z / (zoneCount / sceneCount);  // 0 or 1

			// 경계값 처리
			xIdx = min(xIdx, sceneCount - 1);
			zIdx = min(zIdx, sceneCount - 1);

			int32 sceneIdx = (zIdx * sceneCount) + xIdx;
			_scenes[sceneIdx]->AddZone(zone);
			zone->SetScene(_scenes[sceneIdx]);
		}
	}
}

void World::CreateMoveTable()
{
	int32 zoneCount = _config.mapSize / _config.zoneSize;
	int32 totalZoneCount = zoneCount * zoneCount;

	_moveTable.assign(totalZoneCount, Vector<MoveResultRef>(totalZoneCount, nullptr));
	for (int32 oldId = 0; oldId < totalZoneCount; ++oldId)
	{
		for (int32 newId = 0; newId < totalZoneCount; ++newId)
		{
			if (oldId == newId) continue;

			// 인접해있는 Zone만
			if (IsTooFar(oldId, newId)) continue;

			ZoneRef oldZone = GetZoneById(oldId);
			ZoneRef newZone = GetZoneById(newId);
			if (oldZone == nullptr || newZone == nullptr) continue;

			_moveTable[oldId][newId] = GetMoveResult(oldZone, newZone);
		}
	}
}

MoveResultRef World::GetCalculateMoveResult(ZoneRef oldZone, ZoneRef newZone)
{
	Vector<ZoneRef> oldAdjacentZones = GetAdjacentZones(oldZone->GetPos());
	Vector<ZoneRef> newAdjacentZones = GetAdjacentZones(newZone->GetPos());

	// 투 포인터 알고리즘을 위한 정렬
	auto compare = [](const ZoneRef& a, const ZoneRef& b) { return a->GetId() < b->GetId(); };
	std::sort(oldAdjacentZones.begin(), oldAdjacentZones.end(), compare);
	std::sort(newAdjacentZones.begin(), newAdjacentZones.end(), compare);

	MoveResultRef result = MakeShared<MoveResult>();
	auto itOld = oldAdjacentZones.begin();
	auto itNew = newAdjacentZones.begin();

	while (itOld != oldAdjacentZones.end() && itNew != newAdjacentZones.end())
	{
		int32 oldAdjId = (*itOld)->GetId();
		int32 newAdjId = (*itNew)->GetId();

		if (oldAdjId < newAdjId)
			result->leaveZones.push_back(*itOld++);
		else if (oldAdjId > newAdjId)
			result->enterZones.push_back(*itNew++);
		else
		{
			result->keepZones.push_back(*itOld);
			++itOld;
			++itNew;
		}
	}

	// 한쪽 리스트가 먼저 끝난 경우 남은 항목 정리
	while (itOld != oldAdjacentZones.end()) result->leaveZones.push_back(*itOld++);
	while (itNew != newAdjacentZones.end()) result->enterZones.push_back(*itNew++);

	return result;
}

MoveResultRef World::GetMoveResult(ZoneRef oldZone, ZoneRef newZone)
{
	if (oldZone == nullptr || newZone == nullptr) return nullptr;

	int32 oldId = oldZone->GetId();
	int32 newId = newZone->GetId();

	if (oldId < _moveTable.size() && newId < _moveTable.size())
	{
		if (_moveTable[oldId][newId])
			return _moveTable[oldId][newId];
	}

	return GetCalculateMoveResult(oldZone, newZone);
}

bool World::IsTooFar(int32 oldId, int32 newId)
{
	int32 zoneCount = _config.mapSize / _config.zoneSize;

	int32 oldX = oldId % zoneCount;
	int32 oldZ = oldId / zoneCount;

	int32 newX = newId % zoneCount;
	int32 newZ = newId / zoneCount;

	int32 diffX = std::abs(oldX - newX);
	int32 diffZ = std::abs(oldZ - newZ);

	return (diffX >= 2 || diffZ >= 2);
}

void World::SpawnMonsters()
{
	auto spawnList = GDataManager->GetMonsterSpawnList();

	for (const SpawnData& data : spawnList)
	{
		MonsterRef monster = MakeShared<Monster>();
		monster->Init(data);

		EnterCreature(monster);
	}
}

void World::PlayerEnterToGame(GameSessionRef session, SummaryDataRef summary, StatData stat)
{
	if (session == nullptr) return;

	PlayerRef player = MakeShared<Player>();
	player->Init(summary, stat);
	player->SetSession(session);
	session->SetPlayer(player);

	// Redis 저장 (Chat 서버 정보)
	GRedisManager->SetPlayerInfo(player->GetObjectId(), Utils::ws2s(player->GetName()), GConfigManager->GetGame().port);

	Protocol::S_EnterGame packet;
	auto* info = packet.mutable_myplayer();
	player->MakeObjectInfo(*info);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);
	session->Send(sendBuffer);
}

void World::EnterCreature(CreatureRef creature)
{
	if (creature == nullptr) return;
	ZoneRef mainZone = GetZoneByPos(creature->GetPos());
	GameSceneRef mainScene = mainZone->GetScene();
	creature->SetGameScene(mainScene);

	auto adjacentZones = GWorld->GetAdjacentZones(creature->GetPos());
	Map<GameSceneRef, Vector<ZoneRef>> sceneGroups;
	for (ZoneRef zone : adjacentZones)
	{
		if (GameSceneRef scene = zone->GetScene())
			sceneGroups[scene].push_back(zone);
	}

	for (auto& item : sceneGroups)
	{
		GameSceneRef scene = item.first;
		Vector<ZoneRef> zones = item.second;
		scene->DoAsync(&GameScene::HandleVisionEnter, creature, zones);
	}

	mainScene->DoAsync([mainZone, creature]() {
		mainZone->Enter(creature);
	});
}

void World::LeaveCreature(CreatureRef creature)
{
	if (creature == nullptr) return;

	ZoneRef mainZone = GetZoneByPos(creature->GetPos());
	GameSceneRef mainScene = mainZone->GetScene();

	auto adjacentZones = GWorld->GetAdjacentZones(creature->GetPos());
	Map<GameSceneRef, Vector<ZoneRef>> sceneGroups;
	for (ZoneRef zone : adjacentZones)
	{
		if (GameSceneRef scene = zone->GetScene())
			sceneGroups[scene].push_back(zone);
	}

	for (auto& item : sceneGroups)
	{
		GameSceneRef scene = item.first;
		Vector<ZoneRef> zones = item.second;
		scene->DoAsync(&GameScene::HandleVisionLeave, creature, zones);
	}

	mainScene->DoAsync([mainZone, creature]() {
		mainZone->Leave(creature);
	});
}

ZoneRef World::GetZoneByPos(Vector3 pos)
{
	int32 zoneCount = _config.mapSize / _config.zoneSize;

	int32 x = static_cast<int32>(pos.x / _config.zoneSize);
	int32 z = static_cast<int32>(pos.z / _config.zoneSize);

	if (x < 0 || x >= zoneCount || z < 0 || z >= zoneCount)
		return nullptr;

	int32 id = (z * zoneCount) + x;

	return _zones[id];
}

ZoneRef World::GetZoneById(int32 id)
{
	if (id < 0 || id >= _zones.size())
		return nullptr;

	return _zones[id];
}

Vector<ZoneRef> World::GetAdjacentZones(Vector3 pos)
{
	int32 zoneCount = _config.mapSize / _config.zoneSize;

	Vector<ZoneRef> adjacent;
	int32 centerX = static_cast<int32>(pos.x / _config.zoneSize);
	int32 centerZ = static_cast<int32>(pos.z / _config.zoneSize);

	for (int32 x = centerX - 1; x <= centerX + 1; ++x)
	{
		for (int32 z = centerZ - 1; z <= centerZ + 1; ++z)
		{
			if (x >= 0 && x < zoneCount && z >= 0 && z < zoneCount)
				adjacent.push_back(_zones[(z * zoneCount + x)]);
		}
	}

	return adjacent;
}



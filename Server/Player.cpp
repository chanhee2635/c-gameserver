#include "pch.h"
#include "Player.h"
#include "GameSession.h"
#include "ClientPacketHandler.h"
#include "IdGenerator.h"
#include "DataManager.h"
#include "World.h"
#include "Zone.h"
#include "GameScene.h"
#include "NavigationManager.h"
#include "DBManager.h"

void Player::Init(SummaryDataRef summary, StatData& stat)
{
	// DB ID -> Memory ID
	_playerDbId = summary->objectId;
	// summary : objectid, name, level, templateId, objectType
	_summary = summary;
	_summary->objectId = IdGenerator::GenerateId(_summary->objectType);
	_stats = stat;
	_stats.pos.y = GNavigationManager->GetHeight(stat.pos);
	_config = GDataManager->GetPlayerData(summary->templateId, summary->level);

	_maxCombo = GDataManager->GetMaxCombo(_summary->templateId);
}

bool Player::InAttackRange(CreatureRef target)
{
	if (target == nullptr || target->IsDead()) return false;

	Vector3 targetPos = target->GetPos();
	Vector3 dir = targetPos - GetPos();
	float distSq = dir.LengthSquared();

	if (distSq > GetAttackRangeSq()) 
		return false;

	dir.Normalize();
	float dot = dir.Dot(GetForward());
	float cosThreshold = cosf((GetAttackAngle() / 2.0f) * (3.141592f / 180.0f));

	return dot >= cosThreshold;
}

void Player::AddExp(int32 rewardExp)
{
	int32 initLvl = _summary->level;
	_stats.exp += rewardExp;

	while (true)
	{
		int64 nextLevelExp = GDataManager->GetPlayerRequireExp(_summary->templateId, _summary->level + 1);
		if (_stats.exp < nextLevelExp) break;

		_summary->level++;
		ApplyLevelUpStatus();

		Protocol::S_ChangeLevel packet;
		packet.set_objectid(_summary->objectId);
		packet.set_level(_summary->level);
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);
		GetGameScene()->BroadcastAround(_stats.pos, sendBuffer, _summary->objectId);
	}

	if (_summary->level > initLvl)
		GDBManager->DoAsyncPush(&DBManager::SavePlayerLevelUp, _playerDbId, _summary->level, _stats);

	Protocol::S_ChangeExp packet;
	packet.set_level(_summary->level);
	packet.set_exp(_stats.exp);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);
	Send(sendBuffer);
}

void Player::ApplyLevelUpStatus()
{
	const PlayerData* config = GDataManager->GetPlayerData(_summary->templateId, _summary->level);
	if (config != nullptr)
	{
		_config = config;
		_stats.hp = config->maxHp;
		_stats.mp = config->maxMp;
	}
}

bool Player::CheckComboIndex(int32 comboIndex)
{
	return 0 < comboIndex && _maxCombo >= comboIndex;
}

void Player::SetRevive()
{
	SetState(CreatureState::Idle);
	SetHp(GetMaxHp());
}

void Player::HandleAttack(float yaw, int32 comboIndex)
{
	uint64 currentTick = GetTickCount64();
	if (!CanAttack(comboIndex, currentTick)) return;

	SetYaw(yaw);
	SetAttackTick(currentTick);

	Protocol::S_Attack packet;
	packet.set_objectid(GetObjectId());
	packet.set_yaw(GetYaw());
	packet.set_comboindex(comboIndex);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);

	auto adjacentZones = GWorld->GetAdjacentZones(GetPos());
	GetGameScene()->BroadcastToZones(adjacentZones, sendBuffer, GetObjectId());

	// 씬 별로 몬스터 공격
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
		scene->DoAsync(&GameScene::PlayerAttack, static_pointer_cast<Player>(shared_from_this()), zones);
	}
}

void Player::Send(SendBufferRef sendBuffer)
{
	if (auto session = _session.lock())
		session->Send(sendBuffer);
}
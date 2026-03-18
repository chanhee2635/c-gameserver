#include "pch.h"
#include "Player.h"
#include "GameSession.h"
#include "IdGenerator.h"
#include "DataManager.h"
#include "GameScene.h"
#include "NavigationManager.h"
#include "ClientPacketHandler.h"
#include "DBManager.h"
#include "ConfigManager.h"
#include "World.h"

void Player::Init(const PlayerSummaryData& summary, const PlayerLoadData& loadData)
{
	_playerDbId	= summary.dbId;
	_objectId	= IdGenerator::GenerateId(GameObjectType::Player);
	_objectType	= GameObjectType::Player;
	_name		= summary.name;
	_level		= summary.level;
	_templateId = summary.templateId;

	_config = GDataManager->GetPlayerData(_templateId, _level);
	ASSERT_CRASH(_config != nullptr);

	_maxCombo = GDataManager->GetMaxCombo(_templateId);
	_speed	  = _config->speed;

	_pos = Vector3(loadData.pos.x, GNavigationManager->GetHeight(loadData.pos), loadData.pos.z);
	_yaw = loadData.yaw;
	_hp = loadData.hp;
	_mp = loadData.mp;
	_exp = loadData.exp;
	_state = CreatureState::Idle;
}

void Player::HandleMoveJob(const MoveJobRef& job)
{
	if (job == nullptr) return;

	Vector3 oldPos = _pos;
	CreatureState oldState = _state;

	_pos = job->pos;
	_state = job->state;
	_yaw = job->yaw;

	bool stateChanged = job->state != oldState;
	bool posChanged = Vector3::DistanceSquared(oldPos, _pos) > 0.0001f;

	if (stateChanged || posChanged)
		_isDirty = true;
}

void Player::HandleAttack(float yaw, int32 comboIndex, Vector3 clientPos)
{
	if (IsDead()) return;
	if (comboIndex < 1 || comboIndex > _maxCombo) return;

	float distSq = Vector3::DistanceSquared(clientPos, _pos);
	Vector3 attackPos = (distSq <= ATTACK_POS_TOLERANCE_SQ) ? clientPos : _pos;

	_yaw = yaw;
	_pos = attackPos;
	_state = CreatureState::Attack;

	GameSceneRef scene = GetGameScene();
	if (scene == nullptr) return;

	ZoneRef zone = GetZone();
	if (zone == nullptr) return;

	Protocol::S_Attack packet;
	packet.set_object_id(_objectId);
	packet.set_yaw(yaw);
	packet.set_combo_index(comboIndex);
	*packet.mutable_pos() = GameUtil::ToProto(attackPos);
	scene->BroadcastToAdjacentZones(zone, ClientPacketHandler::MakeSendBuffer(packet));

	uint64 hitDelay = GetHitDelay(comboIndex);
	auto self = static_pointer_cast<Player>(shared_from_this());

	scene->DoTimer(hitDelay, [scene, self, attackPos, yaw]() {
		if (self->IsDead()) return;
		scene->HandleAttackHitDetection(self, attackPos, yaw);
	});
}

void Player::Send(SendBufferRef sendBuffer)
{
	if (auto session = _session.lock())
		session->Send(sendBuffer);
}

void Player::Revive(Vector3 pos)
{
	_hp = _config ? _config->maxHp : 1;
	_state = CreatureState::Idle;
	_pos = pos;
	_yaw = 0.f;
	_isDirty = true;
}

void Player::GainExp(int64 rewardExp)
{
	if (IsDead() || rewardExp <= 0) return;

	_exp += rewardExp;

	Protocol::S_ChangeExp expPkt;
	expPkt.set_object_id(_objectId);
	expPkt.set_exp(_exp);
	Send(ClientPacketHandler::MakeSendBuffer(expPkt));

	TryLevelUp();
}

void Player::OnDead()
{
}

void Player::MakeStatInfo(Protocol::StatInfo& info) const
{
	info.set_hp(_hp);
	info.set_mp(_mp);
	info.set_exp(_exp);
}

void Player::TryLevelUp()
{
	int64 reqExp = GDataManager->GetPlayerRequireExp(_templateId, _level);
	if (_exp < reqExp) return;

	// 다음 레벨 데이터 존재 여부 확인
	const PlayerData* nextConfig = GDataManager->GetPlayerData(_templateId, _level + 1);
	if (nextConfig == nullptr) return;   // 최대 레벨

	// 레벨업 처리
	_exp -= reqExp;
	_level += 1;
	_config = nextConfig;
	_speed = _config->speed;
	_hp = _config->maxHp;   // HP 풀 회복
	_mp = _config->maxMp;
	_isDirty = true;

	GameSceneRef scene = GetGameScene();
	ZoneRef zone = GetZone();

	if (scene && zone)
	{
		Protocol::S_ChangeLevel broadcastPkt;
		broadcastPkt.set_object_id(_objectId);
		broadcastPkt.set_level(_level);
		broadcastPkt.set_max_hp(_config->maxHp);
		broadcastPkt.set_hp(_hp);
		scene->BroadcastToAdjacentZones(zone,
			ClientPacketHandler::MakeSendBuffer(broadcastPkt));
	}

	Protocol::S_ChangeLevel lvPkt;
	lvPkt.set_object_id(_objectId);
	lvPkt.set_level(_level);
	lvPkt.set_max_hp(_config->maxHp);
	lvPkt.set_hp(_hp);
	lvPkt.set_exp(_exp);
	Send(ClientPacketHandler::MakeSendBuffer(lvPkt));

	uint64  dbId = _playerDbId;
	int32   level = _level;
	int32   hp = _hp;
	int32   mp = _mp;
	int64   exp = _exp;
	Vector3 pos = _pos;
	float   yaw = _yaw;

	GDBManager->DoAsync([dbId, level, hp, mp, exp, pos, yaw]() {
		GDBManager->SavePlayerLevelUp(dbId, level, hp, mp, exp, pos, yaw);
	});

	TryLevelUp();
}

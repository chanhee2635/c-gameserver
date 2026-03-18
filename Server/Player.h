#pragma once
#include "Creature.h"

class Player : public Creature
{
public:
	Player() = default;
	virtual ~Player() = default;

	void Init(const PlayerSummaryData& summary, const PlayerLoadData& loadData);
	void HandleMoveJob(const MoveJobRef& job);
	void HandleAttack(float yaw, int32 comboIndex, Vector3 clientPos);

	uint64 GetPlayerDbId() { return _playerDbId; }
	void SetSession(GameSessionRef session) { _session = session; }
	void Send(SendBufferRef sendBuffer);

	void Revive(Vector3 pos);
	void SetPos(Vector3 pos) { _pos = pos; }
	void SetYaw(float yaw) { _yaw = yaw; }
	float GetYaw()	  const { return _yaw; }
	int32 GetMaxHp()  const { return _config ? _config->maxHp : 0; }
	int32 GetMp()	  const { return _mp; }
	int64 GetExp()    const { return _exp; }
	int32 GetAttack() const { return _config ? _config->attack : 0; }
	float GetAttackRange() const { return _config ? _config->attackRange : 0.f; }
	float GetAttackAngle() const { return _config ? _config->attackAngle : 0.f; }
	void GainExp(int64 rewardExp);
	virtual float GetAttackSpeed() const override { return _config ? _config->attackSpeed : 1.0f; }
	virtual int32 GetDefense() const override { return _config ? _config->defense : 0; }
	virtual void OnDead() override;

	void MakeStatInfo(Protocol::StatInfo& info) const override;

private:
	void TryLevelUp();

	uint64	_playerDbId = 0;
	int32	_mp = 0;
	int64	_exp = 0;
	const PlayerData* _config = nullptr;
	weak_ptr<GameSession>	_session;

	static constexpr float ATTACK_POS_TOLERANCE_SQ = 9.0f;
};
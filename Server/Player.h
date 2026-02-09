#pragma once
#include "Creature.h"

class Player : public Creature
{
public:
	Player() {};
	virtual ~Player() = default;

	void Init(SummaryDataRef summary, StatData& stat);

	uint64 GetPlayerDbId() { return _playerDbId; }
	const PlayerData* GetConfig() { return _config; }
	GameSessionRef GetSession() { return _session.lock(); }
	void SetSession(GameSessionRef session) { _session = session; }
	void SetLastMoveTime(float time) { _lastMoveTime = time; }
	int32 GetMaxHp() { return _config->maxHp; }
	int32 GetDamage() override { return _config->attack; }
	int32 GetDefense() override{ return _config->defense; }
	double GetSpeed() { return _config->speed; }
	float GetLastMoveTime() { return _lastMoveTime; }
	float GetAttackRangeSq() { return _config->attackRange * _config->attackRange; }
	uint64 GetAttackTick() { return static_cast<uint64>(_config->attackSpeed * 1000); }
	float GetAttackAngle() { return _config->attackAngle; }
	bool InAttackRange(CreatureRef target);
	void AddExp(int32 rewardExp);
	void ApplyLevelUpStatus();
	bool CheckComboIndex(int32 comboIndex);
	void SetRevive();

	void HandleAttack(float yaw, int32 comboIndex);

	void Send(SendBufferRef sendBuffer);

private:
	const PlayerData* _config = nullptr;

	float _lastMoveTime = 0.0f;

	uint64 _playerDbId = 0;
	weak_ptr<GameSession>	_session;
};


#pragma once
#include "Creature.h"

class Monster : public Creature
{
public:
	Monster() {};
	virtual ~Monster() = default;

	void Init(const SpawnData& data);
	void Reset();
	virtual void Update();

	const MonsterData* GetConfig() { return _config; }
	uint64 GetRespawnTick() { return _respawnTick; }
	int32 GetMaxHp() { return _config->maxHp; }
	int32 GetDamage() override { return _config->attack; }
	int32 GetDefense() override { return _config->defense; }
	double GetSpeed() { return _config->speed; }
	float GetSearchRangeSq() { return _config->searchRange * _config->searchRange; }
	float GetMaxSearchRangeSq() { return _config->maxSearchRange * _config->maxSearchRange; }
	float GetAttackRangeSq() { return _config->attackRange * _config->attackRange; }
	uint64 GetAttackTick() { return static_cast<uint64>(_config->attackSpeed * 1000); }
	PlayerRef GetTargetPlayer() { return _target.lock(); }
	//void OnDead() override;

	void HandleGatherResult(PlayerRef player, float distSq);

protected:
	virtual void UpdateIdle();
	virtual void UpdateMoving();
	virtual void UpdateReturn();
	virtual void UpdateAttack();
	virtual void UpdateDead();

	void TickMoveTo(Vector3 targetPos);

private:
	const MonsterData* _config = nullptr;

	Vector<Vector3> _path;
	int32 _pathIndex = 0;
	Vector3 _lastTargetPos;

	float _lastTargetDistSq = 0.0f;
	uint64 _nextSearchTick = 0;
	uint64 _respawnTick = 0;
	Vector3 _spawnPos;

	weak_ptr<Player> _target;
};


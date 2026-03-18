#pragma once
#include "Creature.h"

class Monster : public Creature
{
public:
	Monster() = default;
	virtual ~Monster() = default;

	void Init(const SpawnData& data);
	void Reset();
	virtual void Update();
	void HandleGatherResult(PlayerRef player, float distSq);

	int32 GetMaxHp()			const { return _config->maxHp; }
	int64 GetRewardExp()		const { return _config->rewardExp; }
	float GetSearchRangeSq()	const { return _config->searchRange * _config->searchRange; }
	float GetMaxSearchRangeSq() const { return _config->maxSearchRange * _config->maxSearchRange; }
	float GetAttackRangeSq()	const { return _config->attackRange * _config->attackRange; }
	virtual float GetAttackSpeed() const override { return _config ? _config->attackSpeed : 1.0f; }
	virtual int32 GetDefense() const override { return _config ? _config->defense : 0; }
	virtual void OnDead() override;


	PlayerRef GetTargetPlayer() const { return _target.lock(); }
	

protected:
	virtual void UpdateIdle();
	virtual void UpdateMoving();
	virtual void UpdateReturn();
	virtual void UpdateAttack();
	virtual void UpdateDead();
	void TickMoveTo(Vector3 targetPos);

private:
	void ExecuteAttack(int64 now, PlayerRef target);
	void ApplyAttackDamage(PlayerRef target);

	const MonsterData* _config = nullptr;
	uint64 _respawnTick = 10000;

	Vector<Vector3> _path;
	int32 _pathIndex = 0;
	Vector3 _lastTargetPos;
	float _lastTargetDistSq = 0.0f;
	int64 _nextSearchTick = 0;
	int64 _nextAttackTick = 0;
	Vector3 _spawnPos;
	float _deltaTime = 0.0f;

	weak_ptr<Player> _target;
};


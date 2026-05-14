#pragma once
#include "Creature.h"
#include "Struct.h"

class Monster : public Creature
{
public:
    Monster() = default;
    virtual ~Monster() = default;

    void Init(const SpawnData& data);
    void Reset();
    virtual void Update(float deltaTime = 0.05f);
    void HandleGatherResult(PlayerRef player, float distSq);

    int64  GetRewardExp()        const { return _config ? _config->rewardExp : 0; }
    uint64 GetRespawnDelayMs()   const { return _respawnDelayMs; }
    float  GetSearchRangeSq()    const { return _config ? _config->searchRange * _config->searchRange : 0.f; }
    float  GetMaxSearchRangeSq() const { return _config ? _config->maxSearchRange * _config->maxSearchRange : 0.f; }
    float  GetAttackRangeSq()    const { return _config ? _config->attackRange * _config->attackRange : 0.f; }

    virtual float GetAttackSpeed() const override { return _config ? _config->attackSpeed : 1.0f; }
    virtual int32 GetDefense()     const override { return _config ? _config->defense : 0; }
    virtual int32 GetAttack()      const override { return _config ? _config->attack : 0; }

    PlayerRef GetTargetPlayer() const { return _target.lock(); }

protected:
    virtual void OnDead() override;

    virtual void UpdateIdle();
    virtual void UpdateMoving();
    virtual void UpdateReturn();
    virtual void UpdateAttack();
    virtual void UpdateDead();

    void TickMoveTo(Vector3 targetPos);

private:
    void ExecuteAttack(int64 now, PlayerRef target);
    void ApplyAttackDamage(PlayerRef target);
    void ResetPath() { _path.clear(); _pathIndex = 0; }

    const MonsterData* _config = nullptr;

    Vector3 _spawnPos;
    Vector3 _lastTargetPos;
    float   _lastTargetDistSq = 0.f;
    float   _deltaTime = 0.05f;  
    int64   _nowTick = 0;

    Vector<Vector3> _path;
    int32  _pathIndex = 0;

    int64  _nextUpdateTick = 0;
    int64  _searchTick = 0;     
    int64  _nextSearchTick = 0;
    int64  _nextAttackTick = 0;
    int64  _nextFindPathTick = 0;
    uint64 _respawnDelayMs = 10000;

    static constexpr float ARRIVE_DIST_SQ = 0.04f;  

    PlayerWeakRef _target;
};
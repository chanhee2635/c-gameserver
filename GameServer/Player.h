#pragma once
#include "Creature.h"

class Player : public Creature
{
public:
    Player() = default;
    virtual ~Player() = default;

    void Init(const PlayerSummaryData& summary, const PlayerLoadData& loadData);

    void           SetSession(GameSessionRef session) { _session = session; }
    GameSessionRef GetSession() { return _session.lock(); }
    void           Send(SendBufferRef sendBuffer);

    uint64 GetPlayerDbId() const { return _playerDbId; }

    bool IsSaveDirty()    const { return _isSaveDirty; }
    void ClearSaveDirty()       { _isSaveDirty = false; }

    int32 GetMp()    const { return _mp; }
    int64 GetExp()   const { return _exp; }
    int32 GetMaxMp() const { return _config ? _config->maxMp : 0; }
    float GetAttackRange() const { return _config ? _config->attackRange : 0.f; }
    float GetAttackAngle() const { return _config ? _config->attackAngle : 0.f; }

    virtual int32 GetAttack()      const override { return _config ? _config->attack : 0; }
    virtual int32 GetDefense()     const override { return _config ? _config->defense : 0; }
    virtual float GetAttackSpeed() const override { return _config ? _config->attackSpeed : 1.f; }
    virtual float GetMoveSpeed()   const override { return _config ? _config->speed : 0.f; }

    void HandleMoveJob(const MoveJob& job);
    void HandleAttack(float yaw, int32 comboIndex, Vector3 clientPos);
    void GainExp(int64 rewardExp);
    void Revive(Vector3 pos);

    void MakeStatInfo(Protocol::StatInfo& info) const override;

protected:
    virtual void OnDead() override;

private:
    void TryLevelUp();

    uint64  _playerDbId  = 0;
    int32   _mp          = 0;
    int64   _exp         = 0;
    int32   _maxCombo    = 0;
    bool    _isSaveDirty = false;

    const PlayerData* _config = nullptr;
    GameSessionWeakRef _session;

};

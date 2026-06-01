#pragma once
#include "GameObject.h"
#include "Zone.h"
#include "Struct.h"

class Creature : public GameObject
{
public:
    Creature() = default;
    virtual ~Creature() = default;

    Protocol::CreatureState GetState() const { return _state; }
    void                    SetState(Protocol::CreatureState state) { _state = state; }
    bool                    IsDead()  const { return _state == Protocol::DEAD; }

    int32 GetHp()    const { return _hp; }
    int32 GetMaxHp() const { return _maxHp; }
    int32 TakeDamage(int32 incomingAttack);

    ZoneRef GetZone() { return _zone.lock(); }
    void    SetZone(ZoneRef zone) { _zone = zone; }

    // Register/unregister into the scene's per-type container (_players/_monsters).
    // Derived types cast themselves and call the matching GameScene method.
    virtual void AddToScene(GameScene* scene) = 0;
    virtual void RemoveFromScene(GameScene* scene) = 0;

    virtual int32 GetAttack()      const { return 0; }
    virtual int32 GetDefense()     const { return 0; }
    virtual float GetAttackSpeed() const { return 1.0f; }

    void           SetVelocity(const Vector3& v) { _velocity = v; }
    const Vector3& GetVelocity() const { return _velocity; }

    uint64 GetHitDelay(int32 comboIndex) const;

    bool    ShouldBroadcastMove(uint64 nowMs) const;
    Vector3 PredictPos(uint64 nowMs) const;
    void    CommitMoveAnchor(uint64 nowMs);

    void MakePosInfo(Protocol::PosInfo& info)   const override;
    void MakeStatInfo(Protocol::StatInfo& info) const override;

protected:
    const PrefabData* _prefabData = nullptr;
    Protocol::CreatureState _state = Protocol::IDLE;
    int32   _hp = 0;
    int32   _maxHp = 0;
    float   _speed = 0.f;
    Vector3 _velocity{};

    Vector3                 _lastSentPos{};
    Vector3                 _lastSentVelocity{};
    Protocol::CreatureState _lastSentState = Protocol::IDLE;
    uint64                  _lastSentTick = 0;

    ZoneWeakRef _zone;
};
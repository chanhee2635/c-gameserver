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

    bool IsDirty()           const { return _isDirty; }
    void SetDirty(bool flag) { _isDirty = flag; }

    virtual int32 GetAttack()      const { return 0; }
    virtual int32 GetDefense()     const { return 0; }
    virtual float GetAttackSpeed() const { return 1.0f; }
    virtual float GetMoveSpeed()   const { return _speed; }

    uint64 GetHitDelay(int32 comboIndex) const;

    void MakePosInfo(Protocol::PosInfo& info)   const override;
    void MakeStatInfo(Protocol::StatInfo& info) const override;

protected:
    virtual void OnDead() {}

    const PrefabData* _prefabData = nullptr;
    Protocol::CreatureState _state = Protocol::IDLE;
    int32   _hp = 0;
    int32   _maxHp = 0;
    float   _speed = 0.f;
    bool    _isDirty = false;

    ZoneWeakRef _zone;
};
#pragma once
#include "Protocol.pb.h"

// ─── DB 데이터 전송 구조체 ────────────────────────────────────────────────────

struct PlayerSummaryData
{
    uint64  dbId       = 0;
    wstring name;
    int32   templateId = 0;
    int32   level      = 1;
};

struct PlayerLoadData
{
    Vector3 pos;
    float   yaw = 0.f;
    int32   hp  = 0;
    int32   mp  = 0;
    int64   exp = 0;
};

// ─── 게임 데이터 테이블 구조체 ────────────────────────────────────────────────

struct MonsterData
{
    int32   id            = 0;
    int32   level         = 1;
    wstring name;
    int32   maxHp         = 0;
    int32   attack        = 0;
    int32   defense       = 0;
    int64   rewardExp     = 0;
    float   speed         = 0.f;
    float   searchRange   = 0.f;
    float   maxSearchRange = 0.f;
    float   attackRange   = 0.f;
    float   attackSpeed   = 0.f;
};

struct PlayerData
{
    int32 id          = 0;
    int32 level       = 1;
    int32 maxHp       = 200;
    int32 maxMp       = 100;
    int32 attack      = 10;
    int32 defense     = 5;
    int64 reqExp      = 100;
    float speed       = 5.f;
    float attackRange = 2.f;
    float attackSpeed = 1.f;
    float attackAngle = 90.f;
};

struct PrefabData
{
    int32              id             = 0;
    wstring            name;
    int32              maxCombo       = 0;
    Vector<int32>      comboHitDelays;
    int32              deathDurationMs = 0;
    int32              poolSize        = 0;
};

struct SpawnData
{
    int32   id          = 0;
    Vector3 pos;
    float   yaw         = 0.f;
    uint64  respawnDelayMs = 0;
};

struct MoveJob
{
    uint64                  objectId = 0;
    Vector3                 pos;
    Vector3                 velocity;
    float                   yaw = 0.f;
    Protocol::CreatureState state = Protocol::IDLE;
    uint64                  sendServerTimeMs = 0;  
};
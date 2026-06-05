#pragma once
#include <random>
#include "JobQueue.h"
#include "DummyTypes.h"

struct AgentState
{
    DummySessionWeakRef session;
    Vector3             pos = {};
    Vector3             target = {};
    float               speed = 2.0f;
    int32               chatSeq = 0;
    std::chrono::steady_clock::time_point nextMoveTime;
    std::chrono::steady_clock::time_point nextChatTime;
};

class DummyGroup : public JobQueue
{
public:
    void  Tick();                                              
    void  AddAgent(DummySessionRef session, Vector3 spawnPos);  
    void  RemoveSome(int32 n);                                  
    void  Stop() { _stopped.store(true, std::memory_order_relaxed); }
    int32 Count() const { return _count.load(std::memory_order_relaxed); }

    static constexpr uint32 TICK_MS = 100;   // 에이전트 이동 주기(ms)

private:
    Vector3 RandomMapPoint();
    uint32  RandomChatDelayMs();
    bool    RollWorldChat();

    Vector<AgentState> _agents;
    std::mt19937       _rng{ std::random_device{}() };
    Atomic<int32>      _count = 0;
    Atomic<bool>       _stopped = false;

    static constexpr uint32 GROUP_TICK_MS = 10;     // 그룹 깨우는 간격(ms)
    static constexpr float  MOVE_SPEED = 4.0f;
    static constexpr float  MAP_MIN = 10.f;
    static constexpr float  MAP_MAX = 990.f;
    static constexpr float  ARRIVE_DIST_SQ = 1.0f;
    static constexpr uint32 CHAT_MIN_MS = 50000;
    static constexpr uint32 CHAT_MAX_MS = 100000;
    static constexpr float  WORLD_CHAT_CHANCE = 0.01f;
};

class DummySimulator
{
public:
    void  Start();   // 그룹 생성 + 최초 예약 (전용 스레드 없음)
    void  Stop();    // 모든 그룹 정지
    void  AddSession(DummySessionRef session, Vector3 spawnPos);
    void  RemoveN(int32 n);
    int32 GetActiveCount();

private:
    Vector<std::shared_ptr<DummyGroup>> _groups;
    Atomic<uint32> _rr = 0;

    static constexpr int32 GROUP_COUNT = 16;   // 워커보다 넉넉히, 그룹당 Tick 짧게
};

extern DummySimulator* GDummySimulator;
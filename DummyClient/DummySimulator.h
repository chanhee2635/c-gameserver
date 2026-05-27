#pragma once
#include <random>

struct AgentState
{
    DummySessionWeakRef session;
    Vector3             pos    = {};   
    Vector3             target = {};   
    float               speed  = 2.0f;
    int32               chatSeq = 0;
    std::chrono::steady_clock::time_point nextMoveTime;
    std::chrono::steady_clock::time_point nextChatTime;
};

class DummySimulator
{
public:
    void Start();
    void Stop();
    void AddSession(DummySessionRef session, Vector3 spawnPos);

    int32 GetActiveCount();
    void  RemoveN(int32 n);

private:
    void Run();

    Vector3 RandomMapPoint();
    uint32  RandomChatDelayMs();
    bool    RollWorldChat();

    static constexpr uint32 TICK_MS    = 100;    
    static constexpr float  MOVE_SPEED = 6.0f; 

    static constexpr float  MAP_MIN        = 10.f;   
    static constexpr float  MAP_MAX        = 990.f;
    static constexpr float  ARRIVE_DIST_SQ = 1.0f;   

    static constexpr uint32 CHAT_MIN_MS = 50000;      
    static constexpr uint32 CHAT_MAX_MS = 100000;      
    static constexpr float  WORLD_CHAT_CHANCE = 0.01f; 

    static inline const std::chrono::steady_clock::time_point s_baseTime =
        std::chrono::steady_clock::now();

    Atomic<bool>       _running = false;
    Mutex              _lock;
    Vector<AgentState> _agents;
    std::mt19937       _rng{ std::random_device{}() };
};

extern DummySimulator* GDummySimulator;

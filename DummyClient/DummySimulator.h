#pragma once

struct AgentState
{
    DummySessionWeakRef session;
    Vector3             center = {};
    float               angle = 0.f;
    float               speed = 2.0f;
    std::chrono::steady_clock::time_point nextSendTime;  
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

    static constexpr uint32 TICK_MS = 50;
    static constexpr float  MOVE_RADIUS = 3.f;

    static inline const std::chrono::steady_clock::time_point s_baseTime =
        std::chrono::steady_clock::now();

    Atomic<bool>       _running = false;
    Mutex              _lock;
    Vector<AgentState> _agents;
};

extern DummySimulator* GDummySimulator;
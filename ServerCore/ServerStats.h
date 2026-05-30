#pragma once
#include <atomic>
#include <mutex>
#include <vector>
#include <chrono>

struct NetworkStats {
    Atomic<uint64> recvBytes   = 0;
    Atomic<uint64> sendBytes   = 0;
    Atomic<uint64> recvPackets = 0;
    Atomic<uint64> sendPackets = 0;
    Atomic<uint64> recvBacklogBytes = 0;
    Atomic<uint32> sendQueueMaxDepth = 0;
};

struct JobStats {
    Atomic<uint64> jobsExecuted = 0;
};

struct IocpStats {
    Atomic<uint64> iocpCallCount      = 0;
    Atomic<uint64> totalProcessTimeUs = 0;
};

struct MemoryStats {
    Atomic<uint64> poolHitCount       = 0;
    Atomic<uint64> poolMissCount      = 0;
    Atomic<int64>  liveAllocCount     = 0;
    Atomic<int32>  frameOverflowCount = 0;
};

struct DbStats {
    Atomic<uint64> queryCount   = 0;  
    Atomic<uint64> queryFailed  = 0;   
    Atomic<uint64> queryTotalUs = 0;   
};

struct GameMetrics {
    Atomic<int32>  connectedSessions  = 0;
    Atomic<uint64> totalConnections   = 0;
    Atomic<int32>  activePlayers      = 0;
    Atomic<int32>  peakPlayers        = 0;  
    Atomic<int32>  activeMonsters     = 0;
    Atomic<uint64> tickCount          = 0;   
    Atomic<uint64> suspiciousPackets  = 0;
    Atomic<uint64> tickLagTotalMs     = 0;  
    Atomic<uint64> tickLagCount       = 0;   
    Atomic<uint64> tickLagMaxMs       = 0;   

    std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
    uint64 GetUptime() const {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
    }
};

struct SystemStats {
    double cpuUsage = 0.0;
    size_t memUsageMB = 0;
    void Refresh();
};

class ServerStats
{
public:
    ServerStats() = default;
    ~ServerStats() { Shutdown(); }

    NetworkStats network;
    JobStats     job;
    IocpStats    iocp;
    MemoryStats  memory;
    DbStats      db;
    GameMetrics  game;
    SystemStats  system;

    void Init();
    void RunUpdateLoop();
    void Shutdown() { _running = false; }

private:
    void RenderReport();

    Atomic<bool> _running = false;
    static constexpr int32 STATS_PANEL_LINES = 10;
};
#pragma once
#include <atomic>
#include <mutex>
#include <vector>
#include <numeric>
#include <algorithm>

/*=======================================================================
  ServerStats
  - 인프라 계층 통계 (IOCP, Memory, RecvBuffer, Job, Latency, System)
  - 게임 로직 통계는 기존 GMetrics 가 담당
  - Report(): 콘솔 상단 고정 패널에 실시간 현황 표시
  - ReportLoadTest(): 부하 테스트용 Latency 보고서 출력
=======================================================================*/

// ── 순환 수신 버퍼 통계 ──────────────────────────────────────────────
struct RecvBufferStats
{
    std::atomic<uint64> linearizeCount  = 0;   // wrap 발생 시 임시 복사 횟수
    std::atomic<uint64> bufferFullCount = 0;   // 수신 버퍼 포화로 Disconnect 횟수
};

// ── 네트워크 I/O 통계 (바이트/패킷, 방향별) ─────────────────────────
struct NetworkStats
{
    std::atomic<uint64> recvBytes      = 0;
    std::atomic<uint64> sendBytes      = 0;
    std::atomic<uint64> recvPackets    = 0;
    std::atomic<uint64> recvBatchCount = 0;  // 한 번의 WSARecv 에 담긴 패킷 배치 수
    std::atomic<uint64> sendPackets    = 0;
    std::atomic<uint64> broadcastCount = 0;
    std::atomic<uint64> broadcastTotal = 0;  // 누적 수신자 수 (평균 계산용)
};

// ── 세션 연결 현황 ───────────────────────────────────────────────────
struct SessionStats
{
    std::atomic<int32>  currentActive     = 0;
    std::atomic<uint64> totalConnected    = 0;
    std::atomic<uint64> totalDisconnected = 0;
};

// ── IOCP 이벤트 처리 성능 ────────────────────────────────────────────
struct IocpStats
{
    std::atomic<uint64> iocpCallCount      = 0;   // 구간 처리 건수
    std::atomic<uint64> totalProcessTimeUs = 0;   // 구간 처리 총 시간(us)
    std::atomic<uint64> accumCallCount     = 0;   // 누적 처리 건수
    std::atomic<uint64> accumProcessTimeUs = 0;   // 누적 처리 총 시간(us)
};

// ── 메모리 풀 성능 ───────────────────────────────────────────────────
struct MemoryStats
{
    std::atomic<uint64> poolHitCount         = 0;   // TLS 캐시 적중
    std::atomic<uint64> poolMissCount        = 0;   // 글로벌 풀 접근
    std::atomic<uint64> allocBatchCount      = 0;   // 배치 할당 횟수
    std::atomic<uint64> fetchFromGlobalCount = 0;   // TLS → Global fetch
    std::atomic<uint64> returnToGlobalCount  = 0;   // TLS → Global return
    std::atomic<int64>  liveAllocCount       = 0;   // 현재 살아있는 블록 수
};

// ── Job 처리 성능 ────────────────────────────────────────────────────
struct JobStats
{
    std::atomic<uint64> jobsQueued   = 0;
    std::atomic<uint64> jobsExecuted = 0;
    std::atomic<uint64> timeSlices   = 0;
    std::atomic<uint64> timerFired   = 0;
};

// ── 부하 테스트용 지연 시간 통계 (P99, 손실율) ──────────────────────
struct LatencyStats
{
    mutable std::mutex  mtx;
    std::vector<int64>  samples;

    std::atomic<uint64> totalSent     = 0;
    std::atomic<uint64> totalReceived = 0;
    std::atomic<uint64> accumCount    = 0;
    std::atomic<int64>  accumSum      = 0;
    std::atomic<int64>  accumP99Max   = 0;

    void   Record(int64 us);
    int64  Avg()      const;
    int64  Max()      const;
    int64  P99()      const;
    uint64 Lost()     const;
    float  LossRate() const;
    void   Clear();
};

// ── 시스템 리소스 (CPU, 메모리) ──────────────────────────────────────
struct SystemStats
{
    double cpuUsage   = 0.0;
    size_t memUsageMB = 0;

    void Refresh();
};

/*------------------------------------------------------------------------
  ServerStats  (싱글톤)
------------------------------------------------------------------------*/
class ServerStats
{
public:
    static ServerStats& Get() { static ServerStats instance; return instance; }

    // ── 독립 카운터 그룹 ─────────────────────────────────────────────
    RecvBufferStats recvBuffer;
    NetworkStats    network;
    SessionStats    session;
    IocpStats       iocp;
    MemoryStats     memory;
    JobStats        job;
    SystemStats     system;
    LatencyStats    latency;

    /*
    * @brief 서버 시작 시 콘솔 상단 패널 라인 수 예약
    *        (Logger::ReserveStatsPanel 위임)
    */
    void Init();

    /*
    * @brief 콘솔 상단 고정 패널에 현황 표시
    *        GMetrics 의 게임 로직 카운터도 함께 렌더링
    */
    void Report();

    /*
    * @brief 부하 테스트 지연 시간 보고서 (LatencyStats 기반)
    */
    void ReportLoadTest();
};

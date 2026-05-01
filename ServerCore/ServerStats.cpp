#include "pch.h"
#include "ServerStats.h"
#include "Logger.h"
#include "GameMetrics.h"
#include <psapi.h>
#include <iomanip>
#pragma comment(lib, "psapi.lib")

// 콘솔 상단 Stats 패널 라인 수
// Report() 에서 출력하는 라인 수와 반드시 일치해야 함
static constexpr int STATS_PANEL_LINES = 11;

// ─────────────────────────────────────────────────────────────────────────────
// LatencyStats
// ─────────────────────────────────────────────────────────────────────────────

void LatencyStats::Record(int64 us)
{
    totalReceived++;
    accumCount++;
    accumSum += us;

    std::lock_guard<std::mutex> lock(mtx);
    samples.push_back(us);
}

int64 LatencyStats::Avg() const
{
    std::lock_guard<std::mutex> lock(mtx);
    if (samples.empty()) return 0;
    return std::accumulate(samples.begin(), samples.end(), int64(0))
        / static_cast<int64>(samples.size());
}

int64 LatencyStats::Max() const
{
    std::lock_guard<std::mutex> lock(mtx);
    if (samples.empty()) return 0;
    return *std::max_element(samples.begin(), samples.end());
}

int64 LatencyStats::P99() const
{
    std::lock_guard<std::mutex> lock(mtx);
    if (samples.empty()) return 0;
    std::vector<int64> sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    size_t idx = static_cast<size_t>(sorted.size() * 0.99);
    if (idx >= sorted.size()) idx = sorted.size() - 1;
    return sorted[idx];
}

uint64 LatencyStats::Lost() const
{
    uint64 sent = totalSent.load();
    uint64 recv = totalReceived.load();
    return sent > recv ? sent - recv : 0;
}

float LatencyStats::LossRate() const
{
    uint64 sent = totalSent.load();
    if (sent == 0) return 0.f;
    return 100.f * static_cast<float>(Lost()) / static_cast<float>(sent);
}

void LatencyStats::Clear()
{
    std::lock_guard<std::mutex> lock(mtx);
    samples.clear();
    totalSent     = 0;
    totalReceived = 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// SystemStats
// ─────────────────────────────────────────────────────────────────────────────

void SystemStats::Refresh()
{
    // 메모리 (Working Set)
    PROCESS_MEMORY_COUNTERS pmc = {};
    pmc.cb = sizeof(pmc);
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        memUsageMB = pmc.WorkingSetSize / (1024 * 1024);

    // CPU (두 측정점 delta 로 프로세스 사용률 계산)
    static ULARGE_INTEGER prevKernel{}, prevUser{}, prevWall{};

    FILETIME ftNow{}, ftCreation{}, ftExit{}, ftKernel{}, ftUser{};
    GetSystemTimeAsFileTime(&ftNow);
    GetProcessTimes(GetCurrentProcess(), &ftCreation, &ftExit, &ftKernel, &ftUser);

    ULARGE_INTEGER curKernel, curUser, curWall;
    curKernel.LowPart = ftKernel.dwLowDateTime; curKernel.HighPart = ftKernel.dwHighDateTime;
    curUser.LowPart   = ftUser.dwLowDateTime;   curUser.HighPart   = ftUser.dwHighDateTime;
    curWall.LowPart   = ftNow.dwLowDateTime;    curWall.HighPart   = ftNow.dwHighDateTime;

    if (prevWall.QuadPart != 0 && curWall.QuadPart > prevWall.QuadPart)
    {
        uint64 cpuTime  = (curKernel.QuadPart - prevKernel.QuadPart)
                        + (curUser.QuadPart   - prevUser.QuadPart);
        uint64 wallTime = curWall.QuadPart - prevWall.QuadPart;

        SYSTEM_INFO si{};
        GetSystemInfo(&si);
        cpuUsage = 100.0 * static_cast<double>(cpuTime)
                         / static_cast<double>(wallTime)
                         / si.dwNumberOfProcessors;
    }

    prevKernel = curKernel;
    prevUser   = curUser;
    prevWall   = curWall;
}

// ─────────────────────────────────────────────────────────────────────────────
// ServerStats
// ─────────────────────────────────────────────────────────────────────────────

void ServerStats::Init()
{
    Logger::ReserveStatsPanel(STATS_PANEL_LINES);
}

void ServerStats::Report()
{
    system.Refresh();

    // ── IOCP 구간 평균 계산 ──────────────────────────────────────────
    uint64 iocpCalls  = iocp.iocpCallCount.exchange(0);
    uint64 totalTime  = iocp.totalProcessTimeUs.exchange(0);
    uint64 avgUs      = (iocpCalls > 0) ? (totalTime / iocpCalls) : 0;

    iocp.accumCallCount     += iocpCalls;
    iocp.accumProcessTimeUs += totalTime;
    uint64 accumAvgUs = (iocp.accumCallCount > 0)
        ? (iocp.accumProcessTimeUs / iocp.accumCallCount) : 0;

    // ── 방송 수신자 누적 평균 ────────────────────────────────────────
    uint64 bcastCount = network.broadcastCount.exchange(0);
    uint64 bcastTotal = network.broadcastTotal.exchange(0);
    static uint64 prevTotal = 0, prevCount = 0;
    prevTotal += bcastTotal;
    prevCount += bcastCount;
    uint64 bcastAvg = (prevCount > 0) ? (prevTotal / prevCount) : 0;

    // ── GMetrics 에서 세션/게임 데이터 읽기 ─────────────────────────
    auto& gm = GMetrics;

    std::lock_guard<std::mutex> guard(Logger::GetConsoleLock());

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hOut, &csbi);
    COORD logPos = csbi.dwCursorPosition;

    SetConsoleCursorPosition(hOut, { 0, 0 });

    std::cout
        << "=== Server Stats ======================================\033[K\n"

        // 세션: GMetrics 에서 읽음
        << "[Session]  active="       << gm.connectedSessions.load()
        << "  conn="                  << gm.totalConnections.load()
        << "  disc="                  << gm.totalDisconnections.load()
        << "  (rv0=" << gm.disconnectRecv0.load()
        << " sv0=" << gm.disconnectSend0.load()
        << " ovfl=" << gm.disconnectRecvOverflow.load()
        << " err=" << gm.disconnectHandleError.load() << ")\033[K\n"

        // 네트워크: GMetrics 에서 읽음
        << "[Network]  recvPkt="      << gm.totalPacketsReceived.load()
        << "  sendPkt="               << gm.totalPacketsSent.load()
        << "  recvKB="                << gm.totalBytesReceived.load() / 1024
        << "  sendKB="                << gm.totalBytesSent.load()    / 1024
        << "  invalid="               << gm.invalidPackets.load()
        << "  broadcast="             << bcastCount
        << "  recipients_avg="        << bcastAvg << "\033[K\n"

        // 순환 RecvBuffer 통계 (ServerStats 독자 카운터)
        << "[RecvBuf]  linearize="    << recvBuffer.linearizeCount.exchange(0)
        << "  bufFull="               << recvBuffer.bufferFullCount.exchange(0) << "\033[K\n"

        // IOCP 성능 (ServerStats 독자 카운터)
        << "[IOCP]     calls="        << iocpCalls
        << "/s  avg="                 << avgUs
        << "us  avg_all="             << accumAvgUs << "us\033[K\n"

        // 메모리 풀 (ServerStats 독자 카운터)
        << "[Memory]   hit="          << memory.poolHitCount.exchange(0)
        << "  miss="                  << memory.poolMissCount.exchange(0)
        << "  batch="                 << memory.allocBatchCount.exchange(0)
        << "  fetch="                 << memory.fetchFromGlobalCount.exchange(0)
        << "  return="                << memory.returnToGlobalCount.exchange(0)
        << "  live="                  << memory.liveAllocCount << "\033[K\n"

        // Job 처리 (ServerStats 독자 카운터)
        << "[Job]      queued="       << job.jobsQueued.exchange(0)
        << "/s  exec="                << job.jobsExecuted.exchange(0)
        << "/s  slice="               << job.timeSlices.exchange(0)
        << "/s  timer="               << job.timerFired.exchange(0) << "/s\033[K\n"

        // 게임 오브젝트: GMetrics 에서 읽음
        << "[Game]     players="      << gm.activePlayers.load()
        << "  monsters="              << gm.activeMonsters.load()
        << "  deaths(P/M)="           << gm.totalPlayerDeaths.load()
        << "/" << gm.totalMonsterDeaths.load()
        << "  zoneChg="               << gm.totalZoneChanges.load() << "\033[K\n"

        // SendBuffer 풀: GMetrics 에서 읽음
        << "[SndBuf]   alloc="        << gm.sendBufferChunkAlloc.load()
        << "  reuse="                 << gm.sendBufferChunkReuse.load()
        << "  broadcasts="            << gm.totalBroadcasts.load()
        << "  splits="                << gm.totalBroadcastSplits.load() << "\033[K\n"

        // 시스템 리소스
        << "[System]   cpu=" << std::fixed << std::setprecision(1)
        << system.cpuUsage << "%"
        << "  mem=" << system.memUsageMB << "MB"
        << "  uptime=" << gm.UptimeSeconds() << "s\033[K\n"

        << "=======================================================\033[K\n";

    std::cout.flush();

    if (logPos.Y < STATS_PANEL_LINES)
        logPos.Y = STATS_PANEL_LINES;
    SetConsoleCursorPosition(hOut, logPos);
}

void ServerStats::ReportLoadTest()
{
    auto& lat = latency;
    auto& gm  = GMetrics;

    int64  avg      = lat.Avg();
    int64  maxVal   = lat.Max();
    int64  p99      = lat.P99();
    uint64 sent     = lat.totalSent.load();
    uint64 received = lat.totalReceived.load();
    uint64 tps      = received / 5;

    uint64 totalCount = lat.accumCount.load();
    int64  totalAvg   = (totalCount > 0) ? (lat.accumSum.load() / (int64)totalCount) : 0;

    int64 prevMax = lat.accumP99Max.load();
    if (p99 > prevMax) lat.accumP99Max.store(p99);

    std::cout
        << "\n======= Load Test Stats =======\n"
        << "[Sessions]   active=" << gm.connectedSessions.load() << "\n"
        << "[Latency]    avg=" << avg
        << "us  max=" << maxVal
        << "us  P99=" << p99 << "us\n"
        << "[Accum]      avg=" << totalAvg
        << "us  P99_peak=" << lat.accumP99Max.load()
        << "us  samples=" << totalCount << "\n"
        << "[Throughput] sent=" << sent
        << "  recv=" << received
        << "  TPS~" << tps << "/s\n"
        << "[Loss]       rate=" << std::fixed << std::setprecision(2)
        << lat.LossRate() << "%"
        << "  lost=" << lat.Lost() << "\n"
        << "================================\n";

    std::cout.flush();
    lat.Clear();
}

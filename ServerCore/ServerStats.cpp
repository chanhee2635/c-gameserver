#include "pch.h"
#include "ServerStats.h"
#include "ThreadManager.h"
#include "Logger.h"
#include <psapi.h>
#include <iomanip>

#pragma comment(lib, "psapi.lib")

void SystemStats::Refresh()
{
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        memUsageMB = pmc.WorkingSetSize / (1024 * 1024);

    static ULARGE_INTEGER prevKernel{}, prevUser{}, prevWall{};
    FILETIME ftNow, ftCreation, ftExit, ftKernel, ftUser;
    GetSystemTimeAsFileTime(&ftNow);
    GetProcessTimes(GetCurrentProcess(), &ftCreation, &ftExit, &ftKernel, &ftUser);

    ULARGE_INTEGER curKernel, curUser, curWall;
    curKernel.LowPart = ftKernel.dwLowDateTime; curKernel.HighPart = ftKernel.dwHighDateTime;
    curUser.LowPart = ftUser.dwLowDateTime; curUser.HighPart = ftUser.dwHighDateTime;
    curWall.LowPart = ftNow.dwLowDateTime; curWall.HighPart = ftNow.dwHighDateTime;

    if (prevWall.QuadPart != 0)
    {
        uint64 cpuTime = (curKernel.QuadPart - prevKernel.QuadPart) + (curUser.QuadPart - prevUser.QuadPart);
        uint64 wallTime = curWall.QuadPart - prevWall.QuadPart;

        SYSTEM_INFO si;
        GetSystemInfo(&si);
        cpuUsage = (100.0 * cpuTime) / wallTime / si.dwNumberOfProcessors;
    }
    prevKernel = curKernel; prevUser = curUser; prevWall = curWall;
}

void ServerStats::Init()
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD  mode = 0;
    if (GetConsoleMode(hOut, &mode))
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    _running = true;
}

void ServerStats::RunUpdateLoop()
{
    GLogger->ReserveStatsPanel(STATS_PANEL_LINES);

    while (_running)
    {
        RenderReport();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void ServerStats::RenderReport()
{
    system.Refresh();

    uint64 iocpCount = iocp.iocpCallCount.exchange(0);
    uint64 iocpTime = iocp.totalProcessTimeUs.exchange(0);
    uint64 avgIocpUs = (iocpCount > 0) ? (iocpTime / iocpCount) : 0;
    uint64 jobs = job.jobsExecuted.exchange(0);
    uint64 ticks = game.tickCount.exchange(0);
    uint64 dbCount = db.queryCount.exchange(0);
    uint64 dbTotalUs = db.queryTotalUs.exchange(0);
    uint64 avgDbUs = (dbCount > 0) ? (dbTotalUs / dbCount) : 0;
    uint64 poolHit = memory.poolHitCount.exchange(0);
    uint64 poolMiss = memory.poolMissCount.exchange(0); 
    int32  frameOverflow = memory.frameOverflowCount.exchange(0);
    uint64 suspicious = game.suspiciousPackets.exchange(0);
    uint64 recvKB = network.recvBytes.exchange(0) / 1024;
    uint64 sendKB = network.sendBytes.exchange(0) / 1024;
    uint64 recvPkt = network.recvPackets.exchange(0);
    uint64 sendPkt = network.sendPackets.exchange(0);
    uint64 lagTotal = game.tickLagTotalMs.exchange(0);
    uint64 lagCount = game.tickLagCount.exchange(0);
    uint64 lagMax = game.tickLagMaxMs.exchange(0);
    uint64 avgLagMs = (lagCount > 0) ? (lagTotal / lagCount) : 0;

    std::wstringstream ss;

    ss << L"==================== [ SERVER STATUS ] ====================\033[K\n";
    ss << L" [UPTIME]   " << game.GetUptime() << L"s  [CPU] " << std::fixed << std::setprecision(1) << system.cpuUsage << L"%  [MEM] " << system.memUsageMB << L"MB\033[K\n";
    ss << L" [SESSIONS] Active: " << game.connectedSessions.load() << L" (Total: " << game.totalConnections.load() << L")\033[K\n";
    ss << L" [NETWORK]  In: " << recvKB << L"KB/s (" << recvPkt << L"pkt/s)  Out: " << sendKB << L"KB/s (" << sendPkt << L"pkt/s)\033[K\n";
    ss << L" [ENGINE]   IOCP: " << iocpCount << L"/s (" << avgIocpUs << L"us)  Job: " << jobs << L"/s\033[K\n";
    ss << L" [TICKS]    Scene: " << ticks << L"/s  AvgLag: " << avgLagMs << L"ms  MaxLag: " << lagMax << L"ms";
    if (avgLagMs >= 30)
        ss << L"  \033[31m[WARN: tick delay]\033[0m";
    ss << L"\033[K\n";
    ss << L" [MEMORY]   Hit: " << poolHit << L"  Miss: " << poolMiss << L"  Live: " << memory.liveAllocCount.load();

    if (frameOverflow > 0)
        ss << L"  \033[31mFrameOvf: " << frameOverflow << L"\033[0m"; 

    ss << L"\033[K\n";
    ss << L" [WORLD]    Players: " << game.activePlayers.load()
        << L" (Peak: " << game.peakPlayers.load()
        << L")  Monsters: " << game.activeMonsters.load();

    if (suspicious > 0)
        ss << L"  \033[31mSuspicious: " << suspicious << L"\033[0m";

    ss << L"\033[K\n";
    ss << L" [DB]       Queries: " << dbCount << L"/s  Failed: " << db.queryFailed.load() << L"  AvgLatency: " << avgDbUs << L"us\033[K\n";
    ss << L"===========================================================\033[K\n";

    {
        LockGuard guard(GLogger->GetConsoleLock());

        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hOut, &csbi);
        COORD prevPos = csbi.dwCursorPosition;

        SetConsoleCursorPosition(hOut, { 0, 0 });
        std::wcout << ss.str();
        std::wcout.flush();

        if (prevPos.Y < STATS_PANEL_LINES)
            prevPos.Y = (SHORT)STATS_PANEL_LINES;

        SetConsoleCursorPosition(hOut, prevPos);
    }
}

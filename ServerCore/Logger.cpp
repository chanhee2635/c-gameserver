#include "pch.h"
#include "Logger.h"

std::mutex Logger::_consoleLock;
int        Logger::_statsPanelLines = 0;

void Logger::Init(const std::string& filename)
{
    std::lock_guard<std::mutex> guard(_consoleLock);
    if (_file.is_open()) return;
    _file.open(filename, std::ios::app);
}

void Logger::ReserveStatsPanel(int lines)
{
    // Windows VT 처리 활성화 (ANSI 커서 이동 지원)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    _statsPanelLines = lines;

    // 상단에 Stats 영역만큼 빈 줄 예약
    std::lock_guard<std::mutex> guard(_consoleLock);
    for (int i = 0; i < lines; i++)
        std::cout << "\n";
}

void Logger::Write(LogLevel level, const std::string& msg)
{
    std::string line = Timestamp()
        + " [" + LevelToString(level) + "]"
        + " [T" + std::to_string(LThreadId) + "]"
        + " " + msg + "\n";

    std::lock_guard<std::mutex> guard(_consoleLock);
    std::cout << line;
    if (_file.is_open())
        _file << line;
}

std::string Logger::Timestamp()
{
    auto now  = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &time);

    char buf[32];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
    return buf;
}

std::string Logger::LevelToString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::INFO: return "INFO";
    case LogLevel::WARN: return "WARN";
    case LogLevel::ERR:  return "ERROR";
    }
    return "UNKNOWN";
}

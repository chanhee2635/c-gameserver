#include "pch.h"
#include "Logger.h"
#include "ThreadManager.h"
#include <iostream>

Mutex Logger::_consoleLock;
int Logger::_statsPanelLines = 0;

void Logger::Init(const wstring& filename)
{
    if (_running) return;

    _file.imbue(std::locale("korean"));
    _file.open(filename, std::ios::app);

    _running = true;

    GThread->Launch(ThreadType::MONITOR, [this]() { LogThreadMain(); });
}

void Logger::Write(LogLevel level, const wstring& msg)
{
    {
        LockGuard lock(_queueLock);
        _logQueue.push({ level, msg, LThreadId });
    }
    _cv.notify_one();
}

void Logger::Shutdown()
{
    if (!_running) return;
    _running = false;
    _cv.notify_all();
}

void Logger::ReserveStatsPanel(int lines)
{
    // ANSI 이스케이프 시퀀스 활성화 (Windows 10 이상 필수)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    _statsPanelLines = lines;

    // 통계창이 들어갈 공간 확보
    LockGuard guard(_consoleLock);
    for (int i = 0; i < lines; i++)
        std::wcout << L"\n";
}

void Logger::LogThreadMain()
{
    std::wcout.imbue(std::locale("korean"));

    while (true)
    {
        LogEntry entry;
        {
            UniqueLock lock(_queueLock);
            _cv.wait(lock, [this] { return !_logQueue.empty() || !_running; });

            if (!_running && _logQueue.empty())
                break;

            entry = std::move(_logQueue.front());
            _logQueue.pop();
        }

        wstring line = Timestamp()
            + L" [" + LevelToString(entry.level) + L"]"
            + L" [T" + std::to_wstring(entry.threadId) + L"]"
            + L" " + entry.message + L"\n";

        {
            LockGuard consoleLock(_consoleLock);

            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(hOut, &csbi);

            // [수정 핵심] 현재 커서가 통계 영역(상단) 안에 있다면 강제로 경계선 아래로 이동
            if (csbi.dwCursorPosition.Y < _statsPanelLines)
            {
                SetConsoleCursorPosition(hOut, { 0, (SHORT)_statsPanelLines });
            }

            std::wcout << line;

            if (_file.is_open()) {
                _file << line;
                _file.flush();
            }
        }
    }
}

wstring Logger::Timestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &time);

    wchar_t buf[32];
    std::wcsftime(buf, sizeof(buf) / sizeof(wchar_t), L"%H:%M:%S", &tm);
    return buf;
}

wstring Logger::LevelToString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::INFO: return L"INFO";
    case LogLevel::WARN: return L"WARN";
    case LogLevel::ERR:  return L"ERROR";
    }
    return L"UNKNOWN";
}
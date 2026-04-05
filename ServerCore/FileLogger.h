#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

enum class LogLevel { INFO, WARN, ERR, FATAL };  // ERROR → ERR (Windows 매크로 충돌)

class FileLogger
{
public:
    bool Init(const std::string& filePath)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _filePath = filePath;
        _file.open(filePath, std::ios::app);
        return _file.is_open();
    }

    void Write(LogLevel level, const std::string& message)
    {
        if (level == LogLevel::INFO) return;

        std::string line = FormatLine(level, message);
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_file.is_open()) return;

        _writeSize += line.size();
        if (_writeSize > 50 * 1024 * 1024)
        {
            _file.close();
            _file.open(_filePath, std::ios::trunc);  // 파일 초기화
            _writeSize = 0;
        }

        _file << line;
        _file.flush();
    }

    void Info(const std::string& msg) { Write(LogLevel::INFO, msg); }
    void Warn(const std::string& msg) { Write(LogLevel::WARN, msg); }
    void Error(const std::string& msg) { Write(LogLevel::ERR, msg); }
    void Fatal(const std::string& msg) { Write(LogLevel::FATAL, msg); }

    template<typename... Args>
    void Infof(const char* fmt, Args... args) { Write(LogLevel::INFO, Format(fmt, args...)); }
    template<typename... Args>
    void Warnf(const char* fmt, Args... args) { Write(LogLevel::WARN, Format(fmt, args...)); }
    template<typename... Args>
    void Errorf(const char* fmt, Args... args) { Write(LogLevel::ERR, Format(fmt, args...)); }

private:
    std::string FormatLine(LogLevel level, const std::string& message)
    {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        char timeBuf[32];
        struct tm tmBuf;
        localtime_s(&tmBuf, &t);
        std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%dT%H:%M:%S", &tmBuf);

        std::ostringstream oss;
        oss << timeBuf
            << "." << std::setfill('0') << std::setw(3) << ms.count()
            << " [" << LevelStr(level) << "] "
            << message << "\n";

        return oss.str();
    }

    const char* LevelStr(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERR:   return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default:              return "INFO";
        }
    }

    template<typename... Args>
    std::string Format(const char* fmt, Args... args)
    {
        char buf[2048];
        snprintf(buf, sizeof(buf), fmt, args...);
        return std::string(buf);
    }

    std::ofstream   _file;
    std::mutex      _mutex;
    std::string     _filePath;
    size_t          _writeSize = 0;
};

extern FileLogger GFileLogger;
#pragma once
#include <mutex>
#include <string>
#include <fstream>

/*---------------
     Logger
----------------
 * 콘솔 + 파일 이중 출력
 * ServerStats 가 동일한 _consoleLock 을 공유해 커서 충돌 방지
 * LOG_INFO / LOG_WARN / LOG_ERROR 매크로로 사용
*/

enum class LogLevel : uint8 { INFO, WARN, ERR };

class Logger
{
public:
    static Logger& Get()
    {
        static Logger instance;
        return instance;
    }

    /*
    * @brief 로그 파일 초기화 (선택, 미호출 시 콘솔만 출력)
    */
    void Init(const std::string& filename);

    /*
    * @brief 레벨별 메시지 콘솔 + 파일에 기록
    */
    void Write(LogLevel level, const std::string& msg);

    // ServerStats 가 커서 조작 시 동일한 락으로 보호하도록 공개
    static std::mutex& GetConsoleLock() { return _consoleLock; }

    // Stats 패널 라인 수 예약 (Init 이후 ServerStats 가 호출)
    static void ReserveStatsPanel(int lines);
    static int  GetStatsPanelLines() { return _statsPanelLines; }

private:
    Logger() = default;

    static std::string LevelToString(LogLevel level);
    static std::string Timestamp();

    static std::mutex   _consoleLock;       // ServerStats 와 공유
    static int          _statsPanelLines;
    std::ofstream       _file;
};

#define LOG_INFO(msg)  Logger::Get().Write(LogLevel::INFO, msg)
#define LOG_WARN(msg)  Logger::Get().Write(LogLevel::WARN, msg)
#define LOG_ERROR(msg) Logger::Get().Write(LogLevel::ERR,  msg)

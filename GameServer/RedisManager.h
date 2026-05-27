#pragma once
#include <optional>
#include <sw/redis++/redis++.h>
#include "ConfigLoader.h"

class RedisManager : public JobQueue
{
public:
    bool Init(const RedisConfig& redis, const ServerConfig& server);

    std::optional<uint64> GetAccountId(const string& token);
    void DeleteToken(const string& token);
    void UnregisterServer();
    void UpdateSessionCount(int32 delta);

private:
    bool Connect(const string& host, int32 port);
    void RegisterServer(const string& name, const string& ip, int32 port, int32 maxCount);
    void ScheduleHeartbeat();
    void Heartbeat();

private:
    int32 _serverId = 0;
    std::unique_ptr<sw::redis::Redis> _redis;
};
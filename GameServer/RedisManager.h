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
    void UnregisterServer(int32 id);
    void UpdateSessionCount(int32 id, int32 delta);

private:
    bool Connect(const string& host, int32 port);
    void RegisterServer(int32 id, const string& name, const string& ip, int32 port, int32 maxCount);
    void ScheduleHeartbeat(int32 serverId);
    void Heartbeat(int32 id);

private:
    std::unique_ptr<sw::redis::Redis> _redis;
};
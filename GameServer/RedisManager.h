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
    void RegisterServer();
    void ScheduleHeartbeat();
    void Heartbeat();

private:
    int32  _serverId = 0;
    string _name;     
    string _ip;       
    int32  _port = 0; 
    int32  _maxCount = 0;
    bool   _registeredLogged = false;
    std::unique_ptr<sw::redis::Redis> _redis;
};
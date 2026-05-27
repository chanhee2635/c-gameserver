#include "pch.h"
#include "RedisManager.h"
using namespace sw::redis;

bool RedisManager::Init(const RedisConfig& redis, const ServerConfig& server)
{
    if (!Connect(redis.host, redis.port))
        return false;

    _serverId = server.id;
    RegisterServer(server.name, server.ip, server.port, server.maxSessions);
    ScheduleHeartbeat();
    LOG_INFO(L"Redis ready");
    return true;
}

bool RedisManager::Connect(const string& host, int32 port)
{
    try
    {
        ConnectionOptions opts;
        opts.host = host;
        opts.port = port;
        opts.socket_timeout = std::chrono::milliseconds(1000);

        ConnectionPoolOptions poolOpts;
        poolOpts.size = 4;

        _redis = std::make_unique<Redis>(opts, poolOpts);
        _redis->ping();
        return true;
    }
    catch (const Error& e)
    {
        LOG_ERROR(L"Redis Connect Failed");
        return false;
    }
}

void RedisManager::ScheduleHeartbeat()
{
    DoTimer(5000, [this]()
    {
        Heartbeat();
        ScheduleHeartbeat();
    });
}

std::optional<uint64> RedisManager::GetAccountId(const string& token)
{
    try
    {
        auto val = _redis->get(token);
        if (!val) return std::nullopt;
        return std::stoull(*val);
    }
    catch (const Error& e)
    {
        LOG_ERROR(L"Redis GetAccountId Failed");
        return std::nullopt;
    }
}

void RedisManager::DeleteToken(const std::string& token)
{
    try
    {
        _redis->del(token);
    }
    catch (const Error& e)
    {
        LOG_ERROR(L"Redis DeleteToken Failed");
    }
}

void RedisManager::RegisterServer(const string& name, const string& ip, int32 port, int32 maxCount)
{
    try
    {
        string key = "servers:" + std::to_string(_serverId);
        _redis->hset(key, "name", name);
        _redis->hset(key, "ip", ip);
        _redis->hset(key, "port", std::to_string(port));
        _redis->hset(key, "current", "0");
        _redis->hset(key, "max", std::to_string(maxCount));
        _redis->expire(key, std::chrono::seconds(15));
        _redis->sadd("server_ids", std::to_string(_serverId));
        LOG_INFO(L"Server registered to Redis id=" + std::to_wstring(_serverId));
    }
    catch (const Error& e)
    {
        LOG_ERROR(L"Redis RegisterServer Failed");
    }
}

void RedisManager::UnregisterServer()
{
    try
    {
        _redis->del("servers:" + std::to_string(_serverId));
        _redis->srem("server_ids", std::to_string(_serverId));
        LOG_INFO(L"Server unregistered from Redis id=" + std::to_wstring(_serverId));
    }
    catch (const Error& e)
    {
        LOG_ERROR(L"Redis UnregisterServer Failed");
    }
}

void RedisManager::Heartbeat()
{
    try
    {
        _redis->expire("servers:" + std::to_string(_serverId), std::chrono::seconds(15));
    }
    catch (const Error& e)
    {
        LOG_ERROR(L"Redis Heartbeat Failed");
    }
}

void RedisManager::UpdateSessionCount(int32 delta)
{
    try
    {
        _redis->hincrby("servers:" + std::to_string(_serverId), "current", delta);
    }
    catch (const Error& e)
    {
        LOG_ERROR(L"Redis UpdateSessionCount Failed");
    }
}
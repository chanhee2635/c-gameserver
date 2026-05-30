#include "pch.h"
#include "RedisManager.h"
using namespace sw::redis;

bool RedisManager::Init(const RedisConfig& redis, const ServerConfig& server)
{
    if (!Connect(redis.host, redis.port))
        return false;

    _serverId = server.id;
    _name = server.name;
    _ip = server.ip;
    _port = server.port;
    _maxCount = server.maxSessions;
    RegisterServer();
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

void RedisManager::RegisterServer()
{
    try
    {
        string key = "servers:" + std::to_string(_serverId);
        _redis->hset(key, "name", _name);
        _redis->hset(key, "ip", _ip);
        _redis->hset(key, "port", std::to_string(_port));
        _redis->hset(key, "max", std::to_string(_maxCount));
        _redis->hsetnx(key, "current", "0");
        _redis->expire(key, std::chrono::seconds(15));
        _redis->sadd("server_ids", std::to_string(_serverId));
        if (!_registeredLogged)
        {
            _registeredLogged = true;
            LOG_INFO(L"Server registered to Redis id=" + std::to_wstring(_serverId));
        }
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
    RegisterServer();
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
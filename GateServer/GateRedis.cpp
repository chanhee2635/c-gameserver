#include "pch.h"
#include "GateRedis.h"
#include <random>
using namespace sw::redis;

static std::string GenerateToken()
{
    static thread_local std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 15);
    static const char* HEX = "0123456789abcdef";
    std::string s; 
    s.reserve(32);
    for (int i = 0; i < 32; ++i) s += HEX[dist(rng)];
    return s;
}

bool GateRedis::Connect(const string& host, int32 port)
{
    try
    {
        ConnectionOptions opts;
        opts.host = host;
        opts.port = port;
        opts.socket_timeout = std::chrono::milliseconds(1000);

        ConnectionPoolOptions poolOpts;
        poolOpts.size = 8;

        _redis = std::make_unique<Redis>(opts, poolOpts);
        _redis->ping();
        return true;
    }
    catch (const Error& e) { LOG_ERROR(L"GateRedis Connect Failed"); return false; }
}

std::optional<uint64> GateRedis::ResolveQueueToken(const string& queueToken)
{
    try
    {
        auto val = _redis->get("qsession:" + queueToken);
        if (!val) return std::nullopt;
        return std::stoull(*val);
    }
    catch (const Error& e) { LOG_ERROR(L"GateRedis ResolveQueueToken Failed"); return std::nullopt; }
}

bool GateRedis::GetServerInfo(int32 serverId, ServerSlot& out)
{
    try
    {
        const string key = "servers:" + std::to_string(serverId);
        auto ip = _redis->hget(key, "ip");
        auto pt = _redis->hget(key, "port");
        auto cu = _redis->hget(key, "current");
        auto mx = _redis->hget(key, "max");
        if (!ip || !pt || !cu || !mx) return false;

        out.ip = *ip;
        out.port = std::stoi(*pt);
        out.current = std::stoi(*cu);
        out.max = std::stoi(*mx);
        return true;
    }
    catch (const Error& e) { LOG_ERROR(L"GateRedis GetServerInfo Failed"); return false; }
}

std::string GateRedis::IssueAuthToken(uint64 accountId)
{
    try
    {
        const string token = GenerateToken();
        _redis->set(token, std::to_string(accountId), std::chrono::seconds(600));
        _redis->set("session:" + std::to_string(accountId), token, std::chrono::seconds(600));
        return token;
    }
    catch (const Error& e) { LOG_ERROR(L"GateRedis IssueAuthToken Failed"); return ""; }
}
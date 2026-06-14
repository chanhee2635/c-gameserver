#pragma once

struct DBConfig
{
    wstring host = L"127.0.0.1";
    uint32 port = 3306;
    wstring user = L"root";
    wstring password;
    wstring name = L"portfolio";
    int32  threads = 2;
};

struct RedisConfig
{
    string host = "127.0.0.1";
    int32  port = 6379;
};

struct ServerConfig
{
    int32  id = 1;
    string name = "Server1";
    string ip = "127.0.0.1";
    uint32 port = 7777;
    int32  maxSessions = 2000;
    int32  acceptPool = 256;
};

struct WorldConfig
{
    int32 mapSize    = 1000;
    int32 zoneSize   = 20;
    int32 sceneCount = 8;
};

struct GameplayConfig
{
    uint32 updateTickMs           = 50;
    float  attackPosToleranceSq   = 9.0f;
    int64  monsterSearchTickMs    = 500;
    int64  monsterIdleTickMs      = 500;
    int64  monsterRepathCooldownMs = 300;
    int64  findPathFailCooldownMs = 3000;
};

struct AppConfig
{
    DBConfig      db;
    ServerConfig  server;
    RedisConfig   redis;
    WorldConfig   world;
    GameplayConfig gameplay;
};

class ConfigLoader
{
public:
    static bool Load(const string& path, AppConfig& outConfig);
};
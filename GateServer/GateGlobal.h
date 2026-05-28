#pragma once

struct GateConfig
{
    string redisHost = "127.0.0.1";
    int32  redisPort = 6379;
    uint16 listenPort = 6666;
};

class GateRedis;
class QueueManager;

extern GateRedis* GGateRedis;
extern QueueManager* GQueueManager;

class GateGlobal
{
public:
    static void Init();
    static void Clear();
    static const GateConfig& GetConfig() { return _config; }

private:
    static GateConfig                    _config;
    static std::unique_ptr<GateRedis>    _gateRedis;
    static std::shared_ptr<QueueManager> _queueManager;   // JobQueue 파생 → shared_ptr 필수
};
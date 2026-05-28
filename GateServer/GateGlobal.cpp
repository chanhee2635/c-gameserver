#include "pch.h"
#include "GateGlobal.h"
#include "GateRedis.h"
#include "QueueManager.h"

GateConfig                    GateGlobal::_config = {};
std::unique_ptr<GateRedis>    GateGlobal::_gateRedis = nullptr;
std::shared_ptr<QueueManager> GateGlobal::_queueManager = nullptr;

GateRedis* GGateRedis = nullptr;
QueueManager* GQueueManager = nullptr;

void GateGlobal::Init()
{
    CoreGlobal::Init();

    _gateRedis = std::make_unique<GateRedis>();
    GGateRedis = _gateRedis.get();
    if (!GGateRedis->Connect(_config.redisHost, _config.redisPort))
        LOG_ERROR(L"GateRedis::Connect failed");

    _queueManager = MakeShared<QueueManager>();
    GQueueManager = _queueManager.get();
    GQueueManager->Start();

    LOG_INFO(L"=== [Gate Ready to Accept Connections] ===");
}

void GateGlobal::Clear()
{
    GQueueManager = nullptr;
    _queueManager = nullptr;
    GGateRedis = nullptr;
    _gateRedis = nullptr;
    CoreGlobal::Clear();
}
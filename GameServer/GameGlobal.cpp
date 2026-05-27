#include "pch.h"
#include "GameGlobal.h"
#include "ConfigLoader.h"
#include "DBManager.h"
#include "RedisManager.h"
#include "SessionManager.h"
#include "DataManager.h"
#include "GridMap.h"
#include "World.h"

AppConfig                          GameGlobal::_config         = {};
std::shared_ptr<DBManager>         GameGlobal::_dbManager      = nullptr;
std::shared_ptr<RedisManager>      GameGlobal::_redisManager   = nullptr;
std::unique_ptr<SessionManager>    GameGlobal::_sessionManager = nullptr;
std::unique_ptr<DataManager>       GameGlobal::_dataManager    = nullptr;
std::unique_ptr<GridMap>           GameGlobal::_gridMap        = nullptr;
std::shared_ptr<World>             GameGlobal::_world          = nullptr;

DBManager*        GDBManager        = nullptr;
RedisManager*     GRedisManager     = nullptr;
SessionManager*   GSessionManager   = nullptr;
DataManager*      GDataManager      = nullptr;
GridMap*          GGridMap          = nullptr;
World*            GWorld            = nullptr;

void GameGlobal::Init()
{
    CoreGlobal::Init();

    if (!ConfigLoader::Load("server.json", _config))
        LOG_ERROR(L"Failed to load server.json : using defaults");

    _dataManager = std::make_unique<DataManager>();
    GDataManager = _dataManager.get();
    if (!GDataManager->LoadData())
        LOG_ERROR(L"DataManager::LoadData() failed");

    _gridMap = std::make_unique<GridMap>();
    GGridMap = _gridMap.get();
    if (!GGridMap->Load("../Common/Map/walkmap.bin"))
        LOG_WARN(L"GridMap: ../Common/Map/walkmap.bin 로드 실패 (충돌/길찾기 비활성화됨)");

    _world = MakeShared<World>();
    GWorld = _world.get();
    GWorld->Init(_config.world);
    GWorld->Start();

    _dbManager = MakeShared<DBManager>();
    GDBManager = _dbManager.get();
    if (!GDBManager->Init(_config.db))
        LOG_ERROR(L"DBManager::Init failed");

    _redisManager = MakeShared<RedisManager>();
    GRedisManager = _redisManager.get();
    if (!GRedisManager->Init(_config.redis, _config.server))
        LOG_ERROR(L"RedisManager::Init failed");

    _sessionManager = std::make_unique<SessionManager>();
    GSessionManager = _sessionManager.get();

    LOG_INFO(L"=== [Server Ready to Accept Connections] ===");
}

void GameGlobal::Clear()
{
    LOG_INFO(L"Server shutting down...");

    GSessionManager = nullptr;
    _sessionManager = nullptr;

    GWorld = nullptr;
    _world = nullptr;

    if (GRedisManager)
        GRedisManager->UnregisterServer(_config.server.id);

    GRedisManager = nullptr;
    _redisManager = nullptr;

    GDBManager = nullptr;
    _dbManager = nullptr;

    GGridMap = nullptr;
    _gridMap = nullptr;

    GDataManager = nullptr;
    _dataManager = nullptr;

    CoreGlobal::Clear();
}

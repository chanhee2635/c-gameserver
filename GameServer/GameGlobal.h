#pragma once
#include <memory>
#include "ConfigLoader.h"

class DBManager;
class RedisManager;
class SessionManager;
class DataManager;
class NavigationManager;
class World;

extern DBManager*        GDBManager;
extern RedisManager*     GRedisManager;
extern SessionManager*   GSessionManager;
extern DataManager*      GDataManager;
extern NavigationManager* GNavigationManager;
extern World*            GWorld;

class GameGlobal
{
public:
    static void Init();
    static void Clear();
    static const AppConfig& GetConfig() { return _config; }

private:
    static AppConfig                          _config;
    static std::shared_ptr<DBManager>         _dbManager;
    static std::shared_ptr<RedisManager>      _redisManager;
    static std::unique_ptr<SessionManager>    _sessionManager;
    static std::unique_ptr<DataManager>       _dataManager;
    static std::unique_ptr<NavigationManager> _navManager;
    static std::shared_ptr<World>             _world;
};

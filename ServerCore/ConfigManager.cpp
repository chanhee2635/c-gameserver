#include "pch.h"
#include "ConfigManager.h"
#include <fstream>

unique_ptr<ConfigManager> GConfigManager = make_unique<ConfigManager>();

bool ConfigManager::LoadConfig(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) return false;

    try {
        json j;
        file >> j;

        auto& global = j["Global"];
        _db.connectionString = Utils::s2ws(global["DB"]["ConnectionString"]);

        _redis.ip = global["Redis"]["Ip"];
        _redis.port = global["Redis"]["Port"];
        _redis.poolSize = global["Redis"]["PoolSize"];

        auto ParseServerConfig = [&](const json& serverJson) -> ServerConfig {
            ServerConfig config;
            config.id = serverJson["Info"]["Id"];
            config.name = Utils::s2ws(serverJson["Info"]["Name"]);
            config.ip = Utils::s2ws(serverJson["Network"]["Ip"]);
            config.port = serverJson["Network"]["Port"];
            config.acceptCount = serverJson["Network"]["AcceptCount"];

            config.recvBufferSize = serverJson["Buffer"]["RecvBufferSize"];
            config.recvBufferCount = serverJson["Buffer"]["RecvBufferCount"];

            config.workerThreadCount = serverJson["Performance"]["WorkerThreadCount"];
            config.dbThreadCount = serverJson["Performance"]["DBThreadCount"];
            config.logicWorkedTick = serverJson["Performance"]["LogicWorkedTick"];
            config.dbWorkedTick = serverJson["Performance"]["DBWorkedTick"];
            config.maxSessionCount = serverJson["Performance"]["MaxSessionCount"];
            return config;
        };

        _loginServer = ParseServerConfig(j["LoginServer"]);
        _chatServer = ParseServerConfig(j["ChatServer"]);
        _gameServer = ParseServerConfig(j["GameServer"]);

        _logic.updateTick = j["Logic"]["UpdateTick"];
        _logic.damageTick = j["Logic"]["DamageTick"];
        _logic.deadTick = j["Logic"]["DeadTick"];
        _logic.searchTick = j["Logic"]["SearchTick"];

        _world.mapSize = j["World"]["MapSize"];
        _world.zoneSize = j["World"]["ZoneSize"];
        _world.sceneCount = j["World"]["SceneCount"];

        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Config Parse Error: " << e.what() << std::endl;
        return false;
    }
}

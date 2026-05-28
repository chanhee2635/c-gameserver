#include "pch.h"
#include "ConfigLoader.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

bool ConfigLoader::Load(const string& path, AppConfig& outConfig)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        LOG_ERROR(L"Config file not found");
        return false;
    }

    json j;
    try
    {
        file >> j;
    }
    catch (const json::parse_error& e)
    {
        LOG_ERROR(L"Config parse error: " + Utils::ToWString(e.what()));
        return false;
    }

    // Database
    if (j.contains("database"))
    {
        auto& db = j["database"];
        if (db.contains("host"))     outConfig.db.host = Utils::ToWString(db["host"].get<string>());
        if (db.contains("port"))     outConfig.db.port = db["port"].get<uint32>();
        if (db.contains("user"))     outConfig.db.user = Utils::ToWString(db["user"].get<string>());
        if (db.contains("password")) outConfig.db.password = Utils::ToWString(db["password"].get<string>());
        if (db.contains("name"))     outConfig.db.name = Utils::ToWString(db["name"].get<string>());
        if (db.contains("threads"))  outConfig.db.threads = db["threads"].get<int32>();
    }

    // Server
    if (j.contains("server"))
    {
        auto& sv = j["server"];
        if (sv.contains("id"))          outConfig.server.id = sv["id"].get<int32>();
        if (sv.contains("name"))        outConfig.server.name = sv["name"].get<std::string>();
        if (sv.contains("ip"))          outConfig.server.ip = sv["ip"].get<std::string>();
        if (sv.contains("port"))        outConfig.server.port = sv["port"].get<uint32>();
        if (sv.contains("maxSessions")) outConfig.server.maxSessions = sv["maxSessions"].get<int32>();
        if (sv.contains("acceptPool"))  outConfig.server.acceptPool = sv["acceptPool"].get<int32>();
    }

    // Redis
    if (j.contains("redis"))
    {
        auto& r = j["redis"];
        if (r.contains("host")) outConfig.redis.host = r["host"].get<std::string>();
        if (r.contains("port")) outConfig.redis.port = r["port"].get<int32>();
    }

    // World
    if (j.contains("world"))
    {
        auto& w = j["world"];
        if (w.contains("mapSize"))    outConfig.world.mapSize    = w["mapSize"].get<int32>();
        if (w.contains("zoneSize"))   outConfig.world.zoneSize   = w["zoneSize"].get<int32>();
        if (w.contains("sceneCount")) outConfig.world.sceneCount = w["sceneCount"].get<int32>();
    }

    // Gameplay
    if (j.contains("gameplay"))
    {
        auto& g = j["gameplay"];
        if (g.contains("updateTickMs"))           outConfig.gameplay.updateTickMs           = g["updateTickMs"].get<uint32>();
        if (g.contains("attackPosToleranceSq"))   outConfig.gameplay.attackPosToleranceSq   = g["attackPosToleranceSq"].get<float>();
        if (g.contains("monsterSearchTickMs"))    outConfig.gameplay.monsterSearchTickMs    = g["monsterSearchTickMs"].get<int64>();
        if (g.contains("findPathFailCooldownMs")) outConfig.gameplay.findPathFailCooldownMs = g["findPathFailCooldownMs"].get<int64>();
    }

    LOG_INFO(L"Config loaded successfully");
    return true;
}
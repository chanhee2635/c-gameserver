#include "pch.h"
#include "ConfigLoader.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace
{
    // Assign into out if the key exists. Type is deduced from out.
    template<typename T>
    void ReadField(const json& j, const char* key, T& out)
    {
        if (j.contains(key)) out = j[key].get<T>();
    }

    // Convert a UTF-8 string field into a wstring field.
    void ReadWString(const json& j, const char* key, wstring& out)
    {
        if (j.contains(key)) out = Utils::ToWString(j[key].get<std::string>());
    }
}

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
        ReadWString(db, "host",     outConfig.db.host);
        ReadField  (db, "port",     outConfig.db.port);
        ReadWString(db, "user",     outConfig.db.user);
        ReadWString(db, "password", outConfig.db.password);
        ReadWString(db, "name",     outConfig.db.name);
        ReadField  (db, "threads",  outConfig.db.threads);
    }

    // Server
    if (j.contains("server"))
    {
        auto& sv = j["server"];
        ReadField(sv, "id",          outConfig.server.id);
        ReadField(sv, "name",        outConfig.server.name);
        ReadField(sv, "ip",          outConfig.server.ip);
        ReadField(sv, "port",        outConfig.server.port);
        ReadField(sv, "maxSessions", outConfig.server.maxSessions);
        ReadField(sv, "acceptPool",  outConfig.server.acceptPool);
    }

    // Redis
    if (j.contains("redis"))
    {
        auto& r = j["redis"];
        ReadField(r, "host", outConfig.redis.host);
        ReadField(r, "port", outConfig.redis.port);
    }

    // World
    if (j.contains("world"))
    {
        auto& w = j["world"];
        ReadField(w, "mapSize",    outConfig.world.mapSize);
        ReadField(w, "zoneSize",   outConfig.world.zoneSize);
        ReadField(w, "sceneCount", outConfig.world.sceneCount);
    }

    // Gameplay
    if (j.contains("gameplay"))
    {
        auto& g = j["gameplay"];
        ReadField(g, "updateTickMs",            outConfig.gameplay.updateTickMs);
        ReadField(g, "attackPosToleranceSq",    outConfig.gameplay.attackPosToleranceSq);
        ReadField(g, "monsterSearchTickMs",     outConfig.gameplay.monsterSearchTickMs);
        ReadField(g, "monsterIdleTickMs",       outConfig.gameplay.monsterIdleTickMs);
        ReadField(g, "monsterRepathCooldownMs", outConfig.gameplay.monsterRepathCooldownMs);
        ReadField(g, "findPathFailCooldownMs",  outConfig.gameplay.findPathFailCooldownMs);
    }

    LOG_INFO(L"Config loaded successfully");
    return true;
}
#pragma once
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ConfigManager
{
public:
	ConfigManager() = default;
	~ConfigManager() = default;

	/*
	* @brief 설정 파일(JSON)을 로드하고 서버 구성 정보를 초기화한다.
	* @param path 설정 파일이 위치한 경로
	* @return 성공 시 true, 파일이 없거나 파싱 실패 시 false 반환
	*/
	bool LoadConfig(const std::string& path);

	const DBConfig& GetDB() const { return _db; }
	const RedisConfig& GetRedis() { return _redis; }
	const ServerConfig& GetLogin() { return _loginServer; }
	const ServerConfig& GetChat() { return _chatServer; }
	const ServerConfig& GetGame() { return _gameServer; }
	const LogicConfig& GetLogic() { return _logic; }
	const WorldConfig GetWorld() { return _world; }

private:
	DBConfig _db;
	RedisConfig _redis;
	ServerConfig _loginServer;
	ServerConfig _chatServer;
	ServerConfig _gameServer;
	LogicConfig _logic;
	WorldConfig _world;
};

extern unique_ptr<ConfigManager> GConfigManager;
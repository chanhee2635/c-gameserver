#include "pch.h"
#include "RedisManager.h"
#include "ConfigManager.h"

unique_ptr<RedisManager> GRedisManager = make_unique<RedisManager>();

RedisManager::RedisManager()
{
}

RedisManager::~RedisManager()
{
}

bool RedisManager::Connect()
{
	auto& redisConfig = GConfigManager->GetRedis();

	sw::redis::ConnectionOptions opts;
	opts.host = redisConfig.ip;
	opts.port = redisConfig.port;
	opts.socket_timeout = std::chrono::milliseconds(1000);

	sw::redis::ConnectionPoolOptions pool_opts;
	pool_opts.size = redisConfig.poolSize;

	_redis = make_unique<Redis>(opts, pool_opts);

	auto pipe = _redis->pipeline();
	for (int32 i = 1; i <= 1000; ++i)
		pipe.set("user" + std::to_string(i), std::to_string(i));
	auto replies = pipe.exec();

	return true;
}

void RedisManager::SaveServerStatus(const std::string& key, const std::string& info, int expireSeconds)
{
	try
	{
		_redis->setex(key, expireSeconds, info);
		_redis->sadd("GameServerList", key);
	}
	catch (const Error& e)
	{
		std::cerr << "Redie Error: " << e.what() << std::endl;
	}
}

void RedisManager::SetPlayerInfo(uint64 playerId, const std::string& nickname, int32 port)
{
	try
	{
		std::string key = "Player:" + std::to_string(playerId);
		std::string value = nickname + ":" + std::to_string(port);

		_redis->setex(key, 3600, value);
	}
	catch (const Error& e)
	{
		std::cerr << "Redie Error: " << e.what() << std::endl;
	}
}

void RedisManager::PublishZoneChange(uint64 playerId, int32 zoneId)
{
	try
	{
		string msg = to_string(playerId) + ":" + to_string(zoneId);
		_redis->publish("ZoneUpdate", msg);
	}
	catch (const Error& e)
	{
		std::cerr << "Redie Error: " << e.what() << std::endl;
	}
}




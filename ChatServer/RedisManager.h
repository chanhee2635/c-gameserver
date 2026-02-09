#pragma once
#include <sw/redis++/redis++.h>

using namespace sw::redis;

class RedisManager
{
public:
	RedisManager(const std::string& host, int port, int poolSize);
	~RedisManager();

	template <typename T>
	bool Set(const std::string& key, const T& value);

	template <typename T>
	std::optional<T> Get(const std::string& key);

	bool CheckPlayer(uint64 playerId, ChatSessionRef session);
	void SubscribeRedis();

private:
	std::unique_ptr<Redis> _redis;
	std::unique_ptr<Redis> _subRedis;
};

extern std::unique_ptr<RedisManager>	GRedisManager;
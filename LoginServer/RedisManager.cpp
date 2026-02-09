#include "pch.h"
#include "RedisManager.h"

unique_ptr<RedisManager> GRedisManager = make_unique<RedisManager>("127.0.0.1", 6379, 10);

RedisManager::RedisManager(const string& host, int port, int poolSize)
{
	sw::redis::ConnectionOptions opts;
	opts.host = host;
	opts.port = port;
	opts.socket_timeout = std::chrono::milliseconds(200);

	sw::redis::ConnectionPoolOptions pool_opts;
	pool_opts.size = poolSize;

	_redis = make_unique<Redis>(opts, pool_opts);
}

RedisManager::~RedisManager()
{
}

Vector<string> RedisManager::GetActiveServers()
{
	Vector<string> serverKeys;
	Vector<string> results;
	try
	{
		_redis->smembers("GameServerList", std::back_inserter(serverKeys));
		for (const string& key : serverKeys) {
			auto val = _redis->get(key);
			if (val) results.push_back(*val);
			else _redis->srem("GameServerList", key);
		}
	}
	catch (const Error& e)
	{
		std::cerr << "Redie Error: " << e.what() << std::endl;
	}

	return results;
}

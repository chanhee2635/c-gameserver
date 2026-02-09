#include "pch.h"
#include "RedisManager.h"
#include "SessionManager.h"
#include "ChatSession.h"

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

	sw::redis::ConnectionOptions subOpts;
	subOpts.host = host;
	subOpts.port = port;
	subOpts.socket_timeout = std::chrono::milliseconds(0);
	_subRedis = make_unique<Redis>(subOpts);
}

RedisManager::~RedisManager()
{
}

template <typename T>
bool RedisManager::Set(const std::string& key, const T& value)
{
	try
	{
		string valStr;

		if constexpr (std::is_arithmetic_v<T>)
			valStr = std::to_string(value);
		else
			valStr = value;

		_redis->set(key, valStr);

		return true;
	}
	catch (const Error& e)
	{
		std::cerr << "Redie Error: " << e.what() << std::endl;
		return false;
	}
}

template <typename T>
std::optional<T> RedisManager::Get(const std::string& key)
{
	try
	{
		auto val = _redis->get(key);
		if (!val) return std::nullopt;

		if constexpr (std::is_same_v<T, int8>) return static_cast<int8>(std::stoi(*val));
		else if constexpr (std::is_same_v<T, int16>) return static_cast<int16>(std::stoi(*val));
		else if constexpr (std::is_same_v<T, int32>) return std::stoi(*val);
		else if constexpr (std::is_same_v<T, int64>) return std::stoll(*val);
		else if constexpr (std::is_same_v<T, uint8>) return static_cast<uint8>(std::stoi(*val));
		else if constexpr (std::is_same_v<T, uint16>) return static_cast<uint16>(std::stoul(*val));
		else if constexpr (std::is_same_v<T, uint32>) return std::stoul(*val);
		else if constexpr (std::is_same_v<T, uint64>) return std::stoull(*val);
		else if constexpr (std::is_same_v<T, float>) return std::stof(*val);
		else if constexpr (std::is_same_v<T, double>) return std::stod(*val);
		else if constexpr (std::is_same_v<T, long long>) return std::stoll(*val);
		else if constexpr (std::is_same_v<T, std::string>) return *val;
		else return std::nullopt;
	}
	catch (const Error& e)
	{
		std::cerr << "Redie Error: " << e.what() << std::endl;
		return std::nullopt;
	}
}

bool RedisManager::CheckPlayer(uint64 playerId, ChatSessionRef session)
{
	std::string key = "Player:" + std::to_string(playerId);

	auto info = Get<std::string>(key);

	if (info.has_value())
	{
		string data = info.value();

		size_t pos = data.find(':');
		string name = (pos == string::npos) ? data : data.substr(0, pos);

		session->SetPlayerId(playerId);
		session->SetName(name);

		std::cout << "Chat Server Auth Success: " << playerId << std::endl;
		return true;
	}

	std::cout << "Chat Server Auth Failed: " << playerId << std::endl;
	return false;
}

void RedisManager::SubscribeRedis()
{
	try
	{
		auto sub = _subRedis->subscriber();

		sub.on_message([](std::string channel, std::string msg) {
			if (channel == "ZoneUpdate")
				GSessionManager->HandleZoneUpdate(msg);
		});

		sub.subscribe("ZoneUpdate");

		while (true)
		{
			sub.consume();
		}
	}
	catch (const Error& e)
	{
		std::cerr << "Redie Error: " << e.what() << std::endl;
	}
}

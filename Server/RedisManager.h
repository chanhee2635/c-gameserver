#pragma once
#include <sw/redis++/redis++.h>

using namespace sw::redis;

class RedisManager
{
public:
	RedisManager();
	~RedisManager();

	bool Connect();

	template <typename T>
	bool Set(const std::string& key, const T& value)
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
	std::optional<T> Get(const std::string& key)
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
			return false;
		}
	}

	void SaveServerStatus(const std::string& key, const std::string& info, int expireSeconds);
	void SetPlayerInfo(uint64 playerId, const std::string& nickname, int32 port);
	void PublishZoneChange(uint64 playerId, int32 zoneId);

private:
	std::unique_ptr<Redis> _redis;
};

extern std::unique_ptr<RedisManager>	GRedisManager;
#pragma once
#include <sstream>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

class Utils
{
public:
	// string (UTF-8) -> wstring (UTF-16)
	static std::wstring s2ws(const std::string& s)
	{
		if (s.empty()) return L"";

		int size = ::MultiByteToWideChar(CP_UTF8, 0, &s[0], (int)s.size(), NULL, 0);

		wstring wstrTo(size, 0);

		::MultiByteToWideChar(CP_UTF8, 0, &s[0], (int)s.size(), &wstrTo[0], size);

		return wstrTo;
	}

	// wstring (UTF-16) -> string (UTF-8)
	static std::string ws2s(const std::wstring& ws)
	{
		if (ws.empty()) return "";

		int size = ::WideCharToMultiByte(CP_UTF8, 0, &ws[0], (int)ws.size(), NULL, 0, NULL, NULL);

		string strTo(size, 0);

		::WideCharToMultiByte(CP_UTF8, 0, &ws[0], (int)ws.size(), &strTo[0], size, NULL, NULL);

		return strTo;
	}

	static std::string Trim(const std::string& s)
	{
		const std::string whitespace = " \t\n\r\f\v";

		size_t start = s.find_first_not_of(whitespace);
		if (start == std::string::npos)
			return "";

		size_t end = s.find_last_not_of(whitespace);

		return s.substr(start, end - start + 1);
	}

	static Vector<std::string> Split(const std::string& s, char delimiter)
	{
		Vector<std::string> tokens;
		std::string token;
		std::istringstream iss(s);

		while (std::getline(iss, token, delimiter))
		{
			tokens.push_back(token);
		}

		return tokens;
	}

	static wstring GetCurrentTime()
	{
		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);

		struct tm buf;
		localtime_s(&buf, &in_time_t);

		wchar_t dateStr[64];
		swprintf_s(dateStr, L"%04d-%02d-%02d %02d:%02d:%02d",
			buf.tm_year + 1900, buf.tm_mon + 1, buf.tm_mday,
			buf.tm_hour, buf.tm_min, buf.tm_sec);

		return wstring(dateStr);
	}

	static void PrintServerStatus(ServiceRef service);
};

struct DBConfig
{
	std::wstring connectionString;
};

struct RedisConfig
{
	std::string ip;
	int32 port;
	int32 poolSize;
};

struct ServerConfig
{
	int32 id;
	std::wstring name;
	std::wstring ip;
	int32 port;
	int32 acceptCount;
	int32 recvBufferSize;
	int32 recvBufferCount;
	int32 workerThreadCount;
	int32 dbThreadCount;
	uint64 logicWorkedTick;
	uint64 dbWorkedTick;
	int32 maxSessionCount;
};

struct LogicConfig
{
	uint64 updateTick;
	uint64 damageTick;
	uint64 deadTick;
	uint64 searchTick;
	uint64 flushTick;
	uint64 monitorTick;
	uint64 redisTick;
};

struct WorldConfig
{
	int32 mapSize;
	int32 zoneSize;
	int32 sceneCount;
};
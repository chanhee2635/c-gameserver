#include "pch.h"
#include "SessionManager.h"
#include "ChatSession.h"
#include "Zone.h"
#include "ConfigManager.h"

extern std::unique_ptr<SessionManager> GSessionManager = make_unique<SessionManager>();

void SessionManager::Start()
{
	WorldConfig config = GConfigManager->GetWorld();

	_zoneCount = config.mapSize / config.zoneSize;
	_totalZoneCount = _zoneCount * _zoneCount;

	// Zones은 생성 이후로 수정/삭제가 이뤄지지 않아 Lock을 잡지 않아도 됨
	_zones.reserve(_totalZoneCount);
	for (int32 i = 0; i < _totalZoneCount; ++i)
		_zones.push_back(MakeShared<Zone>(i));
}

void SessionManager::Add(ChatSessionRef session)
{
	WRITE_LOCK;
	_sessions[session->GetPlayerId()] = session;
}

void SessionManager::Remove(ChatSessionRef session)
{
	uint64 playerId = session->GetPlayerId();
	int32 zoneId = session->GetZoneId();

	{
		WRITE_LOCK;
		_sessions.erase(playerId);
	}

	if (zoneId >= 0)
	{
		_zones[zoneId]->DoAsync([zone = _zones[zoneId], session]() {
			zone->Remove(session);
		});
	}
}

void SessionManager::Broadcast(SendBufferRef sendBuffer)
{
	for (auto& zone : _zones)
	{
		zone->DoAsync([zone, sendBuffer]() {
			zone->Broadcast(sendBuffer);
		});
	}
}

void SessionManager::BroadcastToAdjacent(int32 zoneId, SendBufferRef sendBuffer)
{
	Vector<ZoneRef> zones = GetAdjacentZones(zoneId);

	for (ZoneRef zone : zones)
	{
		zone->DoAsync([zone, sendBuffer]() {
			zone->Broadcast(sendBuffer);
		});
	}
}

Vector<ZoneRef> SessionManager::GetAdjacentZones(int32 zoneId)
{
	Vector<ZoneRef> adjacent;
	int32 row = zoneId / _zoneCount;
	int32 col = zoneId % _zoneCount;

	for (int32 dr = -1; dr <= 1; ++dr)
	{
		for (int32 dc = -1; dc <= 1; ++dc)
		{
			int32 nr = row + dr;
			int32 nc = col + dc;
			if (nr >= 0 && nr < _zoneCount && nc >= 0 && nc < _zoneCount)
				adjacent.push_back(_zones[nr * _zoneCount + nc]);
		}
	}

	return adjacent;
}

void SessionManager::HandleZoneUpdate(const std::string& msg)
{
	size_t pos = msg.find(':');
	if (pos == std::string::npos) return;

	uint64 playerId = std::stoull(msg.substr(0, pos));
	int32 newZoneId = std::stoi(msg.substr(pos + 1));

	ChatSessionRef session = nullptr;

	{
		READ_LOCK;
		auto it = _sessions.find(playerId);
		if (it == _sessions.end())
			return;
		session = it->second;
	}

	int32 oldZoneId = session->GetZoneId();

	if (oldZoneId >= 0 && oldZoneId < _totalZoneCount && oldZoneId != newZoneId)
	{
		_zones[oldZoneId]->DoAsync([zone = _zones[oldZoneId], session]() {
			zone->Remove(session);
		});
	}

	if (newZoneId >= 0 && newZoneId < _totalZoneCount)
	{
		_zones[newZoneId]->DoAsync([zone = _zones[newZoneId], session]() {
			zone->Add(session);
		});
		session->SetZoneId(newZoneId);
	}
}

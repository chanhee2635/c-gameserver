#pragma once

class SessionManager
{
public:
	void Start();

	void Add(ChatSessionRef session);
	void Remove(ChatSessionRef session);
	void Broadcast(SendBufferRef sendBuffer);
	void BroadcastToAdjacent(int32 zoneId, SendBufferRef sendBuffer);
	Vector<ZoneRef> GetAdjacentZones(int32 zoneId);

	void HandleZoneUpdate(const std::string& msg);

private:
	USE_LOCK;
	Map<uint64, ChatSessionRef> _sessions;
	Vector<ZoneRef> _zones;
	int32 _zoneCount;
	int32 _totalZoneCount;
};


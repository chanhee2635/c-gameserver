#pragma once
#include "ChatSession.h"

class Zone : public JobQueue
{
public:
	Zone(int32 zoneId) : _zoneId(zoneId) {}

	void Add(ChatSessionRef session) { _sessions.insert(session); }
	void Remove(ChatSessionRef session) { _sessions.erase(session); }
	void Broadcast(SendBufferRef sendBuffer)
	{
		for (auto& session : _sessions)
			session->Send(sendBuffer);
	}

private:
	int32 _zoneId;
	Set<ChatSessionRef> _sessions;
};


#pragma once
#include "JobQueue.h"

class Room : public JobQueue
{
public:
	void Update();
	// 싱글스레드 환경인마냥 코딩
	void Broadcast(SendBufferRef sendBuffer);

private:
	map<uint64, PlayerRef> _players;
};

extern shared_ptr<Room> GRoom;

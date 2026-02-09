#include "pch.h"
#include "GameSession.h"
#include "ServerPacketHandler.h"


Atomic<int32> GameSession::DummyIdGenerator = 1;

void GameSession::OnConnected()
{
	Protocol::C_AuthToken packet;
	packet.set_token("user" + std::to_string(_dummyId));
	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
	Send(sendBuffer);
}

void GameSession::OnDisconnected()
{
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	ServerPacketHandler::HandlePacket(session, buffer, len);
}

void GameSession::OnSend(int32 len)
{
}

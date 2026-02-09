#include "pch.h"
#include "ChatSession.h"
#include "ServerPacketHandler.h"
#include "DummyUser.h"

Atomic<int32> ChatSession::DummyIdGenerator = 1;

void ChatSession::OnConnected()
{
	Protocol::C_ChatLogin packet;
	packet.set_playerid(GetOwner()->GetObjectId());
	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
	Send(sendBuffer);
}

void ChatSession::OnDisconnected()
{
}

void ChatSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	ServerPacketHandler::HandlePacket(session, buffer, len);
}

void ChatSession::OnSend(int32 len)
{
}

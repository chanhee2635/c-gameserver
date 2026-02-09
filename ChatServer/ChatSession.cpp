#include "pch.h"
#include "ChatSession.h"
#include "SessionManager.h"
#include "ClientPacketHandler.h"

void ChatSession::OnConnected()
{
}

void ChatSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	ClientPacketHandler::HandlePacket(session, buffer, len);
}

void ChatSession::OnSend(int32 len)
{
}
 
void ChatSession::OnDisconnected()
{
	GSessionManager->Remove(static_pointer_cast<ChatSession>(shared_from_this()));
}

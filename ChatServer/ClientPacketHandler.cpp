#include "pch.h"
#include "ClientPacketHandler.h"
#include "RedisManager.h"
#include "SessionManager.h"
#include "ChatSession.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	return false;
}

bool C_ChatLoginHandler(PacketSessionRef& session, Protocol::C_ChatLogin& pkt)
{
	ChatSessionRef chatSession = static_pointer_cast<ChatSession>(session);

	Protocol::S_ChatLogin packet;
	bool success = GRedisManager->CheckPlayer(pkt.playerid(), chatSession);
	packet.set_success(success);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);
	session->Send(sendBuffer);

	if (success)
	{
		chatSession->SetPlayerId(pkt.playerid());
		GSessionManager->Add(chatSession);
	}
	else
		session->Disconnect(L"Login Failed");

	return true;
}

bool C_ChatHandler(PacketSessionRef& session, Protocol::C_Chat& pkt)
{
	ChatSessionRef chatSession = static_pointer_cast<ChatSession>(session);

	if (chatSession->GetPlayerId() == 0) return false;

	if (Utils::Trim(pkt.msg()).empty()) return false;

	Protocol::S_Chat packet;
	packet.set_playerid(chatSession->GetPlayerId());
	packet.set_toserver(pkt.toserver());
	packet.set_name(chatSession->GetName());
	packet.set_msg(pkt.msg());
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);

	// 일반, 서버를 나눠 진행 (TODO: 귓속말)
	if (pkt.toserver())
		GSessionManager->Broadcast(sendBuffer);
	else
	{
		int32 zoneId = chatSession->GetZoneId();
		if (zoneId >= 0)
			GSessionManager->BroadcastToAdjacent(zoneId, sendBuffer);
	}

	return true;
}
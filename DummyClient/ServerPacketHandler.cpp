#include "pch.h"
#include "ServerPacketHandler.h"
#include "GameSession.h"
#include "ChatSession.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	return false;
}

bool S_LoginAuthHandler(PacketSessionRef& session, Protocol::S_LoginAuth& pkt)
{
	return true;
}

bool S_JoinHandler(PacketSessionRef& session, Protocol::S_Join& pkt)
{
	return true;
}

bool S_GameConnectedHandler(PacketSessionRef& session, Protocol::S_GameConnected& pkt)
{
	return true;
}

bool S_PlayerListHandler(PacketSessionRef& session, Protocol::S_PlayerList& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	if (pkt.summaries_size() == 0)
	{
		return false;
	}

	Protocol::SummaryInfo info = pkt.summaries(0);
	Protocol::C_EnterGame packet;
	packet.set_playername(info.name());
	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
	gameSession->Send(sendBuffer);

	return true;
}

bool S_CreatePlayerHandler(PacketSessionRef& session, Protocol::S_CreatePlayer& pkt)
{
	return true;
}

bool S_EnterGameHandler(PacketSessionRef& session, Protocol::S_EnterGame& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	gameSession->SetObjectInfo(pkt.myplayer());

	// 채팅 서버 접속
	gameSession->GetOwner()->ConnectToChat();

	Protocol::C_LoadCompleted packet;
	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
	gameSession->Send(sendBuffer);

	return true;
}

bool S_SpawnHandler(PacketSessionRef& session, Protocol::S_Spawn& pkt)
{
	return true;
}

bool S_DespawnHandler(PacketSessionRef& session, Protocol::S_Despawn& pkt)
{
	return true;
}

bool S_MoveHandler(PacketSessionRef& session, Protocol::S_Move& pkt)
{
	return true;
}

bool S_AttackHandler(PacketSessionRef& session, Protocol::S_Attack& pkt)
{
	return true;
}

bool S_DieHandler(PacketSessionRef& session, Protocol::S_Die& pkt)
{
	return true;
}

bool S_ChangeHpHandler(PacketSessionRef& session, Protocol::S_ChangeHp& pkt)
{
	return true;
}

bool S_ChangeExpHandler(PacketSessionRef& session, Protocol::S_ChangeExp& pkt)
{
	return true;
}

bool S_ChangeStatHandler(PacketSessionRef& session, Protocol::S_ChangeStat& pkt)
{
	return true;
}

bool S_ChangeStateHandler(PacketSessionRef& session, Protocol::S_ChangeState& pkt)
{
	return true;
}

bool S_ChangeLevelHandler(PacketSessionRef& session, Protocol::S_ChangeLevel& pkt)
{
	return true;
}

bool S_ReviveHandler(PacketSessionRef& session, Protocol::S_Revive& pkt)
{
	return true;
}

bool S_ChatLoginHandler(PacketSessionRef& session, Protocol::S_ChatLogin& pkt)
{
	ChatSessionRef chatSession = static_pointer_cast<ChatSession>(session);

	chatSession->GetOwner()->ConnectCompleted();

	return true;
}

bool S_ChatHandler(PacketSessionRef& session, Protocol::S_Chat& pkt)
{
	return true;
}

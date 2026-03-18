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

bool S_PlayerListHandler(PacketSessionRef& session, Protocol::S_PlayerList& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	if (pkt.players_size() == 0)
	{
		Protocol::C_CreatePlayer createPacket;
		createPacket.set_name("Dummy_" + to_string(gameSession->_dummyId));
		createPacket.set_template_id(rand() % 2 + 1);
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(createPacket);
		gameSession->Send(sendBuffer);
		return true;
	}

	const Protocol::PlayerSummary& info = pkt.players(0);
	Protocol::C_EnterGame packet;
	packet.set_name(info.name());
	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
	gameSession->Send(sendBuffer);

	return true;
}

bool S_CreatePlayerHandler(PacketSessionRef& session, Protocol::S_CreatePlayer& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	if (!pkt.success()) return false;

	Protocol::C_EnterGame packet;
	packet.set_name(pkt.player().name());
	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
	gameSession->Send(sendBuffer);

	return true;
}

bool S_EnterGameHandler(PacketSessionRef& session, Protocol::S_EnterGame& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	gameSession->SetObjectInfo(pkt.my_player());

	Protocol::C_LoadCompleted packet;
	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
	gameSession->Send(sendBuffer);

	return true;
}

bool S_UpdateSceneHandler(PacketSessionRef& session, Protocol::S_UpdateScene& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	DummyUserRef owner = gameSession->GetOwner();
	if (owner && !owner->IsChatConnected()) 
		owner->ConnectToChat();

	return true;
}

bool S_ReviveHandler(PacketSessionRef& session, Protocol::S_Revive& pkt)
{
	return false;
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


bool S_ChangeLevelHandler(PacketSessionRef& session, Protocol::S_ChangeLevel& pkt)
{
	return true;
}

bool S_ChatLoginHandler(PacketSessionRef& session, Protocol::S_ChatLogin& pkt)
{
	ChatSessionRef chatSession = static_pointer_cast<ChatSession>(session);

	DummyUserRef owner = chatSession->GetOwner();
	if (owner == nullptr) return false;

	owner->ConnectCompleted();

	return true;
}

bool S_ChatHandler(PacketSessionRef& session, Protocol::S_Chat& pkt)
{
	return true;
}

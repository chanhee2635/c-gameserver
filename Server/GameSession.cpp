#include "pch.h"
#include "GameSession.h"
#include "ClientPacketHandler.h"
#include "GameScene.h"
#include "Player.h"
#include "World.h"
#include "DBManager.h"

void GameSession::OnConnected() {}

void GameSession::OnDisconnected()
{
	PlayerRef player = GetPlayer();
	if (player == nullptr) return;

	GWorld->DoAsync([player]() {
		GWorld->LeaveCreature(player);
	});

	_player = nullptr;
	_summaries.clear();
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	ClientPacketHandler::HandlePacket(session, buffer, len);
}

void GameSession::OnSend(int32 len)
{
}
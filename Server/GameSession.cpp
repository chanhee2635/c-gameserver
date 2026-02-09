#include "pch.h"
#include "GameSession.h"
#include "ClientPacketHandler.h"
#include "GameScene.h"
#include "Player.h"
#include "World.h"
#include "DBManager.h"

void GameSession::OnConnected()
{
	// GameConnected 패킷을 전송
	Protocol::S_GameConnected pkt;
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
	Send(sendBuffer);
}

void GameSession::OnDisconnected()
{
	if (_player)
	{
		uint64 userDbId = _player->GetPlayerDbId();
		StatData stat = _player->GetStat();
		GDBManager->DoAsyncPush(&DBManager::SavePlayerInfo, userDbId, stat);
		GWorld->LeaveCreature(_player);
	}

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
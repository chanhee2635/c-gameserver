#include "pch.h"
#include "ClientPacketHandler.h"
#include "Player.h"
#include "GameSession.h"
#include "GameScene.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	return false;
}

bool C_AuthTokenHandler(PacketSessionRef& session, Protocol::C_AuthToken& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	gameSession->Handle_C_AuthToken(pkt.token());
	return true;
}

bool C_CreatePlayerHandler(PacketSessionRef& session, Protocol::C_CreatePlayer& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	gameSession->Handle_C_CreatePlayer(pkt.templateid(), pkt.name());
	return true;
}

bool C_EnterGameHandler(PacketSessionRef& session, Protocol::C_EnterGame& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	gameSession->Handle_C_EnterGame(Utils::s2ws(pkt.playername()));
	return true;
}

bool C_LoadCompletedHandler(PacketSessionRef& session, Protocol::C_LoadCompleted& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	gameSession->Handle_C_LoadCompleted();
	return true;
}

bool C_MoveHandler(PacketSessionRef& session, Protocol::C_Move& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	
	PlayerRef player = gameSession->GetPlayer();
	if (player == nullptr) return false;

	GameSceneRef scene = player->GetGameScene();
	if (scene == nullptr) return false;

	scene->DoAsync(&GameScene::HandlePlayerMove, player, pkt.pos());

	return true;
}

bool C_AttackHandler(PacketSessionRef& session, Protocol::C_Attack& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	PlayerRef player = gameSession->GetPlayer();
	if (player == nullptr) return false;

	GameSceneRef scene = player->GetGameScene();
	if (scene == nullptr) return false;

	float yaw = pkt.yaw();
	int32 comboIndex = pkt.comboindex();
	scene->DoAsync([player, yaw, comboIndex]() {
		player->HandleAttack(yaw, comboIndex);
	}); 

	return true;
}

bool C_ReviveHandler(PacketSessionRef& session, Protocol::C_Revive& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	PlayerRef player = gameSession->GetPlayer();
	if (player == nullptr || player->IsDead() == false) return false;

	GameSceneRef scene = player->GetGameScene();
	if (scene == nullptr) return false;

	player->GetGameScene()->DoAsync(&GameScene::HandleRevive, player, pkt.iscurrentpos());

	return true;
}

bool C_QuitHandler(PacketSessionRef& session, Protocol::C_Quit& pkt)
{
	session->Disconnect(L"QuitButton");

	return true;
}
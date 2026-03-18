#include "pch.h"
#include "ClientPacketHandler.h"
#include "GameSession.h"
#include "Player.h"
#include "GameScene.h"
#include "DBManager.h"

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
	gameSession->Handle_C_CreatePlayer(pkt.template_id(), pkt.name());
	return true;
}

bool C_EnterGameHandler(PacketSessionRef& session, Protocol::C_EnterGame& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	gameSession->Handle_C_EnterGame(Utils::s2ws(pkt.name()));
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
	gameSession->Handle_C_Move(pkt);
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
	int32 comboIndex = pkt.combo_index();
	Vector3 clientPos = GameUtil::ToServer(pkt.pos());

	scene->DoAsync([player, yaw, comboIndex, clientPos]() {
		player->HandleAttack(yaw, comboIndex, clientPos);
	}); 

	return true;
}

bool C_ReviveHandler(PacketSessionRef& session, Protocol::C_Revive& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	PlayerRef player = gameSession->GetPlayer();
	if (player == nullptr || !player->IsDead()) return false;

	GameSceneRef scene = player->GetGameScene();
	if (scene == nullptr) return false;

	bool isCurrentPos = pkt.is_current_pos();
	scene->DoAsync([scene, player, isCurrentPos]() {
		scene->HandleRevive(player, isCurrentPos);
	});

	return true;
}

bool C_QuitHandler(PacketSessionRef& session, Protocol::C_Quit& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	PlayerRef player = gameSession->GetPlayer();

	if (player == nullptr)
	{
		session->Disconnect(L"Quit");
		return true;
	}

	uint64  dbId = player->GetPlayerDbId();
	int32   hp = player->GetHp();
	int32   mp = player->GetMp();     
	int64   exp = player->GetExp();   
	Vector3 pos = player->GetPos();
	float   yaw = player->GetYaw();

	GDBManager->DoAsync([dbId, hp, mp, exp, pos, yaw, session]() {
		GDBManager->SavePlayerInfo(dbId, hp, mp, exp, pos, yaw);
		session->Disconnect(L"Quit");  
	});

	return true;
}
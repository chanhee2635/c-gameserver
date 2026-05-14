#include "pch.h"
#include "GameSession.h"
#include "ClientPacketHandler.h"
#include "SessionManager.h"
#include "Player.h"
#include "DBManager.h"
#include "World.h"

void GameSession::OnConnected()
{
    GServerStats->game.connectedSessions.fetch_add(1, std::memory_order_relaxed);
    GServerStats->game.totalConnections.fetch_add(1, std::memory_order_relaxed);
}

void GameSession::OnDisconnected()
{
    if (_dbId != 0)
        GSessionManager->Unregister(_dbId);

    // C_LEAVE_GAME 없이 끊긴 경우 (강제 종료, 네트워크 단절 등) 저장 + 퇴장 처리
    PlayerRef player = GetPlayer();
    if (player)
    {
        _player.reset();   // 중복 처리 방지

        uint64  dbId = player->GetPlayerDbId();
        int32   hp   = player->GetHp();
        int32   mp   = player->GetMp();
        int64   exp  = player->GetExp();
        Vector3 pos  = player->GetPos();
        float   yaw  = player->GetYaw();

        GDBManager->DoAsync([dbId, hp, mp, exp, pos, yaw]()
        {
            GDBManager->SavePlayerInfo(dbId, hp, mp, exp, pos, yaw);
        });

        GWorld->DoAsync([player]()
        {
            GWorld->LeaveCreature(player);
        });
    }

    GServerStats->game.connectedSessions.fetch_sub(1, std::memory_order_relaxed);
    GServerStats->game.totalDisconnections.fetch_add(1, std::memory_order_relaxed);
}

void GameSession::OnRecvPacket(std::span<const BYTE> packet, uint16 type)
{
    if (_dbId == 0 && type != Protocol::MsgId::C_AUTH_TOKEN)
    {
        LOG_WARN(L"미인증 세션 패킷 수신 type=" + std::to_wstring(type));
        Disconnect();
        return;
    }

    ClientPacketHandler::Handle(
        std::static_pointer_cast<GameSession>(shared_from_this()),
        packet, type);
}

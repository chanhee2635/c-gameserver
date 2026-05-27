#include "pch.h"
#include "GameSession.h"
#include "ClientPacketHandler.h"
#include "SessionManager.h"
#include "Player.h"
#include "DBManager.h"
#include "World.h"
#include "RedisManager.h"

void GameSession::OnConnected()
{
    GServerStats->game.connectedSessions.fetch_add(1, std::memory_order_relaxed);
    GServerStats->game.totalConnections.fetch_add(1, std::memory_order_relaxed);
}

void GameSession::OnDisconnected()
{
    if (_dbId != 0)
    {
        GSessionManager->Unregister(_dbId, this);
        GRedisManager->UpdateSessionCount(-1);
    }

    LeavePlayer();

    GServerStats->game.connectedSessions.fetch_sub(1, std::memory_order_relaxed);
}

void GameSession::OnRecvPacket(std::span<const BYTE> packet, uint16 type)
{
    if (_dbId == 0 && type != Protocol::MsgId::C_AUTH_TOKEN)
    {
        LOG_WARN(L"Received unauthenticated session packet type=" + std::to_wstring(type));
        Disconnect();
        return;
    }

    ClientPacketHandler::Handle(
        std::static_pointer_cast<GameSession>(shared_from_this()),
        packet, type);
}

void GameSession::LeavePlayer()
{
    PlayerRef player = GetPlayer();
    if (!player) return;

    SetPlayer(nullptr);

    uint64  dbId = player->GetPlayerDbId();
    int32   level = player->GetLevel();
    int32   hp   = player->GetHp();
    int32   mp   = player->GetMp();
    int64   exp  = player->GetExp();
    Vector3 pos  = player->GetPos();
    float   yaw  = player->GetYaw();

    GDBManager->DoAsync([dbId, level, hp, mp, exp, pos, yaw]()
    {
        GDBManager->SavePlayerLevelUp(dbId, level, hp, mp, exp, pos, yaw);
    });

    GWorld->DoAsync([player]()
    {
        GWorld->LeaveCreature(player);
    });
}

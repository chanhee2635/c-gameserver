#include "pch.h"
#include "GameSession.h"
#include "ClientPacketHandler.h"
#include "SessionManager.h"
#include "Player.h"
#include "DBManager.h"
#include "World.h"
#include "RedisManager.h"
#include "GameGlobal.h"

namespace
{
    constexpr uint64 AUTH_HANDSHAKE_TIMEOUT_MS = 10000;

    constexpr uint64 RATE_WINDOW_MS      = 1000;
    constexpr uint32 MAX_PACKETS_PER_SEC = 50;
}

bool GameSession::AllowRecv()
{
    const uint64 now = ::GetTickCount64();
    if (now - _rateWindowStart >= RATE_WINDOW_MS)
    {
        _rateWindowStart = now;
        _rateWindowCount = 0;
    }
    return (++_rateWindowCount <= MAX_PACKETS_PER_SEC);
}

bool GameSession::AllowChat(uint64 cooldownMs)
{
    const uint64 now = ::GetTickCount64();
    if (now - _lastChatTick < cooldownMs)
        return false;
    _lastChatTick = now;
    return true;
}

void GameSession::OnConnected()
{
    GServerStats->game.connectedSessions.fetch_add(1, std::memory_order_relaxed);
    GServerStats->game.totalConnections.fetch_add(1, std::memory_order_relaxed);

    GameSessionWeakRef weak = std::static_pointer_cast<GameSession>(shared_from_this());
    if (GWorld)
    {
        GWorld->DoTimer(AUTH_HANDSHAKE_TIMEOUT_MS, [weak]()
        {
            GameSessionRef s = weak.lock();
            if (!s) return;
            if (s->IsConnected() && s->GetDbId() == 0)
            {
                LOG_WARN(L"Auth handshake timeout, closing session addr=" + s->GetNetAddress().GetIpAddress());
                s->Disconnect();
            }
        });
    }
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
    if (!AllowRecv())
    {
        if (GServerStats)
            GServerStats->network.droppedPackets.fetch_add(1, std::memory_order_relaxed);
        LOG_WARN(L"Inbound rate limit exceeded, closing session addr=" + GetNetAddress().GetIpAddress());
        Disconnect();
        return;
    }

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
    ResetEnterGame();           
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

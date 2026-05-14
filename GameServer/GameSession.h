#pragma once
#include "Session.h"
#include "Protocol.pb.h"

class GameSession : public PacketSession
{
public:
    GameSession() = default;
    ~GameSession() = default;

    virtual void OnConnected() override;
    virtual void OnDisconnected() override;
    virtual void OnRecvPacket(std::span<const BYTE> packet, uint16 type) override;

    // ── Account (set on C_AUTH_TOKEN) ──────────────────────────────────────
    void    SetDbId(uint64 dbId)      { _dbId = dbId; }
    uint64  GetDbId()          const  { return _dbId; }

    // ── Active player (set on C_ENTER_GAME) ────────────────────────────────
    void      SetPlayerDbId(uint64 id)  { _playerDbId = id; }
    uint64    GetPlayerDbId()    const  { return _playerDbId; }

    void      SetPlayer(PlayerRef player) { _player = player; }
    PlayerRef GetPlayer()          const  { return _player.lock(); }

private:
    uint64        _dbId       = 0;   // account DB id
    uint64        _playerDbId = 0;   // selected player DB id (0 = not in-game)
    PlayerWeakRef _player;           // 게임 입장 후 연결된 플레이어 (0 = 로비/미입장)
};

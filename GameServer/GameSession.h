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

    void    SetDbId(uint64 dbId)      { _dbId = dbId; }
    uint64  GetDbId()          const  { return _dbId; }

    void      SetPlayerDbId(uint64 id)  { _playerDbId = id; }
    uint64    GetPlayerDbId()    const  { return _playerDbId; }

    void      SetPlayer(PlayerRef player) { _player = player; }
    PlayerRef GetPlayer()          const  { return _player.lock(); }

    bool TryBeginEnterGame() { bool e = false; return _enterGameLatched.compare_exchange_strong(e, true); }
    void ResetEnterGame()    { _enterGameLatched.store(false); }

    bool AllowRecv();                  // false => over packet-rate limit
    bool AllowChat(uint64 cooldownMs); // false => chat sent too soon

    void LeavePlayer();

private:
    uint64        _dbId       = 0;   // account DB id
    uint64        _playerDbId = 0;   // selected player DB id (0 = not in-game)
    PlayerWeakRef _player;           // 게임 입장 후 연결된 플레이어 (0 = 로비/미입장)
    std::atomic<bool> _enterGameLatched = false;   // one in-flight/active enter per session

    uint64 _rateWindowStart = 0;     // current 1s window start tick
    uint32 _rateWindowCount = 0;     // packets received in the current window
    uint64 _lastChatTick    = 0;     // last accepted chat tick
};

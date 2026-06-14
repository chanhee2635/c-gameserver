#pragma once
#include "Session.h"

class DummySession : public PacketSession
{
public:
    DummySession() = default;
    ~DummySession() = default;

    virtual void OnConnected()    override;
    virtual void OnDisconnected() override;
    virtual void OnRecvPacket(std::span<const BYTE> packet, uint16 type) override;
    virtual void OnSend(uint32 len) override;

    void          SetAuthToken(const string& token) { _authToken = token; }
    void          SetAccountIdx(int32 idx) { _accountIdx = idx; }  
    int32         GetAccountIdx() const { return _accountIdx; } 
    void          SetPlayerName(const string& name) { _playerName = name; }
    const string& GetPlayerName() const { return _playerName; }

    void    SetSpawnPos(Vector3 pos) { _spawnPos = pos; }   
    Vector3 GetSpawnPos() const { return _spawnPos; }  

    void SetCorrection(Vector3 pos) { LockGuard g(_corrLock); _corrPos = pos; _hasCorr = true; }
    bool TakeCorrection(Vector3& out) { LockGuard g(_corrLock); if (!_hasCorr) return false; out = _corrPos; _hasCorr = false; return true; }

private:
    int32   _accountIdx = -1;
    string  _authToken;
    string  _playerName;
    Vector3 _spawnPos = {};  

    Mutex   _corrLock;
    Vector3 _corrPos = {};
    bool    _hasCorr = false;
};


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
    void          SetPlayerName(const string& name) { _playerName = name; }
    const string& GetPlayerName() const { return _playerName; }

    void    SetSpawnPos(Vector3 pos) { _spawnPos = pos; }   
    Vector3 GetSpawnPos() const { return _spawnPos; }  

private:
    string  _authToken;
    string  _playerName;
    Vector3 _spawnPos = {};  
};


#pragma once
#include "Session.h"

enum class ESessionState
{
    IDLE,
    AUTH,
    PLAYER_LIST,
    IN_GAME,
};

class DummySession : public PacketSession
{
public:
    DummySession() = default;
    ~DummySession() = default;

    virtual void OnConnected() override;
    virtual void OnDisconnected() override;
    virtual void OnRecvPacket(std::span<const BYTE> packet, uint16 type) override;
    virtual void OnSend(uint32 len) override;

    void SetAuthToken(const string& token) { _authToken = token; }

private:
    ESessionState _state = ESessionState::IDLE;
    string _authToken;
};

using DummySessionRef = std::shared_ptr<DummySession>;
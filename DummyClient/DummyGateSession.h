#pragma once
#include "Session.h"

class DummyGateSession : public PacketSession
{
public:
    virtual void OnConnected()    override;
    virtual void OnDisconnected() override {}
    virtual void OnRecvPacket(std::span<const BYTE> packet, uint16 type) override;
    virtual void OnSend(uint32 len) override {}

    void SetQueueToken(const string& t) { _queueToken = t; }
    void SetServerId(int32 id) { _serverId = id; }
    void SetAccountIdx(int32 idx) { _accountIdx = idx; }

private:
    string _queueToken;
    int32  _serverId = 1;
    int32  _accountIdx = -1;
};
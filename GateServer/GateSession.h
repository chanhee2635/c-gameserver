#pragma once
#include "Session.h"
#include "GatePacket.pb.h"

class GateSession;
using GateSessionRef = std::shared_ptr<GateSession>;
using GateSessionWeakRef = std::weak_ptr<GateSession>;

class GateSession : public PacketSession
{
public:
    GateSession() = default;
    ~GateSession() = default;

    virtual void OnConnected()    override {}
    virtual void OnDisconnected() override {}   // 큐 제거는 QueueManager Tick이 담당
    virtual void OnRecvPacket(std::span<const BYTE> packet, uint16 type) override;

    GateSessionRef GetGateRef() { return std::static_pointer_cast<GateSession>(shared_from_this()); }

private:
    void Handle_CQEnter(const Protocol::CQEnter& pkt);
    void Handle_CQLeave(const Protocol::CQLeave& pkt);

    uint64 _accountId = 0;
    int32  _serverId = 0;
    bool   _enqueued = false;
};
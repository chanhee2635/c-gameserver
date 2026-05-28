#include "pch.h"
#include "GateSession.h"
#include "PacketUtils.h"
#include "GateGlobal.h"
#include "GateRedis.h"
#include "QueueManager.h"

void GateSession::OnRecvPacket(std::span<const BYTE> packet, uint16 type)
{
    const BYTE* body = packet.data() + sizeof(PacketHeader);
    const int   bodySize = static_cast<int>(packet.size() - sizeof(PacketHeader));

    switch (type)
    {
    case Protocol::C_Q_ENTER:
    {
        Protocol::CQEnter pkt;
        if (pkt.ParseFromArray(body, bodySize)) Handle_CQEnter(pkt);
        break;
    }
    case Protocol::C_Q_LEAVE:
    {
        Protocol::CQLeave pkt;
        if (pkt.ParseFromArray(body, bodySize)) Handle_CQLeave(pkt);
        break;
    }
    default:
        LOG_WARN(L"GateSession unknown packet type=" + std::to_wstring(type));
        Disconnect();
        break;
    }
}

void GateSession::Handle_CQEnter(const Protocol::CQEnter& pkt)
{
    if (_enqueued) return;   // 중복 등록 방지

    auto accountId = GGateRedis->ResolveQueueToken(pkt.queue_token());
    if (!accountId)
    {
        Protocol::SQEnter res;
        res.set_success(false);
        res.set_reason("invalid queue token");
        Send(MakeSendBuffer<Protocol::S_Q_ENTER>(res));
        Disconnect();
        return;
    }

    ServerSlot slot;
    if (!GGateRedis->GetServerInfo(pkt.server_id(), slot))
    {
        Protocol::SQEnter res;
        res.set_success(false);
        res.set_reason("server unavailable");
        Send(MakeSendBuffer<Protocol::S_Q_ENTER>(res));
        Disconnect();
        return;
    }

    _accountId = *accountId;
    _serverId = pkt.server_id();
    _enqueued = true;

    Protocol::SQEnter res;
    res.set_success(true);
    Send(MakeSendBuffer<Protocol::S_Q_ENTER>(res));

    GQueueManager->Enqueue(GetGateRef(), _accountId, _serverId);
}

void GateSession::Handle_CQLeave(const Protocol::CQLeave& /*pkt*/)
{
    Disconnect();   // 끊으면 다음 Tick이 큐에서 제거
}
#include "pch.h"
#include "DummyGateSession.h"
#include "DummySession.h"
#include "DummyNet.h"
#include "PacketUtils.h"
#include "GatePacket.pb.h"

void DummyGateSession::OnConnected()
{
    Protocol::CQEnter pkt;
    pkt.set_queue_token(_queueToken);
    pkt.set_server_id(_serverId);
    Send(MakeSendBuffer<Protocol::C_Q_ENTER>(pkt));
}

void DummyGateSession::OnRecvPacket(std::span<const BYTE> packet, uint16 type)
{
    const BYTE* body = packet.data() + sizeof(PacketHeader);
    const int   len = static_cast<int>(packet.size() - sizeof(PacketHeader));

    switch (type)
    {
    case Protocol::S_Q_ENTER:
        break;   // 등록 ack, 무시

    case Protocol::S_Q_STATUS:
    {
        Protocol::SQStatus s;
        if (s.ParseFromArray(body, len))
        {
            // 필요하면 로깅 (대량일 땐 끄세요)
            // LOG_INFO(L"queue pos=" + std::to_wstring(s.position()) + L"/" + std::to_wstring(s.total()));
        }
        break;
    }

    case Protocol::S_Q_ADMITTED:
    {
        Protocol::SQAdmitted s;
        if (s.ParseFromArray(body, len))
        {
            auto game = MakeShared<DummySession>();
            game->SetAuthToken(s.auth_token());
            game->SetAccountIdx(_accountIdx);
            GDummyNet->ConnectGame(game);   // 게임서버로 전환
        }
        Disconnect();   // 게이트웨이 연결 종료
        break;
    }

    default:
        break;
    }
}
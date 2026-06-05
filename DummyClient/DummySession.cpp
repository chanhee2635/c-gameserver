#include "pch.h"
#include "DummySession.h"
#include "ServerPacketHandler.h"

void DummySession::OnConnected()
{
    Protocol::CAuthToken pkt;
    pkt.set_auth_token(_authToken);
    auto sendBuffer = MakeSendBuffer<Protocol::C_AUTH_TOKEN>(pkt);
    Send(sendBuffer);
}

void DummySession::OnDisconnected()
{
}

void DummySession::OnRecvPacket(std::span<const BYTE> packet, uint16 type)
{
    switch (type)
    {
    case Protocol::S_PLAYER_LIST:
    case Protocol::S_CREATE_PLAYER:
    case Protocol::S_ENTER_GAME:
    case Protocol::S_READY_TO_ENTER:
        ServerPacketHandler::Handle(static_pointer_cast<DummySession>(shared_from_this()), packet, type);
        break;
    default:
        break;   // ³ª¸ÓÁø ÆÄ½Ì X 
    }
}

void DummySession::OnSend(uint32 len)
{
}
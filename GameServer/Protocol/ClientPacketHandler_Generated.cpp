// AUTO-GENERATED -- DO NOT EDIT MANUALLY
#include "pch.h"
#include "ClientPacketHandler.h"

bool ClientPacketHandler::Handle(GameSessionRef session, std::span<const BYTE> packet, uint16 type)
{
    switch (type)
    {
    case Protocol::C_AUTH_TOKEN: return HandlePacket<Protocol::CAuthToken, OnHandle_C_AUTH_TOKEN>(session, packet);
    case Protocol::C_CREATE_PLAYER: return HandlePacket<Protocol::CCreatePlayer, OnHandle_C_CREATE_PLAYER>(session, packet);
    case Protocol::C_LOAD_COMPLETED: return HandlePacket<Protocol::CLoadCompleted, OnHandle_C_LOAD_COMPLETED>(session, packet);
    case Protocol::C_ENTER_GAME: return HandlePacket<Protocol::CEnterGame, OnHandle_C_ENTER_GAME>(session, packet);
    case Protocol::C_LEAVE_GAME: return HandlePacket<Protocol::CLeaveGame, OnHandle_C_LEAVE_GAME>(session, packet);
    case Protocol::C_MOVE: return HandlePacket<Protocol::CMove, OnHandle_C_MOVE>(session, packet);
    case Protocol::C_ATTACK: return HandlePacket<Protocol::CAttack, OnHandle_C_ATTACK>(session, packet);
    case Protocol::C_REVIVE: return HandlePacket<Protocol::CRevive, OnHandle_C_REVIVE>(session, packet);
    case Protocol::C_CHAT: return HandlePacket<Protocol::CChat, OnHandle_C_CHAT>(session, packet);
    default:
        LOG_WARN(L"Unknown packet type=" + std::to_wstring(type));
        return false;
    }
}

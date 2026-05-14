// AUTO-GENERATED -- DO NOT EDIT MANUALLY
#include "pch.h"
#include "ServerPacketHandler.h"

bool ServerPacketHandler::Handle(DummySessionRef session, std::span<const BYTE> packet, uint16 type)
{
    switch (type)
    {
    case Protocol::S_PLAYER_LIST: return HandlePacket<Protocol::SPlayerList, OnHandle_S_PLAYER_LIST>(session, packet);
    case Protocol::S_CREATE_PLAYER: return HandlePacket<Protocol::SCreatePlayer, OnHandle_S_CREATE_PLAYER>(session, packet);
    case Protocol::S_ENTER_GAME: return HandlePacket<Protocol::SEnterGame, OnHandle_S_ENTER_GAME>(session, packet);
    case Protocol::S_READY_TO_ENTER: return HandlePacket<Protocol::SReadyToEnter, OnHandle_S_READY_TO_ENTER>(session, packet);
    case Protocol::S_LEAVE_GAME: return HandlePacket<Protocol::SLeaveGame, OnHandle_S_LEAVE_GAME>(session, packet);
    case Protocol::S_UPDATE_SCENE: return HandlePacket<Protocol::SUpdateScene, OnHandle_S_UPDATE_SCENE>(session, packet);
    case Protocol::S_ATTACK: return HandlePacket<Protocol::SAttack, OnHandle_S_ATTACK>(session, packet);
    case Protocol::S_DIE: return HandlePacket<Protocol::SDie, OnHandle_S_DIE>(session, packet);
    case Protocol::S_REVIVE: return HandlePacket<Protocol::SRevive, OnHandle_S_REVIVE>(session, packet);
    case Protocol::S_CHANGE_HP: return HandlePacket<Protocol::SChangeHp, OnHandle_S_CHANGE_HP>(session, packet);
    case Protocol::S_CHANGE_EXP: return HandlePacket<Protocol::SChangeExp, OnHandle_S_CHANGE_EXP>(session, packet);
    case Protocol::S_CHANGE_LEVEL: return HandlePacket<Protocol::SChangeLevel, OnHandle_S_CHANGE_LEVEL>(session, packet);
    case Protocol::S_CHAT: return HandlePacket<Protocol::SChat, OnHandle_S_CHAT>(session, packet);
    default:
        LOG_WARN(L"Unknown packet type=" + std::to_wstring(type));
        return false;
    }
}

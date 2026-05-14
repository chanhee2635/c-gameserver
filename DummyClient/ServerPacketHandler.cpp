#include "pch.h"
#include "Protocol/ServerPacketHandler.h"

bool ServerPacketHandler::OnHandle_S_PLAYER_LIST(DummySessionRef session, const Protocol::SPlayerList& pkt)
{
    return true;
}
bool ServerPacketHandler::OnHandle_S_CREATE_PLAYER(DummySessionRef session, const Protocol::SCreatePlayer& pkt)
{
    return true;
}
bool ServerPacketHandler::OnHandle_S_ENTER_GAME(DummySessionRef session, const Protocol::SEnterGame& pkt)
{
    return true;
}
bool ServerPacketHandler::OnHandle_S_ATTACK(DummySessionRef session, const Protocol::SAttack& pkt)
{
    return true;
}
bool ServerPacketHandler::OnHandle_S_DIE(DummySessionRef session, const Protocol::SDie& pkt)
{
    return true;
}
bool ServerPacketHandler::OnHandle_S_CHANGE_HP(DummySessionRef session, const Protocol::SChangeHp& pkt)
{
    return true;
}
bool ServerPacketHandler::OnHandle_S_CHANGE_EXP(DummySessionRef session, const Protocol::SChangeExp& pkt)
{
    return true;
}
bool ServerPacketHandler::OnHandle_S_CHANGE_LEVEL(DummySessionRef session, const Protocol::SChangeLevel& pkt)
{
    return true;
}
bool ServerPacketHandler::OnHandle_S_UPDATE_SCENE(DummySessionRef session, const Protocol::SUpdateScene& pkt)
{
    return true;
}
bool ServerPacketHandler::OnHandle_S_REVIVE(DummySessionRef session, const Protocol::SRevive& pkt)
{
    return true;
}
bool ServerPacketHandler::OnHandle_S_CHAT_LOGIN(DummySessionRef session, const Protocol::SChatLogin& pkt)
{
    return true;
}
bool ServerPacketHandler::OnHandle_S_CHAT(DummySessionRef session, const Protocol::SChat& pkt)
{
    return true;
}

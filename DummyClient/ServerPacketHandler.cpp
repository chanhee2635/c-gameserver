#include "pch.h"
#include "Protocol/ServerPacketHandler.h"
#include "DummySimulator.h"

bool ServerPacketHandler::OnHandle_S_PLAYER_LIST(DummySessionRef session, const Protocol::SPlayerList& pkt)
{
    if (!pkt.success())
    {
        LOG_WARN(L"SPlayerList failed: " + Utils::ToWString(pkt.reason()));
        return false;
    }

    if (pkt.players_size() > 0)
    {
        const string& name = pkt.players(0).name();
        session->SetPlayerName(name);

        Protocol::CEnterGame enterPkt;
        enterPkt.set_name(name);
        session->Send(MakeSendBuffer<Protocol::C_ENTER_GAME>(enterPkt));
    }
    else
    {
        string name = "Dummy_" + std::to_string(session->GetAccountIdx());

        Protocol::CCreatePlayer createPkt;
        createPkt.set_name(name);
        createPkt.set_template_id(1);
        session->Send(MakeSendBuffer<Protocol::C_CREATE_PLAYER>(createPkt));
    }

    return true;
}
bool ServerPacketHandler::OnHandle_S_CREATE_PLAYER(DummySessionRef session, const Protocol::SCreatePlayer& pkt)
{
    if (!pkt.success())
    {
        LOG_WARN(L"Create Player failed: " + Utils::ToWString(pkt.reason()));
        return false;
    }

    const string& name = pkt.player().name();
    session->SetPlayerName(name);

    Protocol::CEnterGame enterPkt;
    enterPkt.set_name(name);
    session->Send(MakeSendBuffer<Protocol::C_ENTER_GAME>(enterPkt));

    return true;
}
bool ServerPacketHandler::OnHandle_S_ENTER_GAME(DummySessionRef session, const Protocol::SEnterGame& pkt)
{
    if (!pkt.success())
    {
        LOG_WARN(L"SEnterGame 실패");
        return false;
    }

    const auto& info = pkt.my_player();
    const auto& pos = info.pos_info().pos();

    session->SetSpawnPos({ pos.x(), pos.y(), pos.z() });
    return true;
}
bool ServerPacketHandler::OnHandle_S_READY_TO_ENTER(DummySessionRef session, const Protocol::SReadyToEnter& pkt)
{
    Protocol::CLoadCompleted loadPkt;
    session->Send(MakeSendBuffer<Protocol::C_LOAD_COMPLETED>(loadPkt));

    GDummySimulator->AddSession(session, session->GetSpawnPos());
    return true;
}
bool ServerPacketHandler::OnHandle_S_LEAVE_GAME(DummySessionRef session, const Protocol::SLeaveGame& pkt)
{
    return false;
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
bool ServerPacketHandler::OnHandle_S_CHAT(DummySessionRef session, const Protocol::SChat& pkt)
{
    return true;
}
bool ServerPacketHandler::OnHandle_S_TIME_SYNC(DummySessionRef session, const Protocol::STimeSync& pkt)
{
    return false;
}
bool ServerPacketHandler::OnHandle_S_MOVE_CORRECTION(DummySessionRef session, const Protocol::SMoveCorrection& pkt)
{
    if (session)
        session->SetCorrection(Vector3(pkt.pos().x(), pkt.pos().y(), pkt.pos().z()));
    return true;
}

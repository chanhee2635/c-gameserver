#pragma once
#include "PacketUtils.h"
#include "DummySession.h"

class ServerPacketHandler
{
public:
    static bool Handle(DummySessionRef session, std::span<const BYTE> packet, uint16 type);

    static bool OnHandle_S_PLAYER_LIST(DummySessionRef session, const Protocol::SPlayerList& pkt);
    static bool OnHandle_S_CREATE_PLAYER(DummySessionRef session, const Protocol::SCreatePlayer& pkt);
    static bool OnHandle_S_ENTER_GAME(DummySessionRef session, const Protocol::SEnterGame& pkt);
    static bool OnHandle_S_READY_TO_ENTER(DummySessionRef session, const Protocol::SReadyToEnter& pkt);
    static bool OnHandle_S_LEAVE_GAME(DummySessionRef session, const Protocol::SLeaveGame& pkt);
    static bool OnHandle_S_UPDATE_SCENE(DummySessionRef session, const Protocol::SUpdateScene& pkt);
    static bool OnHandle_S_ATTACK(DummySessionRef session, const Protocol::SAttack& pkt);
    static bool OnHandle_S_DIE(DummySessionRef session, const Protocol::SDie& pkt);
    static bool OnHandle_S_REVIVE(DummySessionRef session, const Protocol::SRevive& pkt);
    static bool OnHandle_S_CHANGE_HP(DummySessionRef session, const Protocol::SChangeHp& pkt);
    static bool OnHandle_S_CHANGE_EXP(DummySessionRef session, const Protocol::SChangeExp& pkt);
    static bool OnHandle_S_CHANGE_LEVEL(DummySessionRef session, const Protocol::SChangeLevel& pkt);
    static bool OnHandle_S_CHAT(DummySessionRef session, const Protocol::SChat& pkt);
    static bool OnHandle_S_TIME_SYNC(DummySessionRef session, const Protocol::STimeSync& pkt);
    static bool OnHandle_S_MOVE_CORRECTION(DummySessionRef session, const Protocol::SMoveCorrection& pkt);

private:
    template<typename MsgType, bool(*OnHandle)(DummySessionRef, const MsgType&)>
    static bool HandlePacket(DummySessionRef session, std::span<const BYTE> packet)
    {
        MsgType pkt;
        if (!pkt.ParseFromArray(
                packet.data() + sizeof(PacketHeader),
                static_cast<int>(packet.size() - sizeof(PacketHeader))))
            return false;
        return OnHandle(session, pkt);
    }
};

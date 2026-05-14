#pragma once
#include "PacketUtils.h"
#include "GameSession.h"

class ClientPacketHandler
{
public:
    static bool Handle(GameSessionRef session, std::span<const BYTE> packet, uint16 type);

    static bool OnHandle_C_AUTH_TOKEN(GameSessionRef session, const Protocol::CAuthToken& pkt);
    static bool OnHandle_C_CREATE_PLAYER(GameSessionRef session, const Protocol::CCreatePlayer& pkt);
    static bool OnHandle_C_LOAD_COMPLETED(GameSessionRef session, const Protocol::CLoadCompleted& pkt);
    static bool OnHandle_C_ENTER_GAME(GameSessionRef session, const Protocol::CEnterGame& pkt);
    static bool OnHandle_C_LEAVE_GAME(GameSessionRef session, const Protocol::CLeaveGame& pkt);
    static bool OnHandle_C_MOVE(GameSessionRef session, const Protocol::CMove& pkt);
    static bool OnHandle_C_ATTACK(GameSessionRef session, const Protocol::CAttack& pkt);
    static bool OnHandle_C_REVIVE(GameSessionRef session, const Protocol::CRevive& pkt);
    static bool OnHandle_C_CHAT(GameSessionRef session, const Protocol::CChat& pkt);

private:
    static bool IsAuthenticated(const GameSessionRef& session)
    {
        if (session->GetDbId() == 0)
        {
            LOG_WARN(L"Unauthenticated session - packet blocked");
            session->Disconnect();
            return false;
        }
        return true;
    }
    template<typename MsgType, bool(*OnHandle)(GameSessionRef, const MsgType&)>
    static bool HandlePacket(GameSessionRef session, std::span<const BYTE> packet)
    {
        MsgType pkt;
        if (!pkt.ParseFromArray(
                packet.data() + sizeof(PacketHeader),
                static_cast<int>(packet.size() - sizeof(PacketHeader))))
            return false;
        return OnHandle(session, pkt);
    }
};

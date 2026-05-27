#include "pch.h"
#include "ClientPacketHandler.h"
#include "DBManager.h"
#include "RedisManager.h"
#include "SessionManager.h"
#include "GameGlobal.h"
#include "World.h"
#include "GameScene.h"
#include "Player.h"
#include "GameUtil.h"

bool ClientPacketHandler::OnHandle_C_AUTH_TOKEN(GameSessionRef session, const Protocol::CAuthToken& pkt)
{
    if (session->GetDbId() != 0)
    {
        LOG_WARN(L"Duplicate auth try accountDbId=" + std::to_wstring(session->GetDbId()));
        return false;
    }

    auto accountId = GRedisManager->GetAccountId(pkt.auth_token());
    if (!accountId.has_value())
    {
        LOG_WARN(L"Token verification failed");
        session->Disconnect();
        return false;
    }

    session->SetDbId(accountId.value());
    GRedisManager->DeleteToken(pkt.auth_token());
    GSessionManager->Register(accountId.value(), session);
    GRedisManager->UpdateSessionCount(+1);

    if (!GDBManager)
    {
        Protocol::SPlayerList res;
        session->Send(MakeSendBuffer<Protocol::MsgId::S_PLAYER_LIST>(res));
        return true;
    }

    GDBManager->DoAsync([session, id = accountId.value()]()
        {
            Vector<PlayerSummaryData> summaries;
            bool ok = GDBManager->GetPlayerInfo(id, summaries);

            Protocol::SPlayerList res;
            res.set_success(ok);
            if (!ok)
            {
                res.set_reason("Failed to load player list");
            }
            else
            {
                for (const auto& summary : summaries)
                {
                    auto* playerEntry = res.add_players();
                    playerEntry->set_db_id(summary.dbId);
                    playerEntry->set_name(Utils::ToString(summary.name));
                    playerEntry->set_level(summary.level);
                    playerEntry->set_template_id(summary.templateId);
                }
            }
            session->Send(MakeSendBuffer<Protocol::MsgId::S_PLAYER_LIST>(res));
        });

    return true;
}

bool ClientPacketHandler::OnHandle_C_CREATE_PLAYER(GameSessionRef session, const Protocol::CCreatePlayer& pkt)
{
    if (!IsAuthenticated(session)) return false;

    uint64 accountDbId = session->GetDbId();
    int32       templateId = pkt.template_id();
    const string& name    = pkt.name();

    if (!GDBManager)
    {
        Protocol::SCreatePlayer res;
        res.set_success(false);
        res.set_reason("DB not available");
        session->Send(MakeSendBuffer<Protocol::MsgId::S_CREATE_PLAYER>(res));
        return true;
    }

    GDBManager->DoAsync([session, accountDbId, templateId, name]()
    {
        PlayerSummaryData summary;
        bool ok = GDBManager->CreatePlayer(accountDbId, templateId, name, summary);

        Protocol::SCreatePlayer res;
        res.set_success(ok);
        if (ok)
        {
            auto* playerEntry = res.mutable_player();
            playerEntry->set_db_id(summary.dbId);
            playerEntry->set_name(Utils::ToString(summary.name));
            playerEntry->set_level(summary.level);
            playerEntry->set_template_id(summary.templateId);
        }
        else
        {
            res.set_reason("Name already in use or creation failed");
        }
        session->Send(MakeSendBuffer<Protocol::MsgId::S_CREATE_PLAYER>(res));
    });

    return true;
}

bool ClientPacketHandler::OnHandle_C_LOAD_COMPLETED(GameSessionRef session, const Protocol::CLoadCompleted& pkt)
{
    if (!IsAuthenticated(session)) return false;

    return true;
}

bool ClientPacketHandler::OnHandle_C_ENTER_GAME(GameSessionRef session, const Protocol::CEnterGame& pkt)
{
    if (!IsAuthenticated(session)) return false;

    uint64        accountDbId = session->GetDbId();
    const wstring playerName  = Utils::ToWString(pkt.name());

    GDBManager->DoAsync([session, accountDbId, playerName]()
    {
        PlayerSummaryData summary;
        PlayerLoadData    loadData;
        bool ok = GDBManager->GetPlayerDetail(accountDbId, playerName, summary, loadData);

        if (!ok)
        {
            Protocol::SEnterGame res;
            res.set_success(false);
            session->Send(MakeSendBuffer<Protocol::MsgId::S_ENTER_GAME>(res));
            return false;
        }

        session->SetPlayerDbId(summary.dbId);

        GWorld->DoAsync([session, summary, loadData]() {
            GWorld->PlayerEnterToGame(session, summary, loadData);
        });
    });

    return true;
}

bool ClientPacketHandler::OnHandle_C_LEAVE_GAME(GameSessionRef session, const Protocol::CLeaveGame& pkt)
{
    if (!IsAuthenticated(session)) return false;

    session->LeavePlayer();

    return true;
}

bool ClientPacketHandler::OnHandle_C_MOVE(GameSessionRef session, const Protocol::CMove& pkt)
{
    if (!IsAuthenticated(session)) return false;

    PlayerRef player = session->GetPlayer();
    if (!player) return false;

    MoveJob job;
    job.objectId = player->GetObjectId();
    job.pos      = GameUtil::ToServer(pkt.pos_info().pos());
    job.velocity = GameUtil::ToServer(pkt.pos_info().velocity());
    job.yaw      = pkt.pos_info().yaw();
    job.state    = static_cast<Protocol::CreatureState>(pkt.pos_info().state());

    player->SetPendingMove(job);
    return true;
}

bool ClientPacketHandler::OnHandle_C_ATTACK(GameSessionRef session, const Protocol::CAttack& pkt)
{
    if (!IsAuthenticated(session)) return false;

    PlayerRef player = session->GetPlayer();
    if (!player) return false;

    GameSceneRef scene = player->GetGameScene();
    if (!scene) return false;

    float   yaw = pkt.yaw();
    int32   comboIndex = pkt.combo_index();
    Vector3 clientPos = GameUtil::ToServer(pkt.pos());

    scene->DoAsync(MakeJob([player, yaw, comboIndex, clientPos]()
    {
        player->HandleAttack(yaw, comboIndex, clientPos);
    }));

    return true;
}

bool ClientPacketHandler::OnHandle_C_REVIVE(GameSessionRef session, const Protocol::CRevive& pkt)
{
    if (!IsAuthenticated(session)) return false;

    PlayerRef player = session->GetPlayer();
    if (!player) return false;

    GameSceneRef scene = player->GetGameScene();
    if (!scene) return false;

    bool isCurrentPos = pkt.is_current_pos();
    scene->DoAsync(MakeJob([scene, player, isCurrentPos]()
    {
        scene->HandleRevive(player, isCurrentPos);
    }));

    return true;
}

bool ClientPacketHandler::OnHandle_C_CHAT(GameSessionRef session, const Protocol::CChat& pkt)
{
    if (!IsAuthenticated(session)) return false;

    PlayerRef player = session->GetPlayer();
    if (!player) return false;

    const string& msg = pkt.chat();
    if (msg.empty() || msg.size() > 200)
        return true;  

    Protocol::SChat res;
    res.set_object_id(player->GetObjectId());
    res.set_name(player->GetNameUtf8());
    res.set_chat(msg);
    res.set_chat_type(pkt.chat_type());

    SendBufferRef sendBuffer = MakeSendBuffer<Protocol::MsgId::S_CHAT>(res);

    switch (pkt.chat_type())
    {
    case Protocol::CHAT_WORLD:
        GWorld->DoAsync([sendBuffer]()
        {
            GWorld->BroadcastToAll(sendBuffer);
        });
        break;

    case Protocol::CHAT_NEAR:
    default:
    {
        GameSceneRef scene = player->GetGameScene();
        ZoneRef      zone = player->GetZone();
        if (!scene || !zone) return true;

        scene->DoAsync(MakeJob([scene, zone, sendBuffer]()
            {
                scene->BroadcastToAdjacentZones(zone, sendBuffer);
            }));
        break;
    }
    }

    return true;
}

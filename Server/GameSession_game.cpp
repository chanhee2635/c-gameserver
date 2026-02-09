#include "pch.h"
#include "GameSession.h"
#include "ClientPacketHandler.h"
#include "Player.h"
#include "RedisManager.h"
#include "DBManager.h"
#include "World.h"

void GameSession::Handle_C_AuthToken(string token)
{
	GameSessionRef self = static_pointer_cast<GameSession>(shared_from_this());

	GDBManager->DoAsyncPush([self, token]() {
		auto val = GRedisManager->Get<uint64>(token);
		if (!val.has_value())
		{
			cout << "로그인 정보 없음!" << endl;
			return;
		}

		uint64 accountDbId = val.value();
		Vector<SummaryDataRef> summaries;
		if (GDBManager->GetPlayerInfo(accountDbId, OUT summaries))
		{
			GWorld->DoAsyncPush([self, accountDbId, summaries = std::move(summaries)]() mutable {
				self->accountDbId = accountDbId;
				self->SendObjectSummaries(std::move(summaries));
			});
		}
	});
}

void GameSession::Handle_C_CreatePlayer(int32 templateId, string name)
{
	// 아이디 유효성 확인
	GameSessionRef self = static_pointer_cast<GameSession>(shared_from_this());
	GDBManager->DoAsyncPush([self, templateId, name]() {
		SummaryDataRef summary = MakeShared<SummaryData>();
		if (GDBManager->CheckDuplicatedPlayerName(self->accountDbId, templateId, name, OUT summary))
		{
			GWorld->DoAsyncPush([self, summary = std::move(summary)]() mutable {
				self->SendAddPlayerSummary(std::move(summary));
			});
		}
	});
}

void GameSession::Handle_C_EnterGame(wstring playerName)
{
	auto it = _summaries.find(playerName);
	if (it == _summaries.end())
	{
		// TODO: 로그 + 킥!
		return;
	}

	GameSessionRef self = static_pointer_cast<GameSession>(shared_from_this());
	SummaryDataRef summary = it->second;
	uint64 playerDbId = summary->objectId;
	GDBManager->DoAsyncPush([self, summary, playerDbId]() {
		StatData stat;
		if (GDBManager->GetPlayerDetailInfo(playerDbId, stat))
		{
			GWorld->DoAsyncPush([self, summary, stat = std::move(stat)]() mutable {
				GWorld->PlayerEnterToGame(self, summary, std::move(stat));
			});
		}
	});
}

void GameSession::Handle_C_LoadCompleted()
{
	PlayerRef player = GetPlayer();
	if (player == nullptr) return;

	GWorld->EnterCreature(player);
}

//void GameSession::Handle_C_Move(Protocol::PosInfo pos)
//{
//	PlayerRef player = GetPlayer();
//	if (player == nullptr) return;
//
//	GameSceneRef scene = player->GetGameScene();
//	if (scene == nullptr) return;
//
//	scene->MovePlayer(player, pos);
//}


void GameSession::SendObjectSummaries(Vector<SummaryDataRef> summaries)
{
	Protocol::S_PlayerList packet;
	for (SummaryDataRef summary : summaries)
	{
		summary->ToPacket(packet.add_summaries());
		_summaries.insert({ summary->name, summary });
	}

	Send(ClientPacketHandler::MakeSendBuffer(packet));
}

void GameSession::SendAddPlayerSummary(SummaryDataRef summary)
{
	Protocol::S_CreatePlayer packet;
	if (summary->objectId == 0) // 실패
	{
		packet.set_success(false);
	}
	else
	{
		packet.set_success(true);
		summary->ToPacket(packet.mutable_summary());
		_summaries.insert({ summary->name, std::move(summary) });
	}
	Send(ClientPacketHandler::MakeSendBuffer(packet));
}


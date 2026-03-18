#include "pch.h"
#include "GameSession.h"
#include "ClientPacketHandler.h"
#include "Player.h"
#include "RedisManager.h"
#include "DBManager.h"
#include "World.h"
#include "GameScene.h"

void GameSession::Handle_C_AuthToken(string token)
{
	GameSessionRef self = static_pointer_cast<GameSession>(shared_from_this());

	GDBManager->DoAsyncPush([self, token]() {
		auto val = GRedisManager->Get<uint64>(token);
		if (!val.has_value()) return;

		uint64 accountDbId = val.value();
		Vector<PlayerSummaryData> summaries;
		if (GDBManager->GetPlayerInfo(accountDbId, OUT summaries) == false) return;
		
		GWorld->DoAsyncPush([self, accountDbId, summaries = std::move(summaries)]() mutable {
			if (self->IsConnected() == false) return;
			self->SetAccountDbId(accountDbId);
			self->SendSummaries(std::move(summaries));
		});
	});
}

void GameSession::SendSummaries(Vector<PlayerSummaryData> summaries)
{
	Protocol::S_PlayerList packet;
	for (PlayerSummaryData& summary : summaries)
	{
		auto* info = packet.add_players();
		info->set_db_id(summary.dbId);
		info->set_name(Utils::ws2s(summary.name));
		info->set_level(summary.level);
		info->set_template_id(summary.templateId);
		AddSummary(std::move(summary));
	}
	Send(ClientPacketHandler::MakeSendBuffer(packet));
}

void GameSession::Handle_C_CreatePlayer(int32 templateId, string name)
{
	GameSessionRef self = static_pointer_cast<GameSession>(shared_from_this());
	GDBManager->DoAsyncPush([self, templateId, name]() {
		PlayerSummaryData summary;
		bool success = GDBManager->CreatePlayer(self->GetAccountDbId(), templateId, name, OUT summary);

		GWorld->DoAsyncPush([self, success, summary = std::move(summary)]() mutable {
			if (self->IsConnected() == false) return;

			Protocol::S_CreatePlayer packet;
			packet.set_success(success);
			if (success)
			{
				auto* info = packet.mutable_player();
				info->set_db_id(summary.dbId);
				info->set_name(Utils::ws2s(summary.name));
				info->set_level(summary.level);
				info->set_template_id(summary.templateId);
				self->AddSummary(std::move(summary));
			}
			else
				packet.set_reason(Utils::ws2s(L"이미 사용 중인 닉네임입니다."));

			self->Send(ClientPacketHandler::MakeSendBuffer(packet));
		});
	});
}

void GameSession::Handle_C_EnterGame(wstring playerName)
{
	auto it = _summaries.find(playerName);
	if (it == _summaries.end()) return;

	GameSessionRef self = static_pointer_cast<GameSession>(shared_from_this());
	const PlayerSummaryData summary = it->second;
	uint64 playerDbId = summary.dbId;

	GDBManager->DoAsyncPush([self, summary, playerDbId]() {
		PlayerLoadData loadData;
		if (GDBManager->GetPlayerDetailInfo(playerDbId, OUT loadData) == false)
		{
			GWorld->DoAsyncPush([self]() {
				if (self->IsConnected() == false) return;
				Protocol::S_EnterGame packet;
				packet.set_success(false);
				self->Send(ClientPacketHandler::MakeSendBuffer(packet));
			});
			return;
		}

		GWorld->DoAsyncPush([self, summary = std::move(summary), loadData = std::move(loadData)]() mutable {
			if (self->IsConnected() == false) return;
			GWorld->PlayerEnterToGame(self, std::move(summary), std::move(loadData));
		});
	});
}

void GameSession::Handle_C_LoadCompleted()
{
	PlayerRef player = GetPlayer();
	if (player == nullptr) return;

	GWorld->EnterCreature(player);
}

void GameSession::Handle_C_Move(Protocol::C_Move& packet)
{
	PlayerRef player = GetPlayer();
	if (!player) return;

	GameSceneRef scene = player->GetGameScene();
	if (!scene) return;

	MoveJobRef job = MakeShared<MoveJob>();
	job->objectId = player->GetObjectId();
	job->pos = GameUtil::ToServer(packet.pos_info().pos());
	job->yaw = packet.pos_info().yaw();
	job->state = (CreatureState)packet.pos_info().state();

	scene->PushMoveJob(job);
}
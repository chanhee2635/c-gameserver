#include "pch.h"
#include "DBManager.h"
#include "DBConnectionPool.h"
#include "DBBind.h"
#include "DataManager.h"

void DBManager::PushGlobalQueue()
{
	GDBQueue->Push(shared_from_this());
}

bool DBManager::GetPlayerInfo(uint64 accountDbId, OUT Vector<PlayerSummaryData>& summaries)
{
	DBConnectionRef dbConn = GDBConnectionPool->Pop();
	DBBind<1, 4> dbBind(*dbConn, L"SELECT player_id, name, level, class_type FROM Players WHERE account_id = (?)");

	int64 playerDbId = 0;
	WCHAR outName[100];
	int32 level = 0;
	int32 classtype = 0;

	dbBind.BindParam(0, accountDbId);
	dbBind.BindCol(0, OUT playerDbId);
	dbBind.BindCol(1, OUT outName);
	dbBind.BindCol(2, OUT level);
	dbBind.BindCol(3, OUT classtype);

	if (!dbBind.Execute())
		return false;

	while (dbConn->Fetch())
	{
		PlayerSummaryData summary;
		summary.dbId = playerDbId;  
		summary.name = outName;
		summary.level = level;
		summary.templateId = classtype;
		summaries.push_back(summary);
	}

	return true;
}

bool DBManager::CreatePlayer(uint64 accountDbId, int32 templateId, string name, OUT PlayerSummaryData& summary)
{
	auto spawnData = GDataManager->GetSpawnData(SpawnPoint::BEGIN);
	if (spawnData == nullptr) 
		return false;

	auto statData = GDataManager->GetPlayerData(templateId, 1);
	if (statData == nullptr) 
		return false;

	int32 maxHp = statData->maxHp;
	int32 maxMp = statData->maxMp;
	float posX = spawnData->pos.x;
	float posY = spawnData->pos.y;
	float posZ = spawnData->pos.z;
	float yaw = spawnData->yaw;
	wstring wName = Utils::s2ws(name);
	uint64 playerDbId = 0;

	DBConnectionRef dbConn = GDBConnectionPool->Pop();
	DBBind<9, 1> dbBind(*dbConn, L"CALL sp_create_player(?, ?, ?, ?, ?, ?, ?, ?, ?);");

	dbBind.BindParam(0, accountDbId);
	dbBind.BindParam(1, wName.c_str());
	dbBind.BindParam(2, templateId);
	dbBind.BindParam(3, maxHp);
	dbBind.BindParam(4, maxMp);
	dbBind.BindParam(5, posX);
	dbBind.BindParam(6, posY);
	dbBind.BindParam(7, posZ);
	dbBind.BindParam(8, yaw);
	dbBind.BindCol(0, OUT playerDbId);

	bool success = dbBind.Execute() && dbConn->Fetch() && playerDbId != 0;

	if (success)
	{
		summary.dbId = playerDbId;
		summary.name = wName;
		summary.level = 1;
		summary.templateId = templateId;
	}

	return success;
}

bool DBManager::GetPlayerDetailInfo(uint64 userDbid, OUT PlayerLoadData& loadData)
{
	// userDbId 를 가지고 플레이어 목록을 가져와야 한다.
	DBConnectionRef dbConn = GDBConnectionPool->Pop();
	DBBind<1, 7> dbBind(*dbConn, L"SELECT pos_x, pos_y, pos_z, rot_y, hp, mp, exp FROM PlayerDetails WHERE player_id = (?)");
	dbBind.BindParam(0, userDbid);
	dbBind.BindCol(0, OUT loadData.pos.x);
	dbBind.BindCol(1, OUT loadData.pos.y);
	dbBind.BindCol(2, OUT loadData.pos.z);
	dbBind.BindCol(3, OUT loadData.yaw);
	dbBind.BindCol(4, OUT loadData.hp);
	dbBind.BindCol(5, OUT loadData.mp);
	dbBind.BindCol(6, OUT loadData.exp);

	bool success = dbBind.Execute() && dbConn->Fetch();

	return success;
}

bool DBManager::SavePlayerLevelUp(uint64 playerDbId, int32 level, int32 hp, int32 mp, int64 exp, Vector3 pos, float yaw)
{
	DBConnectionRef dbConn = GDBConnectionPool->Pop();
	DBBind<9, 0> dbBind(*dbConn, L"CALL sp_update_player(?,?,?,?,?,?,?,?,?)");
	dbBind.BindParam(0, playerDbId);
	dbBind.BindParam(1, level);
	dbBind.BindParam(2, hp);
	dbBind.BindParam(3, mp);
	dbBind.BindParam(4, exp);
	dbBind.BindParam(5, pos.x);
	dbBind.BindParam(6, pos.y);
	dbBind.BindParam(7, pos.z);
	dbBind.BindParam(8, yaw);
	return dbBind.Execute();
}

bool DBManager::SavePlayerInfo(uint64 playerDbId, int32 hp, int32 mp, int64 exp, Vector3 pos, float yaw)
{
	DBConnectionRef dbConn = GDBConnectionPool->Pop();
	DBBind<8, 0> dbBind(*dbConn, L"UPDATE PlayerDetails SET hp=(?), mp=(?), exp=(?), pos_x=(?), pos_y=(?), pos_z=(?), rot_y=(?) WHERE player_id=(?)");

	dbBind.BindParam(0, hp);
	dbBind.BindParam(1, mp);
	dbBind.BindParam(2, exp);
	dbBind.BindParam(3, pos.x);
	dbBind.BindParam(4, pos.y);
	dbBind.BindParam(5, pos.z);
	dbBind.BindParam(6, yaw);
	dbBind.BindParam(7, playerDbId);

	return dbBind.Execute();
}
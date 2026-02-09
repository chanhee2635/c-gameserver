#include "pch.h"
#include "DBManager.h"
#include "DBConnectionPool.h"
#include "DBBind.h"
#include "DataManager.h"

shared_ptr<DBManager> GDBManager = make_shared<DBManager>();

void DBManager::PushGlobalQueue()
{
	GDBQueue->Push(shared_from_this());
}

bool DBManager::GetPlayerInfo(uint64 accountDbId, OUT Vector<SummaryDataRef>& summaries)
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
		SummaryDataRef summary = MakeShared<SummaryData>();
		summary->objectId = playerDbId;  // 게임에 입장할 땐 generator Id 로 변경
		summary->name = outName;
		summary->level = level;
		summary->templateId = classtype;
		summary->objectType = GameObjectType::Player;
		summaries.push_back(summary);
	}

	return true;
}

bool DBManager::CheckDuplicatedPlayerName(uint64 accountDbId, int32 templateId, string name, OUT SummaryDataRef& summary)
{
	auto spawnData = GDataManager->GetSpawnData(SpawnPoint::BEGIN);
	auto statData = GDataManager->GetPlayerData(templateId);

	DBConnectionRef dbConn = GDBConnectionPool->Pop();
	DBBind<9, 1> dbBind(*dbConn, L"CALL sp_create_player(?, ?, ?, ?, ?, ?, ?, ?, ?);");

	wstring wName = Utils::s2ws(name);
	uint64 playerDbId = 0;
	int32 hp = statData->maxHp;
	int32 mp = statData->maxMp;
	float posX = spawnData->pos.x;
	float posY = spawnData->pos.y;
	float posZ = spawnData->pos.z;
	float yaw = spawnData->yaw;

	dbBind.BindParam(0, accountDbId);
	dbBind.BindParam(1, wName.c_str());
	dbBind.BindParam(2, templateId);
	dbBind.BindParam(3, hp);
	dbBind.BindParam(4, mp);
	dbBind.BindParam(5, posX);
	dbBind.BindParam(6, posY);
	dbBind.BindParam(7, posZ);
	dbBind.BindParam(8, yaw);
	dbBind.BindCol(0, OUT playerDbId);

	if (dbBind.Execute() && dbConn->Fetch())
	{
		summary->objectId = playerDbId;
		summary->name = wName;
		summary->level = 1;
		summary->templateId = templateId;
		summary->objectType = GameObjectType::Player;
		return true;
	}

	return false;
}

bool DBManager::GetPlayerDetailInfo(uint64 userDbid, StatData& stat)
{
	// userDbId 를 가지고 플레이어 목록을 가져와야 한다.
	DBConnectionRef dbConn = GDBConnectionPool->Pop();
	DBBind<1, 7> dbBind(*dbConn, L"SELECT pos_x, pos_y, pos_z, rot_y, hp, mp, exp FROM PlayerDetails WHERE player_id = (?)");
	dbBind.BindParam(0, userDbid);
	dbBind.BindCol(0, OUT stat.pos.x);
	dbBind.BindCol(1, OUT stat.pos.y);
	dbBind.BindCol(2, OUT stat.pos.z);
	dbBind.BindCol(3, OUT stat.yaw);
	dbBind.BindCol(4, OUT stat.hp);
	dbBind.BindCol(5, OUT stat.mp);
	dbBind.BindCol(6, OUT stat.exp);

	if (dbBind.Execute() && dbConn->Fetch())
		return true;

	return false;
}

bool DBManager::SavePlayerInfo(uint64 userDbid, StatData stat)
{
	DBConnectionRef dbConn = GDBConnectionPool->Pop();
	DBBind<8, 0> dbBind(*dbConn, L"UPDATE PlayerDetails SET pos_x=(?), pos_y=(?), pos_z=(?), rot_y=(?), hp=(?), mp=(?), exp=(?) WHERE player_id = (?)");
	dbBind.BindParam(0, stat.pos.x);
	dbBind.BindParam(1, stat.pos.y);
	dbBind.BindParam(2, stat.pos.z);
	dbBind.BindParam(3, stat.yaw);
	dbBind.BindParam(4, stat.hp);
	dbBind.BindParam(5, stat.mp);
	dbBind.BindParam(6, stat.exp);
	dbBind.BindParam(7, userDbid);

	bool success = dbBind.Execute();
	return success;
}

bool DBManager::SavePlayerLevelUp(uint64 userDbid, int32 level, StatData stat)
{
	DBConnectionRef dbConn = GDBConnectionPool->Pop();
	DBBind<9, 0> dbBind(*dbConn, L"CALL sp_update_player(?,?,?,?,?,?,?,?,?)");
	dbBind.BindParam(0, userDbid);
	dbBind.BindParam(1, level);
	dbBind.BindParam(2, stat.hp);
	dbBind.BindParam(3, stat.mp);
	dbBind.BindParam(4, stat.exp);
	dbBind.BindParam(5, stat.pos.x);
	dbBind.BindParam(6, stat.pos.y);
	dbBind.BindParam(7, stat.pos.z);
	dbBind.BindParam(8, stat.yaw);
	return dbBind.Execute();
}

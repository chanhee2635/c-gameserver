#pragma once
#include "GlobalQueue.h"

class DBManager : public JobQueue
{
public:
	bool GetPlayerInfo(uint64 accountDbId, OUT Vector<PlayerSummaryData>& summaries);
	bool CreatePlayer(uint64 accountDbId, int32 templateId, string name, OUT PlayerSummaryData& summary);
	bool GetPlayerDetailInfo(uint64 userDbid, OUT PlayerLoadData& loadData);
	bool SavePlayerLevelUp(uint64 playerDbId, int32 level, int32 hp, int32 mp, int64 exp, Vector3 pos, float yaw);
	bool SavePlayerInfo(uint64 playerDbId, int32 hp, int32 mp, int64 exp, Vector3 pos, float yaw);

protected:
	virtual void PushGlobalQueue() override;
};


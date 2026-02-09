#pragma once
#include "GlobalQueue.h"

class DBManager : public JobQueue
{
public:
	bool GetPlayerInfo(uint64 accountDbId, OUT Vector<SummaryDataRef>& summaries);
	bool CheckDuplicatedPlayerName(uint64 accountDbId, int32 templateId, string name, OUT SummaryDataRef& summary);
	bool GetPlayerDetailInfo(uint64 userDbid, OUT StatData& stat);
	bool SavePlayerInfo(uint64 userDbid, StatData stat);
	bool SavePlayerLevelUp(uint64 userDbid, int32 level, StatData stat);

protected:
	virtual void PushGlobalQueue() override;
};

extern std::shared_ptr<DBManager>	GDBManager;
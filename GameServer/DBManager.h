#pragma once
#include "ConfigLoader.h"
#include "Struct.h"

struct DbConnGuard
{
    DbConnection* conn = nullptr;

    DbConnGuard() { conn = GDbConnectionPool->Pop(); }
    ~DbConnGuard() { if (conn) { conn->Unbind(); GDbConnectionPool->Push(conn); } }

    bool Valid() const { return conn != nullptr; }
};

class DBManager 
{
public:
    bool Init(const DBConfig& config);
    void DoAsync(Job job);

    bool GetPlayerInfo(uint64 accountDbId, OUT Vector<PlayerSummaryData>& summaries);

    bool CreatePlayer(uint64 accountDbId, int32 templateId,
        const string& name, OUT PlayerSummaryData& summary);

    bool GetPlayerDetail(uint64 accountDbId, const wstring& playerName,
        OUT PlayerSummaryData& summary, OUT PlayerLoadData& loadData);

    bool SavePlayerInfo(uint64 playerDbId, int32 hp, int32 mp,
        int64 exp, const Vector3& pos, float yaw);

    bool SavePlayerLevelUp(uint64 playerDbId, int32 level,
        int32 hp, int32 mp, int64 exp,
        const Vector3& pos, float yaw);

private:
    Vector<JobQueueRef> _workers;
    Atomic<uint32>      _nextWorker = 0;
};
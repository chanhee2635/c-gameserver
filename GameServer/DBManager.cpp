#include "pch.h"
#include "DBManager.h"

static bool TimedQuery(std::function<bool()> fn)
{
    auto   t0 = std::chrono::high_resolution_clock::now();
    bool   ok = fn();
    uint64 us = static_cast<uint64>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - t0).count());

    if (GServerStats)
    {
        GServerStats->db.queryCount.fetch_add(1, std::memory_order_relaxed);
        GServerStats->db.queryTotalUs.fetch_add(us, std::memory_order_relaxed);
        if (!ok) GServerStats->db.queryFailed.fetch_add(1, std::memory_order_relaxed);
    }
    return ok;
}

bool DBManager::Init(const DBConfig& config)
{
    wstring connStr =
        L"Driver={MySQL ODBC 9.5 Unicode Driver};"
        L"Server=" + config.host +
        L";Port=" + std::to_wstring(config.port) +
        L";Database=" + config.name +
        L";Uid=" + config.user +
        L";Pwd=" + config.password +
        L";charset=utf8mb4;";

    GDbConnectionPool->Init(connStr.c_str(), config.threads);

    _workers.resize(config.threads);
    for (auto& w : _workers)
        w = MakeShared<JobQueue>();

    return true;
}

void DBManager::DoAsync(Job job)
{
    uint32 idx = _nextWorker.fetch_add(1, std::memory_order_relaxed)
        % static_cast<uint32>(_workers.size());
    _workers[idx]->DoAsync(std::move(job));
}

bool DBManager::GetPlayerInfo(uint64 accountDbId, OUT Vector<PlayerSummaryData>& summaries)
{
    DbConnGuard g;
    if (!g.Valid()) return false;

    uint64     pAccId = accountDbId;
    uint64     outId = 0;
    WCHAR      outName[100] = {};
    int32      outLvl = 0;
    int32      outTmpl = 0;

    DbBind<1, 4> bind(*g.conn,
        L"SELECT player_id, name, level, template_id "
        L"FROM Players WHERE account_id = ?");

    bind.BindParam(pAccId);
    bind.BindCol(outId);
    bind.BindCol(outName, 100);
    bind.BindCol(outLvl);
    bind.BindCol(outTmpl);

    if (!TimedQuery([&] { return bind.Execute(); })) return false;

    while (g.conn->Fetch())
    {
        PlayerSummaryData summary;
        summary.dbId       = outId;
        summary.name       = wstring(outName);
        summary.level      = outLvl;
        summary.templateId = outTmpl;
        summaries.push_back(summary);
        ::memset(outName, 0, sizeof(outName));
    }
    return true;
}

bool DBManager::CreatePlayer(uint64 accountDbId, int32 templateId,
    const string& name, OUT PlayerSummaryData& summary)
{
    wstring wName = Utils::ToWString(name);

    // 1) INSERT Players
    {
        DbConnGuard g;
        if (!g.Valid()) return false;

        uint64 pAccId = accountDbId;
        WCHAR  pName[100] = {};
        int32  pTmpl = templateId;
        ::wcsncpy_s(pName, wName.c_str(), 99);

        DbBind<3, 0> bind(*g.conn,
            L"INSERT INTO Players (account_id, name, level, template_id) "
            L"VALUES (?, ?, 1, ?)");

        bind.BindParam(pAccId);
        bind.BindParam(pName);
        bind.BindParam(pTmpl);

        if (!TimedQuery([&] { return bind.Execute(); })) return false;
    }

    uint64 playerId = 0;
    {
        DbConnGuard g;
        if (!g.Valid()) return false;

        uint64 outId = 0;

        DbBind<0, 1> bind(*g.conn, L"SELECT LAST_INSERT_ID()");
        bind.BindCol(outId);

        if (!TimedQuery([&] { return bind.Execute(); })) return false;
        if (!g.conn->Fetch() || outId == 0) return false;
        playerId = outId;
    }

    // 3) INSERT PlayerDetails
    {
        DbConnGuard g;
        if (!g.Valid()) return false;

        uint64 pId = playerId;

        DbBind<1, 0> bind(*g.conn,
            L"INSERT INTO PlayerDetails (player_id) VALUES (?)");
        bind.BindParam(pId);

        if (!TimedQuery([&] { return bind.Execute(); })) return false;
    }

    summary.dbId = playerId;
    summary.name = wName;
    summary.level = 1;
    summary.templateId = templateId;
    return true;
}

bool DBManager::GetPlayerDetail(uint64 accountDbId, const wstring& playerName,
    OUT PlayerSummaryData& summary, OUT PlayerLoadData& loadData)
{
    DbConnGuard g;
    if (!g.Valid()) return false;

    uint64 pAccId = accountDbId;
    WCHAR  pName[100] = {};
    ::wcsncpy_s(pName, playerName.c_str(), 99);

    uint64 outId = 0;
    int32  outLvl = 0;
    int32  outTemplateId = 0;
    float  outPx = 0, outPy = 0, outPz = 0, outYaw = 0;
    int32  outHp = 0, outMp = 0;
    int64  outExp = 0;

    DbBind<2, 10> bind(*g.conn,
        L"SELECT p.player_id, p.level, p.template_id, "
        L"       pd.pos_x, pd.pos_y, pd.pos_z, pd.rot_y, "
        L"       pd.hp, pd.mp, pd.exp "
        L"FROM Players p "
        L"JOIN PlayerDetails pd ON p.player_id = pd.player_id "
        L"WHERE p.account_id = ? AND p.name = ?");

    bind.BindParam(pAccId);
    bind.BindParam(pName);
    bind.BindCol(outId);
    bind.BindCol(outLvl);
    bind.BindCol(outTemplateId);
    bind.BindCol(outPx);
    bind.BindCol(outPy);
    bind.BindCol(outPz);
    bind.BindCol(outYaw);
    bind.BindCol(outHp);
    bind.BindCol(outMp);
    bind.BindCol(outExp);

    if (!TimedQuery([&] { return bind.Execute(); })) return false;
    if (!g.conn->Fetch()) return false;

    summary.dbId       = outId;
    summary.name       = playerName;
    summary.level      = outLvl;
    summary.templateId = outTemplateId;

    loadData.pos = { outPx, outPy, outPz };
    loadData.yaw = outYaw;
    loadData.hp = outHp;
    loadData.mp = outMp;
    loadData.exp = outExp;

    return true;
}

bool DBManager::SavePlayerInfo(uint64 playerDbId, int32 hp, int32 mp,
    int64 exp, const Vector3& pos, float yaw)
{
    DbConnGuard g;
    if (!g.Valid()) return false;

    int32  pHp = hp;
    int32  pMp = mp;
    int64  pExp = exp;
    float  pPx = pos.x, pPy = pos.y, pPz = pos.z;
    float  pYaw = yaw;
    uint64 pId = playerDbId;

    DbBind<8, 0> bind(*g.conn,
        L"UPDATE PlayerDetails "
        L"SET hp=?, mp=?, exp=?, pos_x=?, pos_y=?, pos_z=?, rot_y=? "
        L"WHERE player_id=?");

    bind.BindParam(pHp);
    bind.BindParam(pMp);
    bind.BindParam(pExp);
    bind.BindParam(pPx);
    bind.BindParam(pPy);
    bind.BindParam(pPz);
    bind.BindParam(pYaw);
    bind.BindParam(pId);

    return TimedQuery([&] { return bind.Execute(); });
}

bool DBManager::SavePlayerLevelUp(uint64 playerDbId, int32 level,
    int32 hp, int32 mp, int64 exp,
    const Vector3& pos, float yaw)
{
    // 1) UPDATE Players.level
    {
        DbConnGuard g;
        if (!g.Valid()) return false;

        int32  pLvl = level;
        uint64 pId = playerDbId;

        DbBind<2, 0> bind(*g.conn,
            L"UPDATE Players SET level=? WHERE player_id=?");
        bind.BindParam(pLvl);
        bind.BindParam(pId);

        if (!TimedQuery([&] { return bind.Execute(); })) return false;
    }

    // 2) UPDATE PlayerDetails
    return SavePlayerInfo(playerDbId, hp, mp, exp, pos, yaw);
}
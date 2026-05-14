#pragma once
#include "GameTypes.h"
#include "ConfigLoader.h"
#include "Struct.h"

struct MoveResult
{
    Vector<ZoneRef> leaveZones;  
    Vector<ZoneRef> enterZones;  
    Vector<ZoneRef> keepZones;   
};

class World : public JobQueue
{
public:
    void Init(const WorldConfig& config);
    void Start();

    void PlayerEnterToGame(GameSessionRef session, PlayerSummaryData summary, PlayerLoadData loadData);
    void EnterCreature(CreatureRef creature);
    void LeaveCreature(CreatureRef creature);

    ZoneRef         GetZoneByPos(Vector3 pos)  const;
    ZoneRef         GetZoneById(int32 id)       const;
    Vector<ZoneRef> GetAdjacentZones(Vector3 pos) const;

    MoveResultRef GetMoveResult(ZoneRef oldZone, ZoneRef newZone) const;

    static void SendSpawnPacketsToPlayer(PlayerRef player, const Vector<ZoneRef>& zones);
    static void SendDespawnPacketsToPlayer(PlayerRef player, const Vector<ZoneRef>& zones);

private:
    void CreateScene();
    void CreateZone();
    void CreateMoveTable();
    void SpawnMonsters();

    MoveResultRef CalculateMoveResult(ZoneRef oldZone, ZoneRef newZone) const;

    WorldConfig  _config;

    Vector<GameSceneRef> _scenes;
    Vector<ZoneRef>      _zones;

    HashMap<GameScene*, Vector<ZoneRef>> _enterSceneGroups;
    HashMap<GameScene*, Vector<ZoneRef>> _leaveSceneGroups;
    HashMap<int32, HashMap<int32, MoveResultRef>> _moveTable;
};


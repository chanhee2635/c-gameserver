#pragma once


class GameScene : public JobQueue
{
friend class World;

public:
	void AddZone(ZoneRef zone);

	void Update();
	void HandlePlayerMove(PlayerRef player, Protocol::PosInfo posInfo);
	void HandleMove(CreatureRef creature, Vector3 oldPos, Vector3 newPos);
	void HandleMoveZone(CreatureRef creature, ZoneRef oldZone, ZoneRef newZone);
	void HandleVisionEnter(CreatureRef creature, Vector<ZoneRef> zones);
	void HandleSpawn(CreatureRef creature, Vector<ZoneRef> zones);
	void HandleVisionLeave(CreatureRef creature, Vector<ZoneRef> zones);
	void HandleDespawn(CreatureRef creature, Vector<ZoneRef> zones);
	void HandleRevive(PlayerRef player, bool currentPosRevive);
	void HandleReviveZone(PlayerRef player, ZoneRef oldZone, ZoneRef newZone);

	void PlayerAttack(PlayerRef player, Vector<ZoneRef> zones);
	void ApplyDamage(CreatureRef attacker, CreatureRef target);
	void HandleDead(CreatureRef attacker, CreatureRef target);

	void FindNearestPlayer(Vector<ZoneRef> zones, MonsterRef monster, Vector3 monsterPos);

	void BroadcastToZone(ZoneRef zone, SendBufferRef sendBuffer, uint64 exceptId = 0);
	void BroadcastToZones(Vector<ZoneRef> zones, SendBufferRef sendBuffer, uint64 exceptId = 0);
	void BroadcastAround(Vector3 pos, SendBufferRef sendBuffer, uint64 exceptId = 0);

private:
	Vector<ZoneRef> _zones;
	uint64 _lastTick = 0;
};


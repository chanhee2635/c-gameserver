#pragma once
class PacketHelper
{
public:
	static void SendSpawnPackets(PlayerRef player, const Vector<ZoneRef>& zones);
	static void SendDespawnPackets(PlayerRef player, const Vector<ZoneRef>& zones, uint64 objectId = 0);
};


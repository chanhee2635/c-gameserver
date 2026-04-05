#include "pch.h"
#include "PacketHelper.h"
#include "ClientPacketHandler.h"
#include "Zone.h"
#include "Player.h"

void PacketHelper::SendSpawnPackets(PlayerRef player, const Vector<ZoneRef>& zones)
{
    if (player == nullptr) return;

    Protocol::S_UpdateScene packet;

    auto flushPacket = [&packet, player]() {
        if (packet.spawns_size() > 0)
        {
            player->Send(ClientPacketHandler::MakeSendBuffer(packet));
            packet.Clear();
        }
    };

    for (const ZoneRef& zone : zones)
    {
        if (zone)
            zone->MakeSpawnPacket(player, packet, flushPacket);
    }

    flushPacket();
}

void PacketHelper::SendDespawnPackets(PlayerRef player, const Vector<ZoneRef>& zones, uint64 objectId)
{
    if (player == nullptr) return;

    Protocol::S_UpdateScene packet;

    auto flushPacket = [&packet, player]() {
        if (packet.despawns_size() > 0)
        {
            player->Send(ClientPacketHandler::MakeSendBuffer(packet));
            packet.Clear();
        }
    };

    for (const ZoneRef& zone : zones)
    {
        if (zone)
            zone->MakeDespawnPacket(player->GetObjectId(), packet, flushPacket);
    }

    flushPacket();
}

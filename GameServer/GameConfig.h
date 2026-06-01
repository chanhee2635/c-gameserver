#pragma once

namespace GameConfig
{
    namespace Zone
    {
        static constexpr uint32 MAX_DESPAWN_BYTES = 16;   
        static constexpr uint32 MAX_MOVE_BYTES = 40;   
        static constexpr uint32 MAX_SPAWN_BYTES = 256;  

        static constexpr uint32 PACKET_USABLE = Config::Buffer::SEND_BUFFER_CHUNK_SIZE - Config::Packet::HEADER_SIZE;

        static constexpr uint32 DESPAWN_FLUSH_COUNT = PACKET_USABLE / MAX_DESPAWN_BYTES;
        static constexpr uint32 MOVE_FLUSH_COUNT = PACKET_USABLE / MAX_MOVE_BYTES;
        static constexpr uint32 SPAWN_FLUSH_COUNT = PACKET_USABLE / MAX_SPAWN_BYTES;
    }

    namespace Move {
        constexpr float  POS_EPS = 0.3f;
        constexpr float  POS_EPS_SQ = POS_EPS * POS_EPS;
        constexpr float  VEL_EPS = 0.5f;
        constexpr float  VEL_EPS_SQ = VEL_EPS * VEL_EPS;
        // Refresh interval for a moving entity whose path matches server prediction
        // (no divergence). 400ms => only 2.5 updates/s to observers, so remote movement
        // hitches as dead-reckoning corrects each sparse update. 150ms (~6.6/s) keeps
        // remote motion smooth at typical densities.
        // Trade-off: this ~2.6x broadcast volume is fine for normal play but, in a single
        // scene packed with >~1000 movers, re-approaches the tick budget (load-tested).
        // Raising the per-scene ceiling further needs AOI/broadcast-volume reduction,
        // not a higher heartbeat.
        constexpr uint64 HEARTBEAT_MS = 150;

        // Anti-cheat: server-authoritative move-speed gate.
        constexpr float  SPRINT_MULT     = 2.0f;   // client sprint = base speed * 2
        constexpr float  SPEED_TOLERANCE = 1.5f;   // latency/jitter headroom
        constexpr uint64 MAX_MOVE_DT_MS  = 1000;   // cap allowance so pauses cannot bank distance
        constexpr float  MOVE_DIST_EPS   = 1.0f;   // ignore sub-unit jitter
    }
}
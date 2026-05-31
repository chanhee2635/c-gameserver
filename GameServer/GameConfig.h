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
        constexpr uint64 HEARTBEAT_MS = 400;

        // Anti-cheat: server-authoritative move-speed gate.
        constexpr float  SPRINT_MULT     = 2.0f;   // client sprint = base speed * 2
        constexpr float  SPEED_TOLERANCE = 1.5f;   // latency/jitter headroom
        constexpr uint64 MAX_MOVE_DT_MS  = 1000;   // cap allowance so pauses cannot bank distance
        constexpr float  MOVE_DIST_EPS   = 1.0f;   // ignore sub-unit jitter
    }
}
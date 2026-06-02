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
        constexpr float  POS_EPS = 0.5f;
        constexpr float  POS_EPS_SQ = POS_EPS * POS_EPS;
        constexpr float  VEL_EPS = 0.5f;
        constexpr float  VEL_EPS_SQ = VEL_EPS * VEL_EPS;
        // Heartbeat / confirm interval for a moving entity whose path still matches server
        // prediction (divergence under POS_EPS). Real motion changes are sent immediately by
        // the threshold checks in ShouldBroadcastMove; this is only the fallback "still here,
        // still on course" packet for perfectly straight movement. Server-side dead reckoning
        // plus the client's velocity-blended coasting keep straight motion smooth between
        // these sparse updates, so 500ms is safe and keeps broadcast volume low.
        constexpr uint64 HEARTBEAT_MS = 500;

        // Dead reckoning: cap how far the server projects a reported position forward by
        // the measured latency (network + queue), so a stalled packet cannot fling the avatar.
        constexpr uint64 DEAD_RECKON_MAX_MS = 150;

        // Reconciliation: min interval between move-correction packets to one player, so a
        // client shoved into a wall doesn't get corrected every tick.
        constexpr uint64 CORRECTION_MIN_INTERVAL_MS = 100;

        // Anti-cheat: server-authoritative move-speed gate.
        constexpr float  SPRINT_MULT     = 2.0f;   // client sprint = base speed * 2
        constexpr float  SPEED_TOLERANCE = 1.5f;   // latency/jitter headroom
        constexpr uint64 MAX_MOVE_DT_MS  = 1000;   // cap allowance so pauses cannot bank distance
        constexpr float  MOVE_DIST_EPS   = 1.0f;   // ignore sub-unit jitter
    }
}
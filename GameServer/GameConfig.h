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
}
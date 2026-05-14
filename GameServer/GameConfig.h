#pragma once

namespace GameConfig
{
    namespace Zone
    {
        static constexpr uint32 MAX_DESPAWN_BYTES = 16;   // uint64 varint(10) + tag(1) + 여유
        static constexpr uint32 MAX_MOVE_BYTES = 64;   // PosInfo: object_id + x,y,z,yaw + tags
        static constexpr uint32 MAX_SPAWN_BYTES = 256;  // ObjectInfo: 이름, 스탯, 위치 등

        static constexpr uint32 PACKET_USABLE = Config::Buffer::SEND_BUFFER_CHUNK_SIZE - Config::Packet::HEADER_SIZE;

        static constexpr uint32 DESPAWN_FLUSH_COUNT = PACKET_USABLE / MAX_DESPAWN_BYTES;
        static constexpr uint32 MOVE_FLUSH_COUNT = PACKET_USABLE / MAX_MOVE_BYTES;
        static constexpr uint32 SPAWN_FLUSH_COUNT = PACKET_USABLE / MAX_SPAWN_BYTES;
    }
}
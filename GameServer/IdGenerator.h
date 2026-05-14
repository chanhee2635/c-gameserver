#pragma once

namespace IdGenerator
{
    inline uint64 Generate()
    {
        static Atomic<uint64> s_counter{ 1 };
        return s_counter.fetch_add(1, std::memory_order_relaxed);
    }
}
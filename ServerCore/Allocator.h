#pragma once

// Global / oversized allocations
class BaseAllocator
{
public:
    static void* Allocate(uint32 size);
    static void  Release(void* ptr);

private:
    static constexpr uint32 AlignSize = Config::Memory::DEFAULT_ALIGNMENT;
};

// Temporary (TLS frame allocator)
class FrameAllocator
{
public:
    FrameAllocator(uint32 bufferSize = BufferSize);
    ~FrameAllocator();

    void* Allocate(uint32 size);
    void  Release(void* ptr);
    void  Clear();

private:
    static constexpr uint32 AlignSize  = Config::Memory::DEFAULT_ALIGNMENT;
    static constexpr uint32 BufferSize = Config::Memory::Frame::DEFAULT_BUFFER_SIZE;

    uint8*  _buffer     = nullptr;
    uint8*  _freePtr    = nullptr;
    uint8*  _endPtr     = nullptr;
    uint32  _bufferSize = 0;
    std::vector<void*> _overflowAllocs;
};

class PoolAllocator
{
public:
    static void* Allocate(uint32 size);
    static void  Release(void* ptr);
};

// Debug: Use-after-free / buffer overflow detector
class StompAllocator
{
public:
    static void* Allocate(uint32 size);
    static void  Release(void* ptr);

private:
    static constexpr uint32 AlignSize = Config::Memory::DEFAULT_ALIGNMENT;
    static const int32 GPageSize;
};

template<typename T, AllocType Type = AllocType::Pool>
class StlAllocator
{
public:
    using value_type = T;

    template<typename Other>
    struct rebind { using other = StlAllocator<Other, Type>; };

    StlAllocator() noexcept {}
    template<typename Other> StlAllocator(const StlAllocator<Other, Type>&) noexcept {}

    bool operator==(const StlAllocator&) const noexcept { return true; }

    T* allocate(size_t count)
    {
        const int32 size = static_cast<int32>(count * sizeof(T));

        if constexpr (Type == AllocType::Frame)
            return static_cast<T*>(LFrameAllocator->Allocate(size));
        else if constexpr (Type == AllocType::Stomp)
            return static_cast<T*>(StompAllocator::Allocate(size));
        else
            return static_cast<T*>(PoolAllocator::Allocate(size));
    }

    void deallocate(T* ptr, size_t count)
    {
        if constexpr (Type == AllocType::Frame)
            return;
        else if constexpr (Type == AllocType::Stomp)
            StompAllocator::Release(ptr);
        else
            PoolAllocator::Release(ptr);
    }
};

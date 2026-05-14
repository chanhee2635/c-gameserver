#pragma once

struct alignas(MEMORY_ALLOCATION_ALIGNMENT) MemoryHeader : public SLIST_ENTRY
{
    MemoryHeader(uint32 size) : _allocSize(size), _magic(MagicNumber) {}

    static void* AttachHeader(MemoryHeader* header, uint32 size)
    {
        new(header) MemoryHeader(size);
        return reinterpret_cast<void*>(header + 1);
    }

    static MemoryHeader* DetachHeader(void* ptr)
    {
        return static_cast<MemoryHeader*>(ptr) - 1;
    }

    uint32 GetAllocSize() const { return _allocSize; }
    uint32 GetMagic()     const { return _magic; }

private:
    uint32 _allocSize;
    uint32 _magic;

    static constexpr uint32 MagicNumber = Config::Memory::MAGIC_NUMBER;
};

class MemoryPool
{
public:
    MemoryPool(int32 allocSize);
    ~MemoryPool();

    void          Push(MemoryHeader* ptr);
    MemoryHeader* Pop();

    int32 GetAllocSize() const { return _allocSize; }

private:
    MemoryHeader* AllocBatch();

    SLIST_HEADER  _header;
    int32         _allocSize = 0;

#ifdef _DEBUG
    Atomic<int32> _useCount     = 0;
    Atomic<int32> _reserveCount = 0;
#endif

    static constexpr uint32 MagicNumber = Config::Memory::MAGIC_NUMBER;
    static constexpr uint32 AllocCount  = Config::Memory::ALLOC_COUNT;
};

class TlsMemoryPool
{
public:
    TlsMemoryPool(MemoryPool* globalPool);
    ~TlsMemoryPool();

    MemoryHeader* Pop();
    void          Push(MemoryHeader* header);
    void          ReturnAll();

private:
    void          PushLocal(MemoryHeader* header);
    MemoryHeader* PopLocal();

    void FetchFromGlobal();
    void ReturnToGlobal();

    MemoryPool*   _globalPool;
    MemoryHeader* _head  = nullptr;
    int32         _count = 0;

    static constexpr int32 MaxCount   = Config::Memory::TLS_MAX_COUNT;
    static constexpr int32 BatchCount = Config::Memory::TLS_BATCH_COUNT;
};

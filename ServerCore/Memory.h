#pragma once
#include "Allocator.h"
#include "MemoryPool.h"

class MemoryManager
{
public:
    MemoryManager();
    ~MemoryManager();

    void* Allocate(uint32 size);
    void  Release(void* ptr);

    MemoryPool* GetPool(uint32 allocSize) { return _poolTable[allocSize]; }

private:
    static constexpr uint32 MAX_POOL_SIZE = Config::Memory::MAX_POOL_SIZE;
    static constexpr uint32 POOL_COUNT    = Config::Memory::POOL_COUNT;

    int32       _poolCount = 0;
    MemoryPool* _pools[POOL_COUNT];
    MemoryPool* _poolTable[MAX_POOL_SIZE + 1];
};

class ThreadLocalMemory
{
public:
    ThreadLocalMemory();
    ~ThreadLocalMemory();

    MemoryHeader* Allocate(int32 allocSize);
    void          Release(MemoryHeader* header);

private:
    static constexpr uint32 MAX_POOL_SIZE = Config::Memory::MAX_POOL_SIZE;
    static constexpr uint32 POOL_COUNT    = Config::Memory::POOL_COUNT;

    TlsMemoryPool* _tlsPools[POOL_COUNT]            = {};
    TlsMemoryPool* _tlsPoolTable[MAX_POOL_SIZE + 1] = {};
};

template<typename Type, typename... Args>
Type* xnew(Args&&... args)
{
    void* memory = PoolAllocator::Allocate(sizeof(Type));
    return new(memory) Type(std::forward<Args>(args)...);
}

template<typename Type>
void xdelete(Type* obj)
{
    obj->~Type();
    PoolAllocator::Release(obj);
}

template<typename Type, typename... Args>
std::shared_ptr<Type> MakeShared(Args&&... args)
{
    return std::shared_ptr<Type>{ xnew<Type>(std::forward<Args>(args)...), xdelete<Type> };
}

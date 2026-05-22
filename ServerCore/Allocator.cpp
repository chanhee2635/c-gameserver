#include "pch.h"
#include "Allocator.h"
#include "Memory.h"

void* BaseAllocator::Allocate(uint32 size)
{
    return ::_aligned_malloc(size, AlignSize);
}

void BaseAllocator::Release(void* ptr)
{
    ::_aligned_free(ptr);
}

const int32 StompAllocator::GPageSize = []() {
    SYSTEM_INFO info;
    ::GetSystemInfo(&info);
    return static_cast<int32>(info.dwPageSize);
}();

void* StompAllocator::Allocate(uint32 size)
{
    const uint32 alignedSize = MemoryUtils::AlignUp(size, AlignSize);
    const uint32 dataPageCount = (alignedSize + GPageSize - 1) / GPageSize;
    const uint32 totalPageCount = dataPageCount + 1;

    void* baseAddress = ::VirtualAlloc(NULL, static_cast<size_t>(totalPageCount * GPageSize), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    void* guardPage = static_cast<int8*>(baseAddress) + (dataPageCount * GPageSize);
    DWORD oldProtect;
    ::VirtualProtect(guardPage, GPageSize, PAGE_NOACCESS, &oldProtect);

    const uint32 dataOffset = (dataPageCount * GPageSize) - alignedSize;
    return static_cast<void*>(static_cast<int8*>(baseAddress) + dataOffset);
}

void StompAllocator::Release(void* ptr)
{
    if (ptr == nullptr) return;

    const uintptr address = reinterpret_cast<uintptr>(ptr);
    const uintptr baseAddress = address - (address % GPageSize);
    ::VirtualFree(reinterpret_cast<void*>(baseAddress), 0, MEM_RELEASE);
}

FrameAllocator::FrameAllocator(uint32 bufferSize) : _bufferSize(bufferSize)
{
    _buffer = static_cast<uint8*>(::_aligned_malloc(bufferSize, AlignSize));
    _freePtr = _buffer;
    _endPtr = _buffer + bufferSize;
}

FrameAllocator::~FrameAllocator()
{
    if (_buffer)
    {
        ::_aligned_free(_buffer);
        _buffer = nullptr;
    }
}

void* FrameAllocator::Allocate(uint32 size)
{
    const uint32 alignedSize = MemoryUtils::AlignUp(size, AlignSize);

    if (_freePtr + alignedSize > _endPtr)
    {
        if (GServerStats)
            GServerStats->memory.frameOverflowCount.fetch_add(1, std::memory_order_relaxed);

        // LOG_WARN 금지: Logger가 FrameAllocator를 사용하므로 여기서 로그 호출 시 무한재귀 발생
        void* ptr = BaseAllocator::Allocate(alignedSize);
        _overflowAllocs.push_back(ptr);
        return ptr;
    }

    void* ptr = _freePtr;
    _freePtr += alignedSize;
    return ptr;
}

void FrameAllocator::Release(void* ptr)
{
    if (ptr >= _buffer && ptr < _endPtr)
        return;

    auto it = std::find(_overflowAllocs.begin(), _overflowAllocs.end(), ptr);
    if (it != _overflowAllocs.end())
    {
        BaseAllocator::Release(ptr);
        _overflowAllocs.erase(it);
    }
}

void FrameAllocator::Clear()
{
    for (void* ptr : _overflowAllocs)
        BaseAllocator::Release(ptr);
    _overflowAllocs.clear();

    _freePtr = _buffer;
}

void* PoolAllocator::Allocate(uint32 size)
{
    return GMemory->Allocate(size);
}

void PoolAllocator::Release(void* ptr)
{
    GMemory->Release(ptr);
}

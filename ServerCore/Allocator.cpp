#include "pch.h"
#include "Allocator.h"
#include "Memory.h"

/*------------------
	BaseAllocator
------------------*/

void* BaseAllocator::Alloc(int32 size)
{
	return ::_aligned_malloc(size, AlignSize);
}

void BaseAllocator::Release(void* ptr)
{
	::_aligned_free(ptr);
}

/*------------------
	StompAllocator
------------------*/

const int32 StompAllocator::PAGE_SIZE = []()
{
	SYSTEM_INFO info;
	::GetSystemInfo(&info);
	return static_cast<int32>(info.dwPageSize);
}();

void* StompAllocator::Alloc(int32 size)
{
	const uint32 alignedSize    = MemoryUtils::AlignUp(size, AlignSize);
	const uint32 dataPageCount  = (alignedSize + PAGE_SIZE - 1) / PAGE_SIZE;
	const uint32 totalPageCount = dataPageCount + 1;  // +1 Guard Page

	void* baseAddress = ::VirtualAlloc(NULL,
		static_cast<size_t>(totalPageCount * PAGE_SIZE),
		MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	// Guard Page: 오버플로우 즉시 Access Violation
	void* guardPage = static_cast<int8*>(baseAddress) + (dataPageCount * PAGE_SIZE);
	DWORD oldProtect;
	::VirtualProtect(guardPage, PAGE_SIZE, PAGE_NOACCESS, &oldProtect);

	const uint32 dataOffset = (dataPageCount * PAGE_SIZE) - alignedSize;
	return static_cast<void*>(static_cast<int8*>(baseAddress) + dataOffset);
}

void StompAllocator::Release(void* ptr)
{
	if (ptr == nullptr) return;

	const uintptr address     = reinterpret_cast<uintptr>(ptr);
	const uintptr baseAddress = address - (address % PAGE_SIZE);
	::VirtualFree(reinterpret_cast<void*>(baseAddress), 0, MEM_RELEASE);
}

/*-----------------
	FrameAllocator
-----------------*/

FrameAllocator::FrameAllocator(uint32 bufferSize) : _bufferSize(bufferSize)
{
	_buffer  = static_cast<uint8*>(::_aligned_malloc(bufferSize, AlignSize));
	_freePtr = _buffer;
	_endPtr  = _buffer + bufferSize;
}

FrameAllocator::~FrameAllocator()
{
	if (_buffer)
	{
		::_aligned_free(_buffer);
		_buffer = nullptr;
	}
}

void* FrameAllocator::Alloc(uint32 size)
{
	const uint32 alignedSize = MemoryUtils::AlignUp(size, AlignSize);

	ASSERT_CRASH(_freePtr + alignedSize <= _endPtr);

	void* ptr = _freePtr;
	_freePtr += alignedSize;
	return ptr;
}

void FrameAllocator::Clear()
{
	_freePtr = _buffer;
}

/*-----------------
	PoolAllocator
-----------------*/

void* PoolAllocator::Alloc(int32 size)
{
	return GMemory->Allocate(size);
}

void PoolAllocator::Release(void* ptr)
{
	GMemory->Release(ptr);
}

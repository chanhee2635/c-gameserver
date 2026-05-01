#include "pch.h"
#include "Memory.h"
#include "MemoryPool.h"

/*--------------
	MemoryManager
--------------*/

MemoryManager::MemoryManager()
{
	for (int32 i = 0; i <= static_cast<int32>(MAX_POOL_SIZE); ++i)
		_poolTable[i] = nullptr;

	int32 currentSize = 0;
	int32 tableIndex  = 1;

	auto CreatePools = [&](int32 endSize, int32 step)
	{
		for (int32 size = currentSize + step; size <= endSize; size += step)
		{
			ASSERT_CRASH(_poolCount < static_cast<int32>(POOL_COUNT));
			MemoryPool* pool = new MemoryPool(size);
			_pools[_poolCount++] = pool;

			while (tableIndex <= size && tableIndex <= static_cast<int32>(MAX_POOL_SIZE))
				_poolTable[tableIndex++] = pool;

			currentSize = size;
		}
	};

	CreatePools(1024, 32);
	CreatePools(2048, 128);
	CreatePools(4096, 256);
}

MemoryManager::~MemoryManager()
{
	for (int32 i = 0; i < _poolCount; i++)
		delete _pools[i];
}

void* MemoryManager::Allocate(uint32 size)
{
	MemoryHeader* header   = nullptr;
	const int32   allocSize = size + sizeof(MemoryHeader);

#ifdef _STOMP
	return StompAllocator::Alloc(size);
#else
	if (allocSize > static_cast<int32>(MAX_POOL_SIZE))
	{
		header = reinterpret_cast<MemoryHeader*>(::_aligned_malloc(allocSize, Config::Memory::SLIST_ALIGNMENT));
		new(header) MemoryHeader(allocSize);
	}
	else if (LThreadMemory != nullptr) [[likely]]
	{
		header = LThreadMemory->Allocate(allocSize);
	}
	else
	{
		header = _poolTable[allocSize]->Pop();
	}

	return MemoryHeader::AttachHeader(header, allocSize);
#endif
}

void MemoryManager::Release(void* ptr)
{
	if (ptr == nullptr) return;

#ifdef _STOMP
	StompAllocator::Release(ptr);
	return;
#endif

	MemoryHeader* header    = MemoryHeader::DetachHeader(ptr);
	const int32   allocSize = header->GetAllocSize();
	ASSERT_CRASH(allocSize > 0);

	if (allocSize > static_cast<int32>(MAX_POOL_SIZE))
		::_aligned_free(header);
	else if (LThreadMemory != nullptr) [[likely]]
		LThreadMemory->Release(header);
	else
		_poolTable[allocSize]->Push(header);
}

/*-----------------
	ThreadLocalMemory
-----------------*/

ThreadLocalMemory::ThreadLocalMemory()
{
	MemoryPool*    prevGlobal = nullptr;
	TlsMemoryPool* prevTls    = nullptr;
	int32          poolIndex  = 0;

	for (int32 size = 1; size <= static_cast<int32>(MAX_POOL_SIZE); ++size)
	{
		MemoryPool* gPool = GMemory->GetPool(size);

		if (gPool != prevGlobal)
		{
			prevTls              = new TlsMemoryPool(gPool);
			_tlsPools[poolIndex++] = prevTls;
			prevGlobal           = gPool;
		}

		_tlsPoolTable[size] = prevTls;
	}
}

ThreadLocalMemory::~ThreadLocalMemory()
{
	for (int32 i = 0; i < static_cast<int32>(POOL_COUNT); ++i)
	{
		if (_tlsPools[i])
		{
			delete _tlsPools[i];
			_tlsPools[i] = nullptr;
		}
	}
}

MemoryHeader* ThreadLocalMemory::Allocate(int32 allocSize)
{
	return _tlsPoolTable[allocSize]->Pop();
}

void ThreadLocalMemory::Release(MemoryHeader* header)
{
	_tlsPoolTable[header->GetAllocSize()]->Push(header);
}

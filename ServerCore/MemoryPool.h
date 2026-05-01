#pragma once

/*-----------------
	MemoryHeader
-----------------*/

struct alignas(Config::Memory::SLIST_ALIGNMENT) MemoryHeader : public SLIST_ENTRY
{
	// [MemoryHeader][Data]
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

/*---------------
	MemoryPool
---------------*/

DECLSPEC_ALIGN(Config::Memory::SLIST_ALIGNMENT)
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
	atomic<int32> _useCount     = 0;
	atomic<int32> _reserveCount = 0;
#endif

	static constexpr uint32 AlignSize   = Config::Memory::SLIST_ALIGNMENT;
	static constexpr uint32 MagicNumber = Config::Memory::MAGIC_NUMBER;
	static constexpr uint32 AllocCount  = Config::Memory::ALLOC_COUNT;
};

/*------------------
	TlsMemoryPool
------------------*/

// 스레드 전용 풀 - Global MemoryPool을 감싸 lock-free 로컬 캐시 제공
class TlsMemoryPool
{
public:
	TlsMemoryPool(MemoryPool* globalPool);
	~TlsMemoryPool();

	MemoryHeader* Pop();
	void          Push(MemoryHeader* header);
	void          ReturnAll();  // 스레드 종료 시 전부 반환

private:
	void          PushLocal(MemoryHeader* header);
	MemoryHeader* PopLocal();

	void FetchFromGlobal();   // 로컬 고갈 시 global에서 BatchCount개 가져옴
	void ReturnToGlobal();    // 로컬 포화 시 global로 BatchCount개 반환

	MemoryPool*   _globalPool;
	MemoryHeader* _head  = nullptr;  // SLIST_ENTRY::Next를 단일 스레드 free list로 재사용
	int32         _count = 0;

	static constexpr int32 MaxCount   = Config::Memory::TLS_MAX_COUNT;
	static constexpr int32 BatchCount = Config::Memory::TLS_BATCH_COUNT;
};

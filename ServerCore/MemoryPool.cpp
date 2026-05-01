#include "pch.h"
#include "MemoryPool.h"

/*-----------
	MemoryPool
-----------*/

MemoryPool::MemoryPool(int32 allocSize) : _allocSize(allocSize)
{
	::InitializeSListHead(&_header);
}

MemoryPool::~MemoryPool()
{
	while (MemoryHeader* header = static_cast<MemoryHeader*>(::InterlockedPopEntrySList(&_header)))
		::_aligned_free(header);
}

void MemoryPool::Push(MemoryHeader* ptr)
{
	ASSERT_CRASH(ptr->GetMagic() == MagicNumber);

	::InterlockedPushEntrySList(&_header, static_cast<PSLIST_ENTRY>(ptr));

#ifdef _DEBUG
	_useCount.fetch_sub(1);
	_reserveCount.fetch_add(1);
#endif
}

MemoryHeader* MemoryPool::Pop()
{
	MemoryHeader* header = static_cast<MemoryHeader*>(::InterlockedPopEntrySList(&_header));

	if (header == nullptr)
		header = AllocBatch();
#ifdef _DEBUG
	else
		_reserveCount.fetch_sub(1);
	_useCount.fetch_add(1);
#endif

	return header;
}

// 풀 고갈 시 AllocCount개를 묶음 할당 (OS 호출 빈도 감소)
MemoryHeader* MemoryPool::AllocBatch()
{
	// AllocCount-1개를 풀에 넣고, 1개는 반환
	for (int32 i = 0; i < AllocCount - 1; ++i)
	{
		MemoryHeader* header = static_cast<MemoryHeader*>(::_aligned_malloc(_allocSize, AlignSize));
		new(header) MemoryHeader(_allocSize);
		::InterlockedPushEntrySList(&_header, static_cast<PSLIST_ENTRY>(header));
	}
#ifdef _DEBUG
	_reserveCount.fetch_add(AllocCount - 1);
#endif

	MemoryHeader* header = static_cast<MemoryHeader*>(::_aligned_malloc(_allocSize, AlignSize));
	new(header) MemoryHeader(_allocSize);
	return header;
}

/*--------------
	TlsMemoryPool
--------------*/

TlsMemoryPool::TlsMemoryPool(MemoryPool* globalPool)
	: _globalPool(globalPool)
{
}

TlsMemoryPool::~TlsMemoryPool()
{
	ReturnAll();
}

MemoryHeader* TlsMemoryPool::Pop()
{
	if (_count == 0) [[unlikely]]
		FetchFromGlobal();

	return PopLocal();
}

void TlsMemoryPool::Push(MemoryHeader* header)
{
	PushLocal(header);

	if (_count >= MaxCount) [[unlikely]]
		ReturnToGlobal();
}

void TlsMemoryPool::FetchFromGlobal()
{
	for (int32 i = 0; i < BatchCount; ++i)
		PushLocal(_globalPool->Pop());
}

void TlsMemoryPool::ReturnToGlobal()
{
	while (_count > BatchCount)
		_globalPool->Push(PopLocal());
}

void TlsMemoryPool::ReturnAll()
{
	while (_head != nullptr)
		_globalPool->Push(PopLocal());
}

void TlsMemoryPool::PushLocal(MemoryHeader* header)
{
	header->Next = _head;
	_head = header;
	_count++;
}

MemoryHeader* TlsMemoryPool::PopLocal()
{
	MemoryHeader* header = _head;
	_head = static_cast<MemoryHeader*>(_head->Next);
	_count--;
	return header;
}

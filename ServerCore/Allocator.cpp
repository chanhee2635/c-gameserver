#include "pch.h"
#include "Allocator.h"
#include "Memory.h"

/*------------------
	BaseAllocator
------------------*/

void* BaseAllocator::Alloc(int32 size)
{
	return ::malloc(size);
}

void BaseAllocator::Release(void* ptr)
{
	::free(ptr);
}

/*------------------
	StompAllocator
------------------*/
// Use-After-Free 취약점을 방지

/*
* 필요한 페이지 크기만큼의 가상 메모리 공간을 할당 (오버플로우 감지)
*/
void* StompAllocator::Alloc(int32 size)
{
	const int64 pageCount = (size + PAGE_SIZE - 1) / PAGE_SIZE;  // 필요한 페이지 크기
	const int64 dataOffset = pageCount * PAGE_SIZE - size;  // 필요한 페이지 크기에서 size 를 뺀 위치 [           offset[size]] (데이터 사용 공간 남용을 막기 위함)
	void* baseAddress = ::VirtualAlloc(NULL, pageCount * PAGE_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE); // Thread-Safe(커널레벨/System Call 원자적 처리)
	// dataOffset(byte) 만큼을 이동하여 해당 위치를 사용하도록 함
	return static_cast<void*>(static_cast<int8*>(baseAddress) + dataOffset);
}

/*
* 가상 메모리 영역을 OS에 반환하고 접근 불가능한 상태로 만든다.(Use-After-Free 감지)
*/
void StompAllocator::Release(void* ptr)
{
	const int64 address = reinterpret_cast<int64>(ptr);
	// 시작 위치를 찾는다.
	const int64 baseAddress = address - (address % PAGE_SIZE);
	::VirtualFree(reinterpret_cast<void*>(baseAddress), 0, MEM_RELEASE);
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

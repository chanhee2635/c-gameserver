#pragma once

enum
{
	SLIST_ALIGNMENT = 16
};

/*-----------------
	MemoryHeader
-----------------*/

DECLSPEC_ALIGN(SLIST_ALIGNMENT) // 시작 주소가 16의 배수가 되도록 메모리 할당
struct MemoryHeader : public SLIST_ENTRY  // MemoryHeader의 크기가 8을 넘어가면 SLIST_ENTRY(8바이트)가 아닌 Mutex로 사용해야 함(원자성 보장X)
{
	// [MemoryHeader][Data]
	MemoryHeader(int32 size) : allocSize(size) {}

	/*
	* 할당된 메모리를 통해 MemoryHeader의 생성자를 실행 후 데이터 포인터 반환
	*/
	static void* AttachHeader(MemoryHeader* header, int32 size)
	{
		new(header)MemoryHeader(size);
		return reinterpret_cast<void*>(++header);
	}

	/*
	* MemoryHeader 포인터 반환
	*/
	static MemoryHeader* DetachHeader(void* ptr)
	{
		MemoryHeader* header = reinterpret_cast<MemoryHeader*>(ptr) - 1;
		return header;
	}

	int32 allocSize;  // 4바이트
	// TODO: 필요한 추가 정보
};

/*---------------
	MemoryPool
---------------*/

DECLSPEC_ALIGN(SLIST_ALIGNMENT)
class MemoryPool
{
public:
	MemoryPool(int32 allocSize);
	~MemoryPool();

	void			Push(MemoryHeader* ptr);
	MemoryHeader*	Pop();

private:
	SLIST_HEADER	_header;
	int32			_allocSize = 0;
	atomic<int32>	_useCount = 0;
	atomic<int32>	_reserveCount = 0;
};


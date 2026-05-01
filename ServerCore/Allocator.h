#pragma once

/*-------------------
	BaseAllocator
-------------------*/

// MAX_POOL_SIZE 초과 또는 전역 fallback용
class BaseAllocator
{
public:
	static void* Alloc(int32 size);
	static void  Release(void* ptr);

private:
	static constexpr uint32 AlignSize = Config::Memory::DEFAULT_ALIGNMENT;
};

/*-------------------
	StompAllocator
-------------------*/

// Use-After-Free / 버퍼 오버플로우 감지 (디버그 전용)
// 데이터 페이지 + Guard Page(PAGE_NOACCESS) 구조로 즉시 크래시 발생
class StompAllocator
{
public:
	static void* Alloc(int32 size);
	static void  Release(void* ptr);

private:
	static const int32 PAGE_SIZE;
	static constexpr uint32 AlignSize = Config::Memory::DEFAULT_ALIGNMENT;
};

/*-------------------
	PoolAllocator
-------------------*/

class PoolAllocator
{
public:
	static void* Alloc(int32 size);
	static void  Release(void* ptr);
};

/*-------------------
	FrameAllocator
-------------------*/

// 틱 단위 임시 할당 (bump pointer) - 해제 비용 없음
// 틱 끝에 LFrameAllocator->Clear() 호출로 일괄 초기화
class FrameAllocator
{
public:
	FrameAllocator(uint32 bufferSize = Config::Memory::Frame::DEFAULT_BUFFER_SIZE);
	~FrameAllocator();

	void* Alloc(uint32 size);
	void  Release(void* ptr) { /* no-op: Clear()로 일괄 해제 */ }
	void  Clear();

private:
	static constexpr uint32 AlignSize = Config::Memory::DEFAULT_ALIGNMENT;

	uint8*  _buffer    = nullptr;
	uint8*  _freePtr   = nullptr;
	uint8*  _endPtr    = nullptr;
	uint32  _bufferSize = 0;
};

/*-------------------
	STL Allocator
-------------------*/

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
			return static_cast<T*>(LFrameAllocator->Alloc(size));
		else if constexpr (Type == AllocType::Stomp)
			return static_cast<T*>(StompAllocator::Alloc(size));
		else
			return static_cast<T*>(PoolAllocator::Alloc(size));
	}

	void deallocate(T* ptr, size_t count)
	{
		if constexpr (Type == AllocType::Frame)
			return;  // Clear()로 일괄 해제
		else if constexpr (Type == AllocType::Stomp)
			StompAllocator::Release(ptr);
		else
			PoolAllocator::Release(ptr);
	}
};

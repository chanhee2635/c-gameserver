#pragma once

namespace Config
{
	namespace Memory
	{
		constexpr unsigned int SLIST_ALIGNMENT       = 16;
		constexpr unsigned int DEFAULT_ALIGNMENT     = 16;
		constexpr unsigned int MAX_POOL_SIZE         = 4096;
		// ~1024: 32 step (32ea), ~2048: 128 step (8ea), ~4096: 256 step (8ea)
		constexpr unsigned int POOL_COUNT            = (1024 / 32) + (1024 / 128) + (2048 / 256);
		constexpr unsigned int MAGIC_NUMBER          = 0xDEADBEEF;
		constexpr unsigned int ALLOC_COUNT           = 8;   // global pool 고갈 시 batch 할당 수
		constexpr unsigned int TLS_MAX_COUNT         = 64;  // TLS 최대 보유 블록 수
		constexpr unsigned int TLS_BATCH_COUNT       = 16;  // global <-> TLS 이동 단위

		namespace Frame
		{
			constexpr unsigned int DEFAULT_BUFFER_SIZE = 1024 * 1024;  // 1MB per thread
		}
	}

	namespace Buffer
	{
		// SendBufferChunk 크기 (64KB: 대부분의 패킷 여러 개를 한 청크에 담을 수 있는 크기)
		constexpr unsigned int SEND_BUFFER_CHUNK_SIZE = 65536;
	}

	namespace Network
	{
		// AcceptEx 주소 버퍼 크기: sizeof(SOCKADDR_IN)=16 + Winsock 추가 16 = 32
		// (CoreConfig.h 는 winsock2.h 보다 먼저 include되므로 sizeof 대신 상수 사용)
		constexpr int ADDR_BUFFER_SIZE = 32;
	}

	namespace Session
	{
		// 세션당 순환 수신 버퍼 크기 (64KB: 패킷 최대 크기 * 충분한 여유)
		constexpr int RECV_BUFFER_SIZE = 65536;
	}
}

// STL 컨테이너 할당 전략 선택자
enum class AllocType : unsigned char
{
	Pool,   // 기본: MemoryPool 사용
	Frame,  // 임시: FrameAllocator 사용 (틱 끝에 일괄 해제)
	Stomp,  // 디버그: VirtualAlloc + Guard Page
};

namespace MemoryUtils
{
	inline unsigned int AlignUp(unsigned int size, unsigned int align)
	{
		return (size + align - 1) & ~(align - 1);
	}
}

#pragma once

namespace Config
{
	static constexpr uint32 KB = 1024;
	static constexpr uint32 MB = 1024 * KB;

	namespace Memory
	{
		constexpr unsigned int DEFAULT_ALIGNMENT     = 16;
		constexpr unsigned int ALLOC_COUNT           = 32;
		constexpr unsigned int MAGIC_NUMBER          = 0xDEADBEEF;

		constexpr unsigned int MAX_POOL_SIZE         = 8192;
		constexpr unsigned int POOL_COUNT            = (1024 / 32) + (1024 / 128) + (2048 / 256) + (4096 / 512);

		constexpr unsigned int TLS_MAX_COUNT         = 32;  
		constexpr unsigned int TLS_BATCH_COUNT       = 16;  

		namespace Frame
		{
			constexpr unsigned int DEFAULT_BUFFER_SIZE = 1024 * 1024;  // 1MB per thread
		}
	}

	namespace Packet
	{
		static constexpr uint32 HEADER_SIZE = sizeof(uint16) * 2;
	}

	namespace Buffer
	{
		static constexpr uint32 SEND_BUFFER_CHUNK_SIZE = 4 * KB;
	}

	namespace Network
	{
		static constexpr uint32 ADDR_BUFFER_SIZE = 64;
		static constexpr uint32 ADDR_BUFFER_ALIGNMENT = 16;
	}

	namespace Session
	{
		static constexpr uint32 DEFAULT_ACCEPT_COUNT = 100;
		static constexpr uint32 RECV_BUFFER_SIZE = 4 * KB;
		static constexpr uint32 RECV_BUFFER_COUNT = 10;
		static constexpr uint32 MAX_PACKET_SIZE = 4 * KB;
	}

	namespace Job
	{
		static constexpr uint32 MAX_WORK_TICK = 64;
	}
}

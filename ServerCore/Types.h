#pragma once
#include <mutex>
#include <atomic>

// ── 스레드 역할 식별자 ────────────────────────────────────────────────
// LThreadType TLS 변수에 저장, 런타임 디버깅 및 역할별 분기에 활용
enum class ThreadType : uint8
{
    NONE    = 0,
    LOGIC   = 1,  // 게임 로직 / IOCP Worker
    IO      = 2,  // IOCP Dispatch 전용
    DB      = 3,  // DB 비동기 처리
    MONITOR = 4,  // 통계 / 모니터링
};

using BYTE		= unsigned char;
using int8		= __int8;
using int16		= __int16;
using int32		= __int32;
using int64		= __int64;
using uint8		= unsigned __int8;
using uint16	= unsigned __int16;
using uint32	= unsigned __int32;
using uint64	= unsigned __int64;
using uintptr	= uintptr_t;

template<typename T>
using Atomic		= std::atomic<T>;
using Mutex			= std::mutex;
using CondVar		= std::condition_variable;
using UniqueLock	= std::unique_lock<std::mutex>;
using LockGuard		= std::lock_guard<std::mutex>;

// shared_ptr

#define USING_SHARED_PTR(name) using name##Ref = std::shared_ptr<class name>;
USING_SHARED_PTR(IocpCore);
USING_SHARED_PTR(IocpObject);
USING_SHARED_PTR(Session);
USING_SHARED_PTR(PacketSession);
USING_SHARED_PTR(Listener);
USING_SHARED_PTR(Service);
USING_SHARED_PTR(ServerService);
USING_SHARED_PTR(ClientService);
USING_SHARED_PTR(LoginService);
USING_SHARED_PTR(ChatService);
// RecvBufferRef 제거: RecvBuffer 는 Session 값 멤버로 변경됨
USING_SHARED_PTR(SendBuffer);
USING_SHARED_PTR(SendBufferChunk);
USING_SHARED_PTR(Job);
USING_SHARED_PTR(JobQueue);
USING_SHARED_PTR(DBJobQueue);
USING_SHARED_PTR(DBConnection);

#define size16(val)		static_cast<int16>(sizeof(val))
#define size32(val)		static_cast<int32>(sizeof(val))
#define len16(arr)		static_cast<int16>(sizeof(arr)/sizeof(arr[0]))
#define len32(arr)		static_cast<int32>(sizeof(arr)/sizeof(arr[0]))

//#define _STOMP
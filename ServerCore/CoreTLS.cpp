#include "pch.h"
#include "CoreTLS.h"

// ── TLS 변수 정의 ───────────────────────────────────────────────────────────
thread_local ThreadType             LThreadType      = ThreadType::NONE;
thread_local uint32                 LThreadId        = 0;
thread_local uint64                 LEndTickCount    = 0;
thread_local uint64                 LEndDBTickCount  = 0;

thread_local DBConnectionRef        LDBConnection    = nullptr;

thread_local std::stack<int32>      LLockStack;
thread_local SendBufferChunkRef     LSendBufferChunk;
thread_local JobQueue*              LCurrentJobQueue = nullptr;

thread_local ThreadLocalMemory*     LThreadMemory    = nullptr;
thread_local FrameAllocator*        LFrameAllocator  = nullptr;

// ── CoreTLS 구현 ────────────────────────────────────────────────────────────

void CoreTLS::OnThreadStart(ThreadType type, uint32 id)
{
    LThreadType     = type;
    LThreadId       = id;

    // GMemory 가 반드시 먼저 초기화되어 있어야 함 (CoreGlobal 선언 순서 보장)
    LThreadMemory   = new ThreadLocalMemory();
    LFrameAllocator = new FrameAllocator();
}

void CoreTLS::OnThreadEnd()
{
    // SendBufferChunk ref 해제 (소멸자 호출 보장)
    LSendBufferChunk = nullptr;

    // TLS 보유 메모리 블록을 글로벌 풀에 반환 후 해제
    if (LThreadMemory)
    {
        delete LThreadMemory;
        LThreadMemory = nullptr;
    }

    if (LFrameAllocator)
    {
        delete LFrameAllocator;
        LFrameAllocator = nullptr;
    }
}

#include "pch.h"
#include "CoreTLS.h"

thread_local ThreadType             LThreadType         = ThreadType::NONE;
thread_local uint32                 LThreadId           = 0;
thread_local uint64				    LEndTickCount       = 0;
thread_local SendBufferChunkRef     LSendBufferChunk;
thread_local JobQueue*              LCurrentJobQueue    = nullptr;
thread_local ThreadLocalMemory*     LThreadMemory       = nullptr;
thread_local FrameAllocator*        LFrameAllocator     = nullptr;

void CoreTLS::OnThreadStart(ThreadType type, uint32 id)
{
    LThreadType     = type;
    LThreadId       = id;
    LThreadMemory   = new ThreadLocalMemory();
    LFrameAllocator = new FrameAllocator();
}

void CoreTLS::OnThreadEnd()
{
    LSendBufferChunk = nullptr;

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

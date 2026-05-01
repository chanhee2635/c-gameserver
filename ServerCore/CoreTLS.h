#pragma once
#include <stack>

// ── 스레드 역할 ────────────────────────────────────────────────────────────
extern thread_local ThreadType              LThreadType;       // 이 스레드의 역할 (LOGIC/DB/MONITOR...)

// ── 스레드 식별 / 시간 슬라이싱 ────────────────────────────────────────────
extern thread_local uint32                  LThreadId;
extern thread_local uint64                  LEndTickCount;     // 로직 스레드 타임슬라이스 만료 틱
extern thread_local uint64                  LEndDBTickCount;   // DB 스레드 타임슬라이스 만료 틱

// ── DB 전용 ───────────────────────────────────────────────────────────────
extern thread_local DBConnectionRef         LDBConnection;

// ── 락 / JobQueue ─────────────────────────────────────────────────────────
extern thread_local std::stack<int32>       LLockStack;        // DeadLockProfiler 용
extern thread_local SendBufferChunkRef      LSendBufferChunk;
extern thread_local class JobQueue*         LCurrentJobQueue;

// ── 메모리 ────────────────────────────────────────────────────────────────
extern thread_local class ThreadLocalMemory* LThreadMemory;
extern thread_local class FrameAllocator*    LFrameAllocator;

/*--------------
    CoreTLS
    스레드 생애주기 TLS 초기화/해제 담당
    ThreadManager::Launch() 가 자동 호출 → 개발자가 빠뜨릴 수 없음
---------------*/
class CoreTLS
{
public:
    /*
    * @brief 스레드 시작 시 호출 — LThreadType, LThreadId, 메모리 캐시 초기화
    * @param type  이 스레드의 역할 (ThreadType 열거형)
    * @param id    ThreadManager 가 부여한 고유 ID
    */
    static void OnThreadStart(ThreadType type, uint32 id);

    /*
    * @brief 스레드 종료 시 호출 — TLS 보유 메모리를 글로벌 풀에 반환 후 해제
    */
    static void OnThreadEnd();
};

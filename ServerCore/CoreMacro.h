#pragma once

#define OUT

#define NAMESPACE_BEGIN(name)   namespace name {
#define NAMESPACE_END           }

/*---------------
       Lock
       USE_LOCK / READ_LOCK / WRITE_LOCK  →  커스텀 Lock 클래스(DeadLockProfiler 연동)
       주석처리된 버전 = std::mutex 단순 버전 (성능 우선 시 교체)
---------------*/
#define USE_MANY_LOCKS(count)       Lock _locks[count];
#define USE_LOCK                    USE_MANY_LOCKS(1)
#define READ_LOCK_IDX(idx)          ReadLockGuard readLockGuard_##idx(_locks[idx], typeid(this).name());
#define READ_LOCK                   READ_LOCK_IDX(0)
#define WRITE_LOCK_IDX(idx)         WriteLockGuard writeLockGuard_##idx(_locks[idx], typeid(this).name());
#define WRITE_LOCK                  WRITE_LOCK_IDX(0)

//#define USE_LOCK                  std::shared_mutex _rwLock;
//#define READ_LOCK                 std::shared_lock<std::shared_mutex> readLock(_rwLock)
//#define WRITE_LOCK                std::unique_lock<std::shared_mutex> writeLock(_rwLock)

/*-----------------
        Crash
        - 원인 메시지 출력 (파일명 + 라인 포함)
        - 디버거 연결 시: __debugbreak() 로 해당 라인에서 즉시 중단
        - 릴리즈/덤프 용:  null 포인터 쓰기로 Access Violation → 크래시 덤프 생성
-----------------*/

#define CRASH(cause)                                                        \
do {                                                                        \
    std::cerr << "[CRASH] " << (cause)                                      \
              << "  (" << __FILE__ << ":" << __LINE__ << ")\n";             \
    std::cerr.flush();                                                      \
    __debugbreak();                                                         \
    uint32* crash = nullptr;                                                \
    __analysis_assume(crash != nullptr);                                    \
    *crash = 0xDEADBEEF;                                                    \
} while(0)

#define ASSERT_CRASH(expr)          \
do {                                \
    if (!(expr))                    \
    {                               \
        CRASH(#expr);               \
        __analysis_assume(expr);    \
    }                               \
} while(0)

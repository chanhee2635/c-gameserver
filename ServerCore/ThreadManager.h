#pragma once
#include <thread>
#include <functional>
#include <tuple>
// CoreTLS::OnThreadStart/OnThreadEnd 를 인라인 Launch 에서 직접 호출
#include "CoreTLS.h"

/*----------------------
    Threadmanager
-----------------------*/

class ThreadManager
{
public:
    ThreadManager();
    ~ThreadManager();

    /*
    * @brief 새 Worker 스레드를 생성하고 콜백을 실행
    * @param type     스레드 역할 (LOGIC / DB / MONITOR / ...)
    * @param callback 스레드가 실행할 함수 또는 람다
    * @param args     callback 에 전달할 인자 (perfect forwarding)
    *
    * @details
    *  - CoreTLS::OnThreadStart() → callback 실행 → CoreTLS::OnThreadEnd() 순서 보장
    *  - 개발자가 TLS 초기화를 직접 호출할 필요 없음
    */
    template<typename Fn, typename... Args>
    void Launch(ThreadType type, Fn&& callback, Args&&... args)
    {
        LockGuard guard(_lock);
        uint32 nextId = ++_threadIdCounter;

        // C++17: parameter pack 캡처는 tuple + std::apply 로 처리
        auto tup = std::make_tuple(std::forward<Args>(args)...);
        _threads.push_back(std::thread(
            [type, nextId,
             cb  = std::forward<Fn>(callback),
             t   = std::move(tup)]() mutable
            {
                CoreTLS::OnThreadStart(type, nextId);
                std::apply(std::move(cb), std::move(t));
                CoreTLS::OnThreadEnd();
            }
        ));
    }

    /* @brief 모든 살아있는 스레드가 끝날 때까지 대기 후 자원 해제 */
    void Join();

    /* ── 폴링 방식 GlobalQueue / JobTimer 제어 ── */
    static void DoGlobalQueueWork();
    static void DoDBQueueWork();
    static void DistributeReservedJobs();

private:
    Mutex               _lock;
    Vector<std::thread> _threads;
    Atomic<uint32>      _threadIdCounter = 0;  // 각 스레드에 고유 ID 부여
};

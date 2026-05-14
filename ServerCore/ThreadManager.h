#pragma once
#include "CoreTLS.h"

class ThreadManager
{
public:
    ThreadManager();
    ~ThreadManager();

    void InitMainThread(ThreadType type);

    template<typename T, typename... Args>
    void Launch(ThreadType type, T&& callback, Args&&... args);

    void Join();

    static void DoGlobalQueueWork();

private:
    Vector<Thread> _threads;
    Mutex          _lock;
    Atomic<uint32> _threadIdCounter = 0;
};

template<typename T, typename... Args>
void ThreadManager::Launch(ThreadType type, T&& callback, Args&&... args)
{
    uint32 nextId = _threadIdCounter.fetch_add(1);

    LockGuard guard(_lock);
    _threads.push_back(Thread([type, nextId, cb = std::forward<T>(callback), ...args = std::forward<Args>(args)]() mutable
        {
            CoreTLS::OnThreadStart(type, nextId);
            std::invoke(std::move(cb), std::move(args)...);
            CoreTLS::OnThreadEnd();
        })
    );
}

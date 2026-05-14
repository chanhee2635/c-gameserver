#include "pch.h"
#include "JobQueue.h"

void JobQueue::Push(Job job)
{
    bool pushToGlobal = false;
    {
        LockGuard lock(_lock);
        _jobs.push(std::move(job));

        if (_pending.exchange(true) == false)
            pushToGlobal = true;
    }

    if (pushToGlobal)
        GGlobalQueue->Push(shared_from_this());
}

void JobQueue::Execute()
{
    LCurrentJobQueue = this;
    const uint64 startTick = ::GetTickCount64();

    while (true)
    {
        Job job;
        {
            LockGuard lock(_lock);

            if (_jobs.empty())
            {
                _pending = false;
                break;
            }

            if (::GetTickCount64() - startTick > Config::Job::MAX_WORK_TICK)
            {
                GGlobalQueue->Push(shared_from_this());
                break;
            }

            job = std::move(_jobs.front());
            _jobs.pop();
        }

        if (job)
        {
            job();
            GServerStats->job.jobsExecuted.fetch_add(1, std::memory_order_relaxed);
        }
    }

    LCurrentJobQueue = nullptr;
}

void JobQueue::DoAsync(Job job)
{
    Push(std::move(job));
}

void JobQueue::DoTimer(uint32 afterMs, Job job)
{
    GJobTimer->Reserve(afterMs, shared_from_this(), std::move(job));
}

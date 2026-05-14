#pragma once
#include "JobQueue.h"

struct TimerItem
{
    uint64          executeTick = 0;
    JobQueueWeakRef owner;
    Job             job;

    bool operator>(const TimerItem& other) const
    {
        return executeTick > other.executeTick;
    }
};

class JobTimer
{
public:
    void Reserve(uint32 afterMs, JobQueueRef owner, Job job);
    void Distribute(uint64 nowTick);
    static uint64 Now() { return ::GetTickCount64(); }

private:
    USE_LOCK;
    PriorityQueue<TimerItem, Vector<TimerItem>, std::greater<TimerItem>> _items;
    Atomic<bool> _distributing = false;
};
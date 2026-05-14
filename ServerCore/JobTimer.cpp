#include "pch.h"
#include "JobTimer.h"
#include "JobQueue.h"

void JobTimer::Reserve(uint32 afterMs, JobQueueRef owner, Job job)
{
    const uint64 executeTick = Now() + afterMs;

    TimerItem item{ executeTick, owner, std::move(job) };

    {
        WRITE_LOCK;
        _items.push(std::move(item));
    }
}

void JobTimer::Distribute(uint64 nowTick)
{
    if (_distributing.exchange(true) == true)
        return;

    FrameVector<TimerItem> items;

    {
        READ_LOCK;
        if (_items.empty() || _items.top().executeTick > nowTick)
        {
            _distributing.store(false);
            return;
        }
        items.reserve(_items.size());
    }

    {
        WRITE_LOCK;
        while (!_items.empty())
        {
            if (nowTick < _items.top().executeTick)
                break;

            TimerItem& item = const_cast<TimerItem&>(_items.top());
            items.push_back(std::move(item));
            _items.pop();
        }
    }

    for (TimerItem& item : items)
    {
        if (JobQueueRef owner = item.owner.lock())
        {
            if (GServerStats) GServerStats->job.timerFired.fetch_add(1, std::memory_order_relaxed);
            owner->Push(std::move(item.job));
        }
    }

    _distributing.store(false);
}
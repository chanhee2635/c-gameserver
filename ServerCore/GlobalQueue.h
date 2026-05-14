#pragma once
#include "LockQueue.h"

class GlobalQueue
{
public:
    GlobalQueue() = default;
    ~GlobalQueue() = default;

    void Push(JobQueueRef jobQueue);
    bool TryPop(OUT JobQueueRef& jobQueue);

private:
    LockQueue<JobQueueRef>  _jobQueues;
};
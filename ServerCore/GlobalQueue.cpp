#include "pch.h"

void GlobalQueue::Push(JobQueueRef jobQueue)
{
    _jobQueues.Push(jobQueue);
}

bool GlobalQueue::TryPop(OUT JobQueueRef& jobQueue)
{
    return _jobQueues.TryPop(OUT jobQueue);
}
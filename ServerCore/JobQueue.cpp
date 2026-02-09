#include "pch.h"
#include "JobQueue.h"
#include "GlobalQueue.h"

/*-------------
	JobQueue
--------------*/

void JobQueue::PushGlobalQueue()
{
	GGlobalQueue->Push(shared_from_this());
}

uint64 JobQueue::GetWorkedTick()
{
	return LEndTickCount;
}

void JobQueue::Push(JobRef&& job, bool pushOnly)
{
	const int32 prevCount = _jobCount.fetch_add(1);
	MonitoringCountAdd(1); // JobCount 모니터링을 위한 전역 변수
	_jobs.Push(job); // WRITE_LOCK

	// 첫 번째 job을 넣은 스레드가 실행까지 담당
	if (prevCount == 0)
	{
		// 이미 실행중인 jobQueue가 없으면 실행 (재귀 방지)
		if (LCurrentJobQueue == nullptr && pushOnly == false)
		{
			Execute();
		}
		else
		{
			// 여유 있는 다른 스레드가 실행하도록 GlobalQueue에 넘긴다
			PushGlobalQueue();
		}
	}
}

// 1) 일감이 너무 몰리면?
void JobQueue::Execute()
{
	LCurrentJobQueue = this;

	while (true)
	{
		Vector<JobRef> jobs;
		_jobs.PopAll(OUT jobs);

		const int32 jobCount = static_cast<int32>(jobs.size());
		for (int32 i = 0; i < jobCount; i++)
			jobs[i]->Execute();

		MonitoringCountSub(jobCount);

		// 남은 일감이 0개라면 종료
		if (_jobCount.fetch_sub(jobCount) == jobCount)
		{
			LCurrentJobQueue = nullptr;
			return;
		}

		const uint64 now = ::GetTickCount64();
		if (now >= GetWorkedTick())
		{
			LCurrentJobQueue = nullptr;
			PushGlobalQueue();
			break;
		}
	}
}

/*--------------
	DBJobQueue
---------------*/

void DBJobQueue::PushGlobalQueue()
{
	GDBQueue->Push(shared_from_this());
}

uint64 DBJobQueue::GetWorkedTick()
{
	return LEndDBTickCount;
}

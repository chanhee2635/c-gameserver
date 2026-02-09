#pragma once
#include "Job.h"
#include "LockQueue.h"
#include "JobTimer.h"

/*-------------
	JobQueue
--------------*/

class JobQueue : public enable_shared_from_this<JobQueue>
{
public:
	void DoAsync(CallbackType&& callback)
	{
		Push(ObjectPool<Job>::MakeShared(std::move(callback)));
	}

	void DoAsyncPush(CallbackType&& callback)
	{
		Push(ObjectPool<Job>::MakeShared(std::move(callback)), true);
	}

	template<typename T, typename Ret, typename... Args>
	void DoAsync(Ret(T::* memFunc)(Args...), Args... args)
	{
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
		Push(ObjectPool<Job>::MakeShared(owner, memFunc, std::forward<Args>(args)...));
	}

	template<typename T, typename Ret, typename... Args>
	void DoAsyncPush(Ret(T::* memFunc)(Args...), Args... args)
	{
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
		Push(ObjectPool<Job>::MakeShared(owner, memFunc, std::forward<Args>(args)...), true);
	}

	void DoTimer(uint64 tickAfter, CallbackType&& callback)
	{
		JobRef job = ObjectPool<Job>::MakeShared(std::move(callback));
		GJobTimer->Reserve(tickAfter, shared_from_this(), job);
	}

	template<typename T, typename Ret, typename... Args>
	void DoTimer(uint64 tickAfter, Ret(T::* memFunc)(Args...), Args... args)
	{
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
		JobRef job = ObjectPool<Job>::MakeShared(owner, memFunc, std::forward<Args>(args)...);
		GJobTimer->Reserve(tickAfter, shared_from_this(), job);
	}

	void ClearJobs() { _jobs.Clear(); }

protected:
	virtual void PushGlobalQueue();
	virtual uint64 GetWorkedTick();

	virtual void MonitoringCountAdd(int32 count) { GLogicJobCount.fetch_add(count); }
	virtual void MonitoringCountSub(int32 count) { GLogicJobCount.fetch_sub(count); }

public:
	void Push(JobRef&& job, bool pushOnly = false);
	void Execute();

protected:
	LockQueue<JobRef> _jobs;
	Atomic<int32>	_jobCount = 0;
};

/*--------------
	DBJobQueue
---------------*/

class DBJobQueue : public JobQueue
{
protected:
	virtual void PushGlobalQueue() override;
	virtual uint64 GetWorkedTick() override;

	virtual void MonitoringCountAdd(int32 count) override { GDBJobCount.fetch_add(count); }
	virtual void MonitoringCountSub(int32 count) override { GDBJobCount.fetch_sub(count); }
};


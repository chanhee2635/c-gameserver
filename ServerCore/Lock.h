#pragma once
#include "Types.h"

/*-------------------
	RW SpinLock
-------------------*/

//[WWWWWWWW][WWWWWWWW][RRRRRRRR][RRRRRRRR]
// W : WriteFlag (Exclusive Lock Owner ThreadId)
// R : ReadFlag  (Shared Lock Count)

// W -> W (0)
// W -> R (0)
// R -> W (X)
class Lock
{
	enum : uint32
	{
		ACQUIRE_TIMEOUT_TICK = 10000,		// 락을 획득하려고 시도한 대기 시간
		MAX_SPIN_COUNT = 5000,				// 락을 획득하지 못했을 때 최대 스핀 횟수 
		WRITE_THREAD_MASK = 0xFFFF'0000,	// 쓰기 락을 획득한 스레드 ID를 저장하는 영역 (상위 16비트)
		READ_COUNT_MASK = 0x0000'FFFF,		// 읽기 락을 획득한 스레드 수를 저장하는 영역 (하위 16비트)
		EMPTY_FLAG = 0x0000'0000
	};
public:
	void WriteLock(const char* name);
	void WriteUnlock(const char* name);
	void ReadLock(const char* name);
	void ReadUnlock(const char* name);

private:
	Atomic<uint32> _lockFlag = EMPTY_FLAG;
	uint16 _writeCount = 0;
};

/*---------------
	LockGuards
----------------*/

class ReadLockGuard
{
public:
	ReadLockGuard(Lock& lock, const char* name) : _lock(lock), _name(name) { _lock.ReadLock(name); }
	~ReadLockGuard() { _lock.ReadUnlock(_name); }
private:
	Lock& _lock;
	const char* _name;
};

class WriteLockGuard
{
public:
	WriteLockGuard(Lock& lock, const char* name) : _lock(lock), _name(name) { _lock.WriteLock(name); }
	~WriteLockGuard() { _lock.WriteUnlock(_name); }
private:
	Lock& _lock;
	const char* _name;
};
#include "pch.h"
#include "DBService.h"
#include "DBConnectionPool.h"
#include "ThreadManager.h"

unique_ptr<DBService> GDBService = make_unique<DBService>();

void DBService::InitTLS()
{
	// 전역 풀에 접근할 때 Lock 경합을 최소화 하기 위함
	// ODBC 특성상 연결 객체 하나당 한 번에 하나의 명령만 처리할 수 있는 입출력 제한...
	// 트랜잭션 구현을 위함 (프로시저로 대체 가능)
	LDBConnection = GDBConnectionPool->Pop();
}

void DBService::Start(int32 threadCount, uint64 workedTick)
{
	_threadCount = threadCount;
	_workedTick = workedTick;

	for (int32 i = 0; i < _threadCount; ++i)
	{
		GThreadManager->Launch([this]() {
			InitTLS();
			while (true)
			{
				LEndDBTickCount = ::GetTickCount64() + _workedTick;
				ThreadManager::DoDBQueueWork();
				this_thread::sleep_for(1ms);
			}
			DestroyTLS();
		});
	}
}

void DBService::DestroyTLS()
{
	// RefCount가 0이 되며 자동으로 Pool에 반납
	LDBConnection = nullptr;
}

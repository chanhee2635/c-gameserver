#include "pch.h"
#include "DBService.h"
#include "DBConnectionPool.h"
#include "ThreadManager.h"

unique_ptr<DBService> GDBService = make_unique<DBService>();

void DBService::InitTLS()
{
	// ���� Ǯ�� ������ �� Lock ������ �ּ�ȭ �ϱ� ����
	// ODBC Ư���� ���� ��ü �ϳ��� �� ���� �ϳ��� ���ɸ� ó���� �� �ִ� ����� ����...
	// Ʈ����� ������ ���� (���ν����� ��ü ����)
	LDBConnection = GDBConnectionPool->Pop();
}

void DBService::Start(int32 threadCount, uint64 workedTick)
{
	_threadCount = threadCount;
	_workedTick = workedTick;

	for (int32 i = 0; i < _threadCount; ++i)
	{
		GThreadManager->Launch(ThreadType::DB, [this]() {
			// CoreTLS::OnThreadStart 가 메모리 TLS 초기화 완료
			// DB 연결은 DB 스레드 전용 (ODBC 핸들은 스레드당 하나)
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
	// RefCount�� 0�� �Ǹ� �ڵ����� Pool�� �ݳ�
	LDBConnection = nullptr;
}

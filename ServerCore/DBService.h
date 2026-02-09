#pragma once
class DBService
{
public:
	/*
	* @brief DB 전용 워커 스레드 생성 및 실행
	* @param threadCount 생성할 DB 스레드 개수
	* @param workedTick 한 루프당 작업 시간
	*/
	void Start(int32 threadCount, uint64 workedTick);

private:
	void InitTLS();
	void DestroyTLS();

private:
	int32 _threadCount = 0;
	uint64 _workedTick = 0;
};

extern std::unique_ptr<DBService>	GDBService;

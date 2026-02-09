#pragma once
#include "DBConnection.h"

/*--------------------
	DBConnectionPool
---------------------*/

class DBConnectionPool
{
public:
	DBConnectionPool() {}
	~DBConnectionPool() { Clear(); }

	/*
	* @brief DB Connection Pool을 초기화하고 연결을 수행한다.
	* @param connectionCount 생성할 커넥션 객체 수 (스레드 고려)
	* @param connectionString DB 접속 정보 문자열
	* @return 모든 커넥션이 성공하면 true, 하나라도 연결이 실패하면 false 반환
	*/
	bool			Connect(int32 connectionCount, const WCHAR* connectionString);
	/*
	* @brief DB Connection Pool 자원 해제 및 ODBC 환경 정리
	* @details 모든 스레드가 종료(Join)된 후 메인 스레드에서 한 번 호출
	*/
	void			Clear();
	DBConnectionRef	Pop();
	void			Push(DBConnection* connection);

private:
	USE_LOCK;
	SQLHENV					_environment = SQL_NULL_HANDLE;  // ODBC 환경의 모든 정보를 저장하고 관리하는 핸들
	// shared_ptr 을 사용하지 않은 이유 : DBConnection을 이리 저리 사용할 것이 아닌 하나의 함수에서 꺼내서 작업을 실행하고 바로 집어넣기 때문 
	Vector<DBConnection*>	_connections;  // DB 연결을 재사용하기 위해 저장할 공간
};

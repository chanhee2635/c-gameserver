#pragma once
#include "Session.h"

class LoginSession : public PacketSession
{
public:
	~LoginSession()
	{
		cout << "~LoginSession" << endl;
	}

	virtual void OnConnected() override;
	// 패킷 데이터가 전부 도착했을 때 호출
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;
	virtual void OnDisconnected() override;

	JobQueueRef GetLogicQueue() { return _logicQueue; }
	DBJobQueueRef GetDBQueue() { return _dbQueue; }

	/*
	* @brief 클라이언트 로그인 요청 처리 (ID/PW 검증 및 비동기 DB 인증 수행)
	*/
	void HandleLoginStart(const string& id, const string& pw);
	/*
	* @brief DB 인증 결과 처리 및 패킷 전송 (성공 시 토큰 발행 및 서버 목록 전달)
	*/
	void HandleLoginEnd(bool success, uint64 dbId);

	void HandleJoinStart(const string& id, const string& pw);

	void HandleJoinEnd(bool success);

	string GenerateUUID();

private:
	// 패킷마다 JobQueue를 가지고 있는 이유
	// 유저끼리의 상호작용이 없기 때문에 한 클라이언트의 순서만 보장
	JobQueueRef _logicQueue;
	DBJobQueueRef _dbQueue;
};


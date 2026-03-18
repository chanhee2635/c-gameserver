#pragma once
#include "IocpCore.h"
#include "IocpEvent.h"
#include "NetAddress.h"
#include "RecvBuffer.h"

class Service;

struct SendBufferNode {
	SendBufferRef buffer;
	SendBufferNode* next = nullptr;
};

/*----------------
	  Session
-----------------*/

class Session : public IocpObject
{
	// 아래 클래스에선 Session의 모든 멤버 접근 가능
	friend class Listener;
	friend class IocpCore;
	friend class Service;

public:
	Session();
	virtual ~Session();

public:
	// 외부에서 사용
	void					Init();
	// 패킷 전송 요청
	void					Send(SendBufferRef sendBuffer);
	/*@brief 즉시 Send하지 않고 큐에 저장*/
	void					PushSendBuffer(SendBufferRef sendBuffer);
	void					FlushSend();
	bool					Connect();
	void					Disconnect(const WCHAR* cause);

	shared_ptr<Service>		GetService() { return _service.lock(); }
	void					SetService(shared_ptr<Service> service) { _service = service; Init(); }

public:
	// 정보 관련
	void					SetNetAddress(NetAddress address) { _netAddress = address; }
	NetAddress				GetAddress() { return _netAddress; }
	SOCKET					GetSocket() { return _socket; }
	bool					IsConnected() { return _connected; }
	SessionRef				GetSessionRef() { return static_pointer_cast<Session>(shared_from_this()); }

public:
	// 인터페이스 구현
	virtual HANDLE			GetHandle() override;
	virtual void			Dispatch(struct IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
	// 전송 관련
	bool					RegisterConnect();
	bool					RegisterDisconnect();
	// 비동기 데이터 수신을 커널에 예약
	void					RegisterRecv();
	// 비동기 데이터 전송을 커널에 예약
	void					RegisterSend();

	// 접속 완료 후 세션을 실질적으로 가동시킨다.
	void					ProcessConnect();
	void					ProcessDisconnect();
	// 비동기 수신 완료 후 데이터를 처리하고 다음 수신 준비
	void					ProcessRecv(int32 numOfBytes);
	// 비동기 전송 완료 후 다음 Send에 전송하도록 준비
	void					ProcessSend(IocpEvent* sendEvent, int32 numOfBytes);

	// 소켓 작업 중 발생한 에러를 처리
	void					HandleError(int32 errorCode);

protected:
	// 컨텐츠 코드에서 재정의
	virtual void			OnConnected() { }
	virtual int32			OnRecv(BYTE* buffer, int32 len) { return len; }
	virtual void			OnSend(int32 len) { }
	virtual void			OnDisconnected() { }

private:
	weak_ptr<Service>		_service;  	/* 순환 참조 해결을 위해 weak_ptr 사용 */
	SOCKET					_socket = INVALID_SOCKET;
	NetAddress				_netAddress = {};
	Atomic<bool>			_connected = false;

private:
	USE_LOCK;

	// 수신 관련
	RecvBufferRef			_recvBuffer;

	// 송신 관련
	queue<SendBufferRef>	_sendQueue;
	//vector<SendBufferRef>	_sendBuffers;
	std::atomic<SendBufferNode*> _sendBufferHead{ nullptr };
	atomic<bool>			_sendRegistered = false;

private:
	// IocpEvent 재사용
	IocpEvent				_connectEvent{ EventType::Connect };
	IocpEvent				_disconnectEvent{ EventType::Disconnect };
	IocpEvent				_recvEvent{ EventType::Recv };
	//IocpEvent				_sendEvent{ EventType::Send };
};

/*-------------------
    PacketSession
--------------------*/

// 정책
// [ PacketHeader ]	[data]
// [size(2)][id(2)]	[data]
struct PacketHeader  
{
	uint16 size;
	uint16 id;
};

class PacketSession : public Session
{
public:
	PacketSession();
	virtual ~PacketSession();

	PacketSessionRef	GetPacketSessionRef() { return static_pointer_cast<PacketSession>(shared_from_this()); }

protected:
	/*
	* @brief 수신된 바이트 스트림을 패킷 단위로 조립
	* @param buffer 수신 버퍼의 읽기 시작 지점
	* @param len 현재 버퍼에 쌓여있는 전체 데이터 크기
	* @return 처리 완료한 바이트 크기 (다음 처리 지점이 됨)
	*/
	virtual int32		OnRecv(BYTE* buffer, int32 len) sealed;
	virtual void		OnRecvPacket(BYTE* buffer, int32 len) abstract;
};
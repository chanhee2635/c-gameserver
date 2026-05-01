#pragma once
#include "IocpCore.h"
#include "IocpEvent.h"
#include "NetAddress.h"
#include "RecvBuffer.h"

class Service;

/*----------------
	  Session
-----------------*/

class Session : public IocpObject
{
	friend class Listener;
	friend class IocpCore;
	friend class Service;

public:
	Session();
	virtual ~Session();

public:
	// 외부에서 사용
	void					Send(SendBufferRef sendBuffer);
	bool					Connect();
	void					Disconnect(const WCHAR* cause = L"");

	shared_ptr<Service>		GetService() { return _service.lock(); }
	void					SetService(shared_ptr<Service> service) { _service = service; }

public:
	// 세션 정보
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
	// 등록 함수
	bool					RegisterConnect();
	bool					RegisterDisconnect();
	void					RegisterRecv();
	void					RegisterSend();

	// 완료 처리 함수
	void					ProcessConnect();
	void					ProcessDisconnect();
	void					ProcessRecv(int32 numOfBytes);
	void					ProcessSend(int32 numOfBytes);  // sendEvent 는 _sendEvent 멤버 사용

	void					HandleError(int32 errorCode);

protected:
	virtual void			OnConnected() { }
	virtual int32			OnRecv(BYTE* buffer, int32 len) { return len; }
	virtual void			OnSend(int32 len) { }
	virtual void			OnDisconnected() { }

private:
	weak_ptr<Service>		_service;
	SOCKET					_socket = INVALID_SOCKET;
	NetAddress				_netAddress = {};
	Atomic<bool>			_connected = false;

private:
	// ─── 수신 ─────────────────────────────────────────────
	// 값 멤버: 힙 할당 없음, 생성자에서 바로 초기화
	RecvBuffer				_recvBuffer;

	// ─── 송신 ─────────────────────────────────────────────
	USE_LOCK;

	// 생산자(Send 호출) 가 쌓는 큐
	Vector<SendBufferRef>	_sendQueue;
	// RegisterSend 에서 swap 한 뒤 WSASend 에 넘기는 배치
	Vector<SendBufferRef>	_sendPendingList;

	Atomic<bool>			_sendRegistered = false;

private:
	// ─── IOCP 이벤트 (모두 값 멤버 → 힙 할당 없음) ────────
	IocpEvent				_connectEvent    { EventType::Connect    };
	IocpEvent				_disconnectEvent { EventType::Disconnect };
	IocpEvent				_recvEvent       { EventType::Recv       };
	SendEvent				_sendEvent;   // sendBuffers / wsaBufs 내장
};

/*-------------------
    PacketSession
--------------------*/

// 패킷 구조
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
	virtual int32		OnRecv(BYTE* buffer, int32 len) sealed;
	virtual void		OnRecvPacket(BYTE* buffer, int32 len) abstract;
};

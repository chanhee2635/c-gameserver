#pragma once

class Session;

enum class EventType : uint8
{
	Connect,
	Disconnect,
	Accept,
	Recv,
	Send
};

/*-----------------
	 IocpEvent
	 (최소 공통 베이스: OVERLAPPED + type + owner)
-----------------*/
struct IocpEvent : public OVERLAPPED
{
	IocpEvent(EventType type);

	// IOCP 재사용 전 OVERLAPPED 초기화
	void			Init();

	EventType		type;
	IocpObjectRef	owner;
};

/*-----------------
	 SendEvent
	 Session의 멤버 변수로 보유 → 힙 할당 제거
-----------------*/
struct SendEvent : public IocpEvent
{
	SendEvent() : IocpEvent(EventType::Send) {}

	Vector<SendBufferRef>	sendBuffers;  // 전송 완료 전까지 참조 유지
	Vector<WSABUF>			wsaBufs;      // WSASend 용 scatter/gather 배열
};

/*-----------------
	 AcceptEvent
	 Listener의 멤버로 보유 → 힙 할당 제거
	 AcceptEx 주소 버퍼도 내부에 보유 (session->_recvBuffer 오염 제거)
-----------------*/
struct AcceptEvent : public IocpEvent
{
	AcceptEvent() : IocpEvent(EventType::Accept) {}

	SessionRef	session = nullptr;
	// AcceptEx: 로컬 + 원격 주소를 각각 ADDR_BUFFER_SIZE 크기로 저장
	BYTE		addrBuffer[Config::Network::ADDR_BUFFER_SIZE * 2] = {};
};

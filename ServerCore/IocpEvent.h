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
-----------------*/

/*
* @brief IOCP 비동기 작업의 Context(문맥)를 저장하는 구조체
* @details 
* 1. GetQueueComletionStatus 함수의 작업 완료 시 LPOVERLAPPED 포인터를 반환
* 2. IocpEvent가 OVERLAPPED를 상속받았기 때문에, 메모리 주소상 IocpEvent의 시작 지점은 OVERLAPPED의 시작 지점과 일치
* 3. LPOVERLAPPED 주소를 static_cast<IocpEvent*>로 변환하면 멤버 변수에 즉시 접근 가능
*/
struct IocpEvent : public OVERLAPPED
{
	IocpEvent(EventType type);

	// OVERLAPPED 구조체 및 이벤트를 재사용하기 위해 초기화
	void			Init();

	EventType		type;   // 이벤트 타입
	IocpObjectRef	owner;  // 이벤트 처리 주체
	SessionRef		session = nullptr;  // 세션을 전달할 임시 저장소

	vector<BYTE> buffer;  // 미사용: 세션의 _recvBuffer 를 사용
	vector<SendBufferRef> sendBuffers; // 여러 버퍼를 한 번에 보낼 때 사용
	vector<WSABUF> wsaBufs;
};


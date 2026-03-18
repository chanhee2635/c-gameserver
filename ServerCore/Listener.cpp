#include "pch.h"
#include "Listener.h"
#include "SocketUtils.h"
#include "IocpEvent.h"
#include "Session.h"
#include "Service.h"

/*---------------
     Listener
---------------*/

Listener::~Listener()
{
    SocketUtils::Close(_socket); 

    // 순환참조 끊기
    for (IocpEvent* acceptEvent : _acceptEvents) 
    {
        delete acceptEvent;
    }
    _service = nullptr;
}

bool Listener::StartAccept(ServiceRef service)
{
    _service = service;
    if (service == nullptr) return false;

    // 서버 통신을 위한 리슨 소켓 생성
    _socket = SocketUtils::CreateSocket();
    if (_socket == INVALID_SOCKET) return false;

    // 리스너 객체를 IOCP 핸들에 등록하여 완료 통지를 받도록 설정
    if (service->GetIocpCore()->Register(shared_from_this()) == false) return false;

    // 소켓 옵션 설정
    if (SocketUtils::SetReuseAddress(_socket, true) == false) return false;
    if (SocketUtils::SetLinger(_socket, 0, 0) == false) return false;

    // 서버 주소 바인딩 및 커널 수신 큐 활성화
    if (SocketUtils::Bind(_socket, service->GetNetAddress()) == false) return false;
    if (SocketUtils::Listen(_socket) == false) return false;

    // 접속 폭주(초당 접속 수)를 대비해서 미리 Accept 이벤트를 생성하여 커널에 등록
    // 너무 많이 등록하면 메모리 낭비 발생
    const int32 acceptCount = service->GetAcceptCount();
    for (int32 i = 0; i < acceptCount; i++)
    {
        IocpEvent* acceptEvent = new IocpEvent(EventType::Accept); 
        acceptEvent->owner = shared_from_this(); 
        _acceptEvents.push_back(acceptEvent);
        RegisterAccept(acceptEvent);
    }

    return true;
}

void Listener::CloseSocket()
{
    SocketUtils::Close(_socket);
}

HANDLE Listener::GetHandle()
{
    return reinterpret_cast<HANDLE>(_socket);
}


void Listener::Dispatch(IocpEvent* iocpEvent, int32 numOfByte)
{
    if (iocpEvent->type == EventType::Accept)
        ProcessAccept(iocpEvent);
}

void Listener::RegisterAccept(IocpEvent* acceptEvent)
{
    // 접속 클라이언트를 담을 세션 미리 생성
    SessionRef session = _service->CreateSession();

    acceptEvent->Init();
    acceptEvent->session = session;

    DWORD bytesReceived = 0;
    // 세션의 수신 버퍼에 주소 정보를 직접 저장 및 Winsock 표준에 따른 주소 공간 확보(SOCKADDR_IN + 16)
    if (false == SocketUtils::AcceptEx(_socket, session->GetSocket(), session->_recvBuffer->WritePos(), 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, OUT &bytesReceived, static_cast<LPOVERLAPPED>(acceptEvent)))
    {
        const int32 errorCode = ::WSAGetLastError();
        if (errorCode != WSA_IO_PENDING)
        {
            // 에러 발생 시 로그 출력 및 재시도
            session->HandleError(errorCode);
            RegisterAccept(acceptEvent);
        }
    }
}

void Listener::ProcessAccept(IocpEvent* acceptEvent)
{
    SessionRef session = acceptEvent->session;

    // 리슨 소켓의 속성을 새로 접속한 소켓에 상속
    if (false == SocketUtils::SetUpdateAcceptSocket(session->GetSocket(), _socket))
    {
        const int32 errorCode = ::WSAGetLastError();
        session->HandleError(errorCode);    // 문제 확인
        RegisterAccept(acceptEvent);    // 실패 시 재시도
        return;
    }

    // 연결된 클라이언트 주소 정보 추출
    SOCKADDR_IN sockAddress;
    int32 sizeOfSockAddr = sizeof(sockAddress);
    if (SOCKET_ERROR == ::getpeername(session->GetSocket(), OUT reinterpret_cast<SOCKADDR*>(&sockAddress), &sizeOfSockAddr))
    {
        const int32 errorCode = ::WSAGetLastError();
        session->HandleError(errorCode);    // 문제 확인
        RegisterAccept(acceptEvent);    // 실패 시 재시도
        return;
    }

    // 주소 저장 및 실제 서비스 시작
    session->SetNetAddress(NetAddress(sockAddress));
    session->ProcessConnect();

    // IocpEvent 재사용으로 추가적인 메모리 할당X
    RegisterAccept(acceptEvent);
}
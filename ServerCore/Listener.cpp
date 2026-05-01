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

    for (AcceptEvent* acceptEvent : _acceptEvents)
    {
        delete acceptEvent;
    }
    _service = nullptr;
}

bool Listener::StartAccept(ServiceRef service)
{
    _service = service;
    if (service == nullptr) return false;

    _socket = SocketUtils::CreateSocket();
    if (_socket == INVALID_SOCKET) return false;

    if (service->GetIocpCore()->Register(shared_from_this()) == false) return false;

    if (SocketUtils::SetReuseAddress(_socket, true) == false) return false;
    if (SocketUtils::SetLinger(_socket, 0, 0) == false) return false;

    if (SocketUtils::Bind(_socket, service->GetNetAddress()) == false) return false;
    if (SocketUtils::Listen(_socket) == false) return false;

    const int32 acceptCount = service->GetAcceptCount();
    for (int32 i = 0; i < acceptCount; i++)
    {
        // AcceptEvent: 자체 addrBuffer 보유 → session->_recvBuffer 오염 없음
        AcceptEvent* acceptEvent = new AcceptEvent();
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
        ProcessAccept(static_cast<AcceptEvent*>(iocpEvent));
}

void Listener::RegisterAccept(AcceptEvent* acceptEvent)
{
    SessionRef session = _service->CreateSession();

    acceptEvent->Init();
    acceptEvent->session = session;
    // addrBuffer 초기화 (재사용 시 이전 데이터 오염 방지)
    ::memset(acceptEvent->addrBuffer, 0, sizeof(acceptEvent->addrBuffer));

    DWORD bytesReceived = 0;
    // AcceptEx: 실제 데이터 수신 크기 = 0 (주소만 받음)
    // 로컬/원격 주소를 각각 ADDR_BUFFER_SIZE 바이트씩 addrBuffer 에 저장
    if (false == SocketUtils::AcceptEx(
        _socket,
        session->GetSocket(),
        acceptEvent->addrBuffer,          // 주소 전용 버퍼 (session 수신 버퍼 오염 없음)
        0,                                // 데이터 수신 크기 = 0
        Config::Network::ADDR_BUFFER_SIZE,
        Config::Network::ADDR_BUFFER_SIZE,
        OUT &bytesReceived,
        static_cast<LPOVERLAPPED>(acceptEvent)))
    {
        const int32 errorCode = ::WSAGetLastError();
        if (errorCode != WSA_IO_PENDING)
        {
            session->HandleError(errorCode);
            RegisterAccept(acceptEvent);
        }
    }
}

void Listener::ProcessAccept(AcceptEvent* acceptEvent)
{
    SessionRef session = acceptEvent->session;

    if (false == SocketUtils::SetUpdateAcceptSocket(session->GetSocket(), _socket))
    {
        const int32 errorCode = ::WSAGetLastError();
        session->HandleError(errorCode);
        RegisterAccept(acceptEvent);
        return;
    }

    SOCKADDR_IN sockAddress;
    int32 sizeOfSockAddr = sizeof(sockAddress);
    if (SOCKET_ERROR == ::getpeername(
        session->GetSocket(),
        OUT reinterpret_cast<SOCKADDR*>(&sockAddress),
        &sizeOfSockAddr))
    {
        const int32 errorCode = ::WSAGetLastError();
        session->HandleError(errorCode);
        RegisterAccept(acceptEvent);
        return;
    }

    session->SetNetAddress(NetAddress(sockAddress));
    session->ProcessConnect();

    RegisterAccept(acceptEvent);
}

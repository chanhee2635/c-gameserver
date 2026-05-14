#include "pch.h"
#include "Listener.h"
#include "IocpEvent.h"
#include "Session.h"
#include "Service.h"

static constexpr int32 SOCKADDR_BUFFER_SIZE = sizeof(SOCKADDR_IN) + 16;

Listener::~Listener()
{
    SocketUtils::Close(_listenSocket);
    for (AcceptEvent* acceptEvent : _acceptEvents)
        xdelete(acceptEvent);
    _acceptEvents.clear();
}

bool Listener::StartAccept(ServerServiceRef service)
{
    _service = service;
    _listenSocket = SocketUtils::CreateSocket();
    if (_listenSocket == INVALID_SOCKET) return false;

    if (!_service->GetIocpCore()->Register(shared_from_this())) return false;

    if (!SocketUtils::SetReuseAddress(_listenSocket, true)) return false;
    if (!SocketUtils::SetLinger(_listenSocket, 0, 0))       return false;
    if (!SocketUtils::Bind(_listenSocket, _service->GetAddress())) return false;
    if (!SocketUtils::Listen(_listenSocket))                return false;

    const int32 acceptCount = _service->GetMaxSessionCount();
    for (int32 i = 0; i < acceptCount; ++i)
    {
        AcceptEvent* acceptEvent = xnew<AcceptEvent>();
        acceptEvent->SetOwner(shared_from_this());
        _acceptEvents.push_back(acceptEvent);
        RegisterAccept(acceptEvent);
    }

    return true;
}

void Listener::Close()
{
    SocketUtils::Close(_listenSocket);
}

void Listener::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
    ASSERT_CRASH(iocpEvent->GetType() == IocpEventType::Accept);
    ProcessAccept(static_cast<AcceptEvent*>(iocpEvent));
}

void Listener::RegisterAccept(AcceptEvent* acceptEvent)
{
    SessionRef session = _service->CreateSession();
    acceptEvent->SetSession(session);
    acceptEvent->Init();

    DWORD bytesReceived = 0;
    if (!SocketUtils::AcceptEx(_listenSocket, session->GetSocket(),
        acceptEvent->GetAddressBuffer(), 0,
        SOCKADDR_BUFFER_SIZE, SOCKADDR_BUFFER_SIZE,
        OUT &bytesReceived, static_cast<LPOVERLAPPED>(acceptEvent)))
    {
        const int32 errCode = ::WSAGetLastError();
        if (errCode != WSA_IO_PENDING)
        {
            LOG_ERROR(L"AcceptEx failed errCode=" + std::to_wstring(errCode));
            RegisterAccept(acceptEvent);
        }
    }
}

void Listener::ProcessAccept(AcceptEvent* acceptEvent)
{
    SessionRef session = acceptEvent->GetSession();

    if (!SocketUtils::SetUpdateAcceptSocket(session->GetSocket(), _listenSocket))
    {
        LOG_WARN(L"SetUpdateAcceptSocket failed, retrying accept");
        RegisterAccept(acceptEvent);
        return;
    }

    SOCKADDR_IN* localAddr  = nullptr;
    SOCKADDR_IN* remoteAddr = nullptr;
    int32 localLen  = 0;
    int32 remoteLen = 0;
    ::GetAcceptExSockaddrs(acceptEvent->GetAddressBuffer(), 0,
        SOCKADDR_BUFFER_SIZE, SOCKADDR_BUFFER_SIZE,
        reinterpret_cast<SOCKADDR**>(&localAddr),  &localLen,
        reinterpret_cast<SOCKADDR**>(&remoteAddr), &remoteLen);

    if (remoteAddr)
        session->SetNetAddress(NetAddress(*remoteAddr));

    session->ProcessConnect();
    RegisterAccept(acceptEvent);
}

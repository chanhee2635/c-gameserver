#include "pch.h"
#include "SocketUtils.h"

LPFN_ACCEPTEX     SocketUtils::AcceptEx    = nullptr;
LPFN_CONNECTEX    SocketUtils::ConnectEx   = nullptr;
LPFN_DISCONNECTEX SocketUtils::DisconnectEx = nullptr;

void SocketUtils::Init()
{
    WSADATA wsaData;
    ASSERT_CRASH(::WSAStartup(MAKEWORD(2, 2), OUT &wsaData) == 0);

    SOCKET dummySocket = CreateSocket();
    ASSERT_CRASH(dummySocket != INVALID_SOCKET);
    bool success = BindWindowsFunction(dummySocket, WSAID_ACCEPTEX,    AcceptEx)
                && BindWindowsFunction(dummySocket, WSAID_CONNECTEX,   ConnectEx)
                && BindWindowsFunction(dummySocket, WSAID_DISCONNECTEX, DisconnectEx);

    Close(dummySocket);
    ASSERT_CRASH(success);
}

void SocketUtils::Clear()
{
    ::WSACleanup();
}

SOCKET SocketUtils::CreateSocket()
{
    return ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
}

bool SocketUtils::SetLinger(SOCKET socket, uint16 onoff, uint16 linger)
{
    LINGER option;
    option.l_onoff  = onoff;
    option.l_linger = linger;
    return SetSockOpt(socket, SOL_SOCKET, SO_LINGER, option);
}

bool SocketUtils::SetTcpNoDelay(SOCKET socket, bool flag)
{
    return SetSockOpt(socket, IPPROTO_TCP, TCP_NODELAY, flag);
}

bool SocketUtils::SetReuseAddress(SOCKET socket, bool flag)
{
    return SetSockOpt(socket, SOL_SOCKET, SO_REUSEADDR, flag);
}

bool SocketUtils::SetRecvBufferSize(SOCKET socket, int32 size)
{
    return SetSockOpt(socket, SOL_SOCKET, SO_RCVBUF, size);
}

bool SocketUtils::SetSendBufferSize(SOCKET socket, int32 size)
{
    return SetSockOpt(socket, SOL_SOCKET, SO_SNDBUF, size);
}

bool SocketUtils::SetUpdateAcceptSocket(SOCKET socket, SOCKET listenSocket)
{
    return SetSockOpt(socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, listenSocket);
}

bool SocketUtils::Bind(SOCKET socket, const NetAddress& netAddr)
{
    return ::bind(socket, reinterpret_cast<const SOCKADDR*>(&netAddr.GetSockAddr()), sizeof(sockaddr_in)) != SOCKET_ERROR;
}

bool SocketUtils::BindAnyAddress(SOCKET socket, uint16 port)
{
    SOCKADDR_IN myAddress;
    ::memset(&myAddress, 0, sizeof(myAddress));
    myAddress.sin_family      = AF_INET;
    myAddress.sin_addr.s_addr = ::htonl(INADDR_ANY);
    myAddress.sin_port        = ::htons(port);

    return ::bind(socket, reinterpret_cast<const SOCKADDR*>(&myAddress), sizeof(myAddress)) != SOCKET_ERROR;
}

bool SocketUtils::Listen(SOCKET socket, int32 backlog)
{
    return ::listen(socket, backlog) != SOCKET_ERROR;
}

void SocketUtils::Close(SOCKET& socket)
{
    if (socket == INVALID_SOCKET)
        return;

    ::closesocket(socket);
    socket = INVALID_SOCKET;
}

template<typename T>
bool SocketUtils::BindWindowsFunction(SOCKET socket, GUID guid, T& fn)
{
    DWORD bytes = 0;
    return ::WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guid, sizeof(guid), &fn, sizeof(fn), &bytes, NULL, NULL) != SOCKET_ERROR;
}

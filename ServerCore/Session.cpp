#include "pch.h"
#include "Session.h"
#include "Service.h"

Session::Session()
{
    _socket = SocketUtils::CreateSocket();
}

Session::~Session()
{
    SocketUtils::Close(_socket);
}

void Session::Send(SendBufferRef sendBuffer)
{
    if (!_connected)
        return;

    bool registerSend = false;
    {
        LockGuard guard(_sendLock);
        _sendQueue.push_back(sendBuffer);

        if (!_isSendRegistered)
        {
            _isSendRegistered = true;
            registerSend = true;
        }
    }

    if (registerSend)
        RegisterSend();
}

void Session::ProcessConnect()
{
    SocketUtils::SetUpdateConnectSocket(_socket);     
    SocketUtils::SetTcpNoDelay(_socket, true);

    _connected = true;

    if (auto service = _service.lock())
        service->AddSession(GetRef());

    OnConnected();
    RegisterRecv();
}

void Session::Disconnect()
{
    if (!_connected.exchange(false))
        return;

    OnDisconnected();

    if (auto service = _service.lock())
        service->ReleaseSession(GetRef());

    SocketUtils::Close(_socket);
}

void Session::RegisterConnect(const NetAddress& address)
{
    _connectEvent.Init();
    _connectEvent.SetOwner(shared_from_this());

    if (!SocketUtils::BindAnyAddress(_socket, 0))
    {
        LOG_ERROR(L"RegisterConnect: BindAnyAddress failed");
        return;
    }

    DWORD numBytes = 0;
    SOCKADDR_IN serverAddr = address.GetSockAddr();
    if (!SocketUtils::ConnectEx(_socket,
        reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(serverAddr),
        nullptr, 0, &numBytes, &_connectEvent))
    {
        int32 errCode = ::WSAGetLastError();
        if (errCode != WSA_IO_PENDING)
        {
            LOG_ERROR(L"ConnectEx failed errCode=" + std::to_wstring(errCode));
            _connectEvent.SetOwner(nullptr);
        }
    }
}

void Session::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
    switch (iocpEvent->GetType())
    {
    case IocpEventType::Recv:
        ProcessRecv(numOfBytes);
        break;
    case IocpEventType::Send:
        ProcessSend(numOfBytes);
        break;
    case IocpEventType::Connect:
        _connectEvent.SetOwner(nullptr);
        ProcessConnect();
        break;
    default:
        break;
    }
}

void Session::RegisterRecv()
{
    if (!_connected)
        return;

    _recvEvent.Init();
    _recvEvent.SetOwner(shared_from_this());

    WSABUF wsaBufs[2];
    uint32 bufCount = _recvBuffer.GetWriteSegments(wsaBufs);
    if (bufCount == 0)
    {
        Disconnect();
        return;
    }

    DWORD numOfBytes = 0;
    DWORD flags = 0;

    if (::WSARecv(_socket, wsaBufs, bufCount, &numOfBytes, &flags, &_recvEvent, nullptr) == SOCKET_ERROR)
    {
        int32 errCode = ::WSAGetLastError();
        if (errCode != WSA_IO_PENDING)
        {
            LOG_ERROR(L"WSARecv failed errCode=" + std::to_wstring(errCode));
            _recvEvent.SetOwner(nullptr);
            Disconnect();
        }
    }
}

void Session::ProcessRecv(int32 numOfBytes)
{

    _recvEvent.SetOwner(nullptr);

    if (numOfBytes == 0)
    {
        Disconnect();
        return;
    }

    GServerStats->network.recvBytes.fetch_add(numOfBytes, std::memory_order_relaxed);

    if (!_recvBuffer.OnWrite(numOfBytes))
    {
        LOG_WARN(L"RecvBuffer full addr=" + GetNetAddress().GetIpAddress());
        Disconnect();
        return;
    }

    while (_recvBuffer.GetDataSize() > 0)
    {
        auto segment = _recvBuffer.ReadSegment();
        int32 processLen = OnRecv(segment.data(), static_cast<uint32>(segment.size()));

        if (processLen < 0)
        {
            LOG_WARN(L"Invalid packet addr=" + GetNetAddress().GetIpAddress());
            Disconnect();
            return;
        }

        if (processLen == 0)
        {
            if (segment.size() < _recvBuffer.GetDataSize())
            {
                _recvBuffer.Linearize();
                continue;
            }
            break;
        }

        if (!_recvBuffer.OnRead(static_cast<uint32>(processLen)))
        {
            Disconnect();
            return;
        }
    }
    RegisterRecv();
}

void Session::RegisterSend()
{
    if (!_connected)
        return;

    _sendEvent.Init();
    _sendEvent.SetOwner(shared_from_this());

    {
        LockGuard guard(_sendLock);
        _sendPendingList.swap(_sendQueue);

        if (_sendPendingList.empty())
        {
            _isSendRegistered = false;
            _sendEvent.SetOwner(nullptr);
            return;
        }
    }

    FrameVector<WSABUF> wsaBufs;
    wsaBufs.reserve(_sendPendingList.size());

    for (auto& buffer : _sendPendingList)
    {
        WSABUF wsaBuf;
        wsaBuf.buf = reinterpret_cast<char*>(buffer->GetBuffer());
        wsaBuf.len = static_cast<ULONG>(buffer->GetWriteSize());
        wsaBufs.push_back(wsaBuf);
    }

    DWORD numOfBytes = 0;
    if (::WSASend(_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()),
        &numOfBytes, 0, &_sendEvent, nullptr) == SOCKET_ERROR)
    {
        int32 errCode = ::WSAGetLastError();
        if (errCode != WSA_IO_PENDING)
        {
            LOG_ERROR(L"WSASend failed errCode=" + std::to_wstring(errCode));
            _sendEvent.SetOwner(nullptr);
            Disconnect();
        }
    }
}

void Session::ProcessSend(int32 numOfBytes)
{
    _sendEvent.SetOwner(nullptr);
    _sendPendingList.clear();

    if (numOfBytes == 0)
    {
        Disconnect();
        return;
    }

    GServerStats->network.sendBytes.fetch_add(numOfBytes, std::memory_order_relaxed);
    GServerStats->network.sendPackets.fetch_add(1, std::memory_order_relaxed);

    OnSend(numOfBytes);

    bool registerSend = false;
    {
        LockGuard guard(_sendLock);
        if (_sendQueue.empty())
            _isSendRegistered = false;
        else
            registerSend = true;
    }

    if (registerSend)
        RegisterSend();
}

int32 PacketSession::OnRecv(const BYTE* buffer, uint32 len)
{
    std::span<const BYTE> data{ buffer, len };
    uint32 processLen = 0;

    while (data.size() >= sizeof(PacketHeader))
    {
        PacketHeader header;
        std::memcpy(&header, data.data(), sizeof(PacketHeader));

        if (header.size < sizeof(PacketHeader) || header.size > MAX_PACKET_SIZE) [[unlikely]]
            return -1;

        if (data.size() < header.size) [[unlikely]]
            break;

        OnRecvPacket(data.first(header.size), header.type);
        GServerStats->network.recvPackets.fetch_add(1, std::memory_order_relaxed);

        data = data.subspan(header.size);
        processLen += header.size;
    }

    return static_cast<int32>(processLen);
}

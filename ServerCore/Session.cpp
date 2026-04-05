#include "pch.h"
#include "Session.h"
#include "SocketUtils.h"
#include "Service.h"
#include "GameMetrics.h"

/*----------------
	  Session
-----------------*/

Session::Session()
{
	_socket = SocketUtils::CreateSocket();
}

Session::~Session()
{
	SocketUtils::Close(_socket);
}

void Session::Init()
{
	auto& config = GetService()->GetConfig();
	_recvBuffer = MakeShared<RecvBuffer>(config.recvBufferSize, config.recvBufferCount);
}

void Session::Send(SendBufferRef sendBuffer)
{
	if (IsConnected() == false)  
		return;

	bool registerSend = false;
	{
		WRITE_LOCK;
		_sendQueue.push(sendBuffer);
		registerSend = _sendRegistered.exchange(true) == false;
	}
	
	if (registerSend)
		RegisterSend();
}

bool Session::Connect()
{
	return RegisterConnect();
}

void Session::Disconnect(const WCHAR* cause)
{
	if (_connected.exchange(false) == false) return;
	RegisterDisconnect();
}

HANDLE Session::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Session::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
	switch (iocpEvent->type)
	{
	case EventType::Connect:
		ProcessConnect();
		break;
	case EventType::Disconnect:
		ProcessDisconnect();
		break;
	case EventType::Recv:
		ProcessRecv(numOfBytes);
		break;
	case EventType::Send:
		ProcessSend(iocpEvent, numOfBytes);
		break;
	default:
		break;
	}
}

bool Session::RegisterConnect()
{
	if (IsConnected()) return false;
	if (GetService()->GetServiceType() != ServiceType::Client) return false;
	if (SocketUtils::SetReuseAddress(_socket, true) == false) return false;
	if (SocketUtils::BindAnyAddress(_socket, 0) == false) return false;

	_connectEvent.Init();
	_connectEvent.owner = shared_from_this();

	DWORD numOfBytes = 0;
	SOCKADDR_IN sockAddr = GetService()->GetNetAddress().GetSockAddr();
	if (false == SocketUtils::ConnectEx(_socket, reinterpret_cast<SOCKADDR*>(&sockAddr), sizeof(sockAddr), nullptr, 0, &numOfBytes, &_connectEvent))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_connectEvent.owner = nullptr;
			return false;
		}
	}

	return true;
}

bool Session::RegisterDisconnect()
{
	_disconnectEvent.Init();
	_disconnectEvent.owner = shared_from_this();

	if (false == SocketUtils::DisconnectEx(_socket, &_disconnectEvent, TF_REUSE_SOCKET, 0))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_disconnectEvent.owner = nullptr;
			return false;
		}
	}

	return true;
}

void Session::RegisterRecv()
{
	if (IsConnected() == false) return;

	_recvEvent.Init();
	_recvEvent.owner = shared_from_this();  

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer->WritePos());
	wsaBuf.len = _recvBuffer->FreeSize();

	DWORD numOfBytes = 0;
	DWORD flags = 0;
	if (SOCKET_ERROR == ::WSARecv(_socket, &wsaBuf, 1, OUT &numOfBytes, OUT &flags, &_recvEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_recvEvent.owner = nullptr;  // RELEASE_REF
		}
	}
}

void Session::RegisterSend()
{
	if (IsConnected() == false) return;

	IocpEvent* sendEvent = new IocpEvent(EventType::Send);
	sendEvent->Init();
	sendEvent->owner = shared_from_this();  // ADD_REF

	{
		WRITE_LOCK;

		while (_sendQueue.empty() == false)
		{
			SendBufferRef sendBuffer = _sendQueue.front();
			_sendQueue.pop();
			sendEvent->sendBuffers.push_back(sendBuffer);
		}
	}

	if (sendEvent->sendBuffers.empty())
	{
		sendEvent->owner = nullptr;
		delete sendEvent;
		_sendRegistered.store(false);
		return;
	}

	sendEvent->wsaBufs.reserve(sendEvent->sendBuffers.size());
	for (SendBufferRef sendBuffer : sendEvent->sendBuffers)
	{
		WSABUF wsaBuf;
		wsaBuf.buf = reinterpret_cast<char*>(sendBuffer->Buffer());
		wsaBuf.len = static_cast<LONG>(sendBuffer->WriteSize());
		sendEvent->wsaBufs.push_back(wsaBuf);
	}

	DWORD numOfBytes = 0;
	if (SOCKET_ERROR == ::WSASend(_socket, sendEvent->wsaBufs.data(), static_cast<DWORD>(sendEvent->wsaBufs.size()), OUT & numOfBytes, 0, sendEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			sendEvent->owner = nullptr;  // RELEASE_REF
			sendEvent->sendBuffers.clear();  // RELEASE_REF
			sendEvent->wsaBufs.clear();
			delete sendEvent;
			_sendRegistered.store(false);
		}
	}
}

void Session::ProcessConnect()
{
	_connectEvent.owner = nullptr;  

	_connected.store(true);

	GMetrics.connectedSessions.fetch_add(1);
	GMetrics.totalConnections.fetch_add(1);

	GetService()->AddSession(GetSessionRef());

	OnConnected();

	RegisterRecv();
}

void Session::ProcessDisconnect()
{
	_disconnectEvent.owner = nullptr;

	GMetrics.connectedSessions.fetch_sub(1);
	GMetrics.totalDisconnections.fetch_add(1);

	OnDisconnected();
	GetService()->ReleaseSession(GetSessionRef());

	::setsockopt(_socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, nullptr, 0);
}

void Session::ProcessRecv(int32 numOfBytes)
{
	_recvEvent.owner = nullptr;  

	if (numOfBytes == 0)
	{
		GMetrics.disconnectRecv0.fetch_add(1);

		Disconnect(L"Recv 0");
		return;
	}

	GMetrics.totalPacketsReceived.fetch_add(1);
	GMetrics.totalBytesReceived.fetch_add(numOfBytes);

	if (_recvBuffer->OnWrite(numOfBytes) == false)
	{
		GMetrics.disconnectRecvOverflow.fetch_add(1);

		Disconnect(L"OnWrite Overflow");
		return;
	}

	int32 dataSize = _recvBuffer->DataSize();
	int32 processLen = OnRecv(_recvBuffer->ReadPos(), dataSize); 

	if (processLen < 0 || dataSize < processLen || _recvBuffer->OnRead(processLen) == false)
	{
		GMetrics.invalidPackets.fetch_add(1);

		Disconnect(L"OnRead Overflow");
		return;
	}

	_recvBuffer->Clean();

	RegisterRecv();
}

void Session::ProcessSend(IocpEvent* sendEvent, int32 numOfBytes)
{
	sendEvent->owner = nullptr;  // RELEASE_REF
	sendEvent->sendBuffers.clear();  // RELEASE_REF
	sendEvent->wsaBufs.clear();
	delete sendEvent;

	if (numOfBytes == 0)
	{
		GMetrics.disconnectSend0.fetch_add(1);

		Disconnect(L"Send 0");
		return;
	}

	GMetrics.totalPacketsSent.fetch_add(1);
	GMetrics.totalBytesSent.fetch_add(numOfBytes);

	OnSend(numOfBytes);

	bool registerSend = false;

	{
		WRITE_LOCK;
		if (_sendQueue.empty())
			_sendRegistered.store(false);
		else
			registerSend = true;
	}

	if (registerSend)
		RegisterSend();
}

void Session::HandleError(int32 errorCode)
{
	switch (errorCode)
	{
	case WSAECONNRESET:  
	case WSAECONNABORTED:  
		GMetrics.disconnectHandleError.fetch_add(1);
		Disconnect(L"HandleError");
		break;
	default:
		// 기타 에러
		cout << "Handle Error : " << errorCode << endl;
		break;
	}
}

/*-------------------
	PacketSession
--------------------*/

PacketSession::PacketSession()
{
}

PacketSession::~PacketSession()
{
}

int32 PacketSession::OnRecv(BYTE* buffer, int32 len)
{
	int processLen = 0;

	while (true)
	{
		int32 dataSize = len - processLen;
		if (dataSize < sizeof(PacketHeader))
			break;

		PacketHeader* header = reinterpret_cast<PacketHeader*>(&buffer[processLen]);

		if (header->size < sizeof(PacketHeader) || header->size > GetService()->GetConfig().recvBufferSize)
			return -1;

		if (dataSize < header->size)
			break;

		OnRecvPacket(&buffer[processLen], header->size);

		processLen += header->size;
	}

	return processLen;
}

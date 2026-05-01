#include "pch.h"
#include "Session.h"
#include "SocketUtils.h"
#include "Service.h"
#include "GameMetrics.h"
#include "ServerStats.h"

/*----------------
	  Session
-----------------*/

Session::Session()
	: _recvBuffer(Config::Session::RECV_BUFFER_SIZE)  // 순환 버퍼 값 초기화
{
	_socket = SocketUtils::CreateSocket();
}

Session::~Session()
{
	SocketUtils::Close(_socket);
}

void Session::Send(SendBufferRef sendBuffer)
{
	if (IsConnected() == false)
		return;

	bool registerSend = false;
	{
		WRITE_LOCK;
		_sendQueue.push_back(sendBuffer);
		// exchange: false → true 이면 최초 등록자 → RegisterSend 호출 권한 획득
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
	// 이미 끊긴 세션의 중복 호출 방지
	if (_connected.exchange(false) == false) return;

	// DisconnectEx 를 통해 재사용 가능 소켓으로 정리
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
		ProcessSend(numOfBytes);
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
	if (false == SocketUtils::ConnectEx(_socket,
		reinterpret_cast<SOCKADDR*>(&sockAddr), sizeof(sockAddr),
		nullptr, 0, &numOfBytes, &_connectEvent))
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

	// 순환 버퍼: wrap-around 시 최대 2개 세그먼트로 분산 수신 (memmove 제거)
	WSABUF wsaBufs[2];
	int32 segCount = _recvBuffer.GetWriteSegments(wsaBufs);
	if (segCount == 0)
	{
		// 수신 버퍼 가득 참 → 프로토콜 이상 (패킷 처리가 따라오지 못하는 상황)
		ServerStats::Get().recvBuffer.bufferFullCount.fetch_add(1, std::memory_order_relaxed);
		Disconnect(L"RecvBuffer Full");
		return;
	}

	DWORD numOfBytes = 0;
	DWORD flags = 0;
	if (SOCKET_ERROR == ::WSARecv(_socket, wsaBufs, segCount,
		OUT &numOfBytes, OUT &flags, &_recvEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_recvEvent.owner = nullptr;
		}
	}
}

void Session::RegisterSend()
{
	if (IsConnected() == false) return;

	// ── Swap 패턴: 락 보유 시간을 O(1)로 최소화 ──────────────
	{
		WRITE_LOCK;
		// _sendQueue(생산자 큐) ↔ _sendPendingList(전송 배치) 를 O(1) 교체
		_sendQueue.swap(_sendPendingList);
	}

	if (_sendPendingList.empty())
	{
		// 교체했더니 실제 데이터 없음 → 등록 해제
		_sendRegistered.store(false);
		return;
	}

	// ── SendEvent 재사용: 힙 할당 없음 ───────────────────────
	_sendEvent.Init();
	_sendEvent.owner = shared_from_this();

	_sendEvent.wsaBufs.clear();
	_sendEvent.wsaBufs.reserve(_sendPendingList.size());
	for (const SendBufferRef& sb : _sendPendingList)
	{
		WSABUF wsaBuf;
		wsaBuf.buf = reinterpret_cast<char*>(sb->Buffer());
		wsaBuf.len = static_cast<ULONG>(sb->WriteSize());
		_sendEvent.wsaBufs.push_back(wsaBuf);
	}

	// sendBuffers 는 전송 완료 시까지 ref-count 유지 (ProcessSend 에서 clear)
	_sendEvent.sendBuffers = _sendPendingList;

	DWORD numOfBytes = 0;
	if (SOCKET_ERROR == ::WSASend(_socket,
		_sendEvent.wsaBufs.data(),
		static_cast<DWORD>(_sendEvent.wsaBufs.size()),
		OUT &numOfBytes, 0, &_sendEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_sendEvent.owner = nullptr;
			_sendEvent.sendBuffers.clear();
			_sendEvent.wsaBufs.clear();
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

	// 쓰기 커서 전진
	if (_recvBuffer.OnWrite(numOfBytes) == false)
	{
		GMetrics.disconnectRecvOverflow.fetch_add(1);
		Disconnect(L"OnWrite Overflow");
		return;
	}

	// Linearize(): wrap 없으면 O(1) 포인터 반환, wrap 있으면 임시 복사
	int32 dataSize = _recvBuffer.DataSize();
	BYTE* data = _recvBuffer.Linearize();
	int32 processLen = OnRecv(data, dataSize);

	if (processLen < 0 || dataSize < processLen || _recvBuffer.OnRead(processLen) == false)
	{
		GMetrics.invalidPackets.fetch_add(1);
		Disconnect(L"OnRead Overflow");
		return;
	}

	RegisterRecv();
}

void Session::ProcessSend(int32 numOfBytes)
{
	// 멤버 이벤트 정리 (힙 delete 없음)
	_sendEvent.owner = nullptr;
	_sendEvent.sendBuffers.clear();   // SendBuffer ref 해제
	_sendEvent.wsaBufs.clear();
	_sendPendingList.clear();

	if (numOfBytes == 0)
	{
		GMetrics.disconnectSend0.fetch_add(1);
		Disconnect(L"Send 0");
		return;
	}

	GMetrics.totalPacketsSent.fetch_add(1);
	GMetrics.totalBytesSent.fetch_add(numOfBytes);

	OnSend(numOfBytes);

	// _sendQueue 에 추가된 항목이 있으면 연속 전송
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

		if (header->size < sizeof(PacketHeader) ||
			header->size > GetService()->GetConfig().recvBufferSize)
			return -1;

		if (dataSize < header->size)
			break;

		OnRecvPacket(&buffer[processLen], header->size);

		processLen += header->size;
	}

	return processLen;
}

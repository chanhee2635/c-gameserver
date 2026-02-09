#include "pch.h"
#include "Session.h"
#include "SocketUtils.h"
#include "Service.h"

/*----------------
	  Session
-----------------*/

Session::Session()
{
	_socket = SocketUtils::CreateSocket();
}

/*
* 자원 해제 필수 처리 ★
*/
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
		// 전송할 버퍼를 큐에 추가
		_sendQueue.push(sendBuffer);
		// 전송 예약이 되어 있는지 확인
		// true 로 값을 바꾸면서, 이전 값이 false 라면 true 반환
		registerSend = _sendRegistered.exchange(true) == false;
	}
	
	// 전송 예약이 되어 있지 않았다면 전송 예약 요청
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

	wcout << "Disconnect : " << cause << endl;

	RegisterDisconnect();
}

HANDLE Session::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Session::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
	// 비동기 I/O 작업 완료 통지를 받아 해당 로직으로 분기
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

	// 커널이 데이터 채워줄 버퍼 설정
	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer->WritePos());
	wsaBuf.len = _recvBuffer->FreeSize();

	DWORD numOfBytes = 0;
	DWORD flags = 0;
	// OVERLAPPED를 상속받은 _recvEvent를 넘겨 완료 시점에 이 주소를 돌려받음
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

	_sendEvent.Init();
	_sendEvent.owner = shared_from_this();  // ADD_REF

	// sendQueue에 쌓인 SendBuffer들을 sendEvent에 이동
	{
		WRITE_LOCK;

		int32 writeSize = 0;
		while (_sendQueue.empty() == false)
		{
			SendBufferRef sendBuffer = _sendQueue.front();

			// TODO: 너무 많이 쌓이면 다음에 전송 (최대치 적용)
			writeSize += sendBuffer->WriteSize();

			_sendQueue.pop();
			_sendEvent.sendBuffers.push_back(sendBuffer);
		}
	}

	// Scatter-Gather (흩어져 있는 데이터들을 모아서 한 번에 보냄)
	vector<WSABUF> wsaBufs;
	wsaBufs.reserve(_sendEvent.sendBuffers.size());
	for (SendBufferRef sendBuffer : _sendEvent.sendBuffers)
	{
		WSABUF wsaBuf;
		wsaBuf.buf = reinterpret_cast<char*>(sendBuffer->Buffer());
		wsaBuf.len = static_cast<LONG>(sendBuffer->WriteSize());
		wsaBufs.push_back(wsaBuf);
	}

	// 비동기 전송 호출
	DWORD numOfBytes = 0;
	if (SOCKET_ERROR == ::WSASend(_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), OUT & numOfBytes, 0, &_sendEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_sendEvent.owner = nullptr;  // RELEASE_REF
			_sendEvent.sendBuffers.clear();  // RELEASE_REF
			_sendRegistered.store(false);
		}
	}
}

void Session::ProcessConnect()
{
	// 순환 참조 해제 (Client-Side의 RegisterConnect)
	_connectEvent.owner = nullptr;  

	// 연결 상태로 원자적 변경
	_connected.store(true);

	// 서비스에서 관리하는 세션 리스트에 추가
	GetService()->AddSession(GetSessionRef());

	// 컨텐츠 코드에서 정의한 로직 실행 
	OnConnected();

	// 최초 수신 예약
	RegisterRecv();
}

void Session::ProcessDisconnect()
{
	_disconnectEvent.owner = nullptr;

	OnDisconnected();
	GetService()->ReleaseSession(GetSessionRef());
}

void Session::ProcessRecv(int32 numOfBytes)
{
	// 순환 참조 해제 (RegisterRecv)
	_recvEvent.owner = nullptr;  

	// 상대방이 접속을 끊었을 때
	if (numOfBytes == 0)
	{
		Disconnect(L"Recv 0");
		return;
	}

	// 커널이 쓴 만큼 쓰기 커서 이동
	if (_recvBuffer->OnWrite(numOfBytes) == false)
	{
		Disconnect(L"OnWrite Overflow");
		return;
	}

	// 컨텐츠 레이어로 데이터 전달
	int32 dataSize = _recvBuffer->DataSize();
	int32 processLen = OnRecv(_recvBuffer->ReadPos(), dataSize); 

	// 처리한 결과 검증
	if (processLen < 0 || dataSize < processLen || _recvBuffer->OnRead(processLen) == false)
	{
		Disconnect(L"OnRead Overflow");
		return;
	}

	// 버퍼 정리
	_recvBuffer->Clean();

	// 수신 재등록
	RegisterRecv();
}

void Session::ProcessSend(int32 numOfBytes)
{
	_sendEvent.owner = nullptr;  // RELEASE_REF
	_sendEvent.sendBuffers.clear();  // RELEASE_REF

	// 상대방이 접속을 끊었을 때
	if (numOfBytes == 0)
	{
		Disconnect(L"Send 0");
		return;
	}

	// 컨텐츠 코드에서 재정의
	OnSend(numOfBytes);

	bool registerSend = false;

	{
		WRITE_LOCK;
		// 큐가 비어있다면 플래그를 false로 변경
		if (_sendQueue.empty())
			_sendRegistered.store(false);
		else
			registerSend = true;
	}

	// 전송하는 동안 새로운 데이터가 쌓였다면 재전송
	if (registerSend)
		RegisterSend();
}

void Session::HandleError(int32 errorCode)
{
	switch (errorCode)
	{
	case WSAECONNRESET:  // 상대방에 의한 강제 종료
	case WSAECONNABORTED:  // 시스템 혹은 타임아웃에 의해 연결 중단
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
		// 버퍼의 처리되지 않은 데이터가 헤더보다 적으면 다음 수신을 기다림
		int32 dataSize = len - processLen;
		if (dataSize < sizeof(PacketHeader))
			break;

		// 헤더 파싱 (포인터 캐스팅으로 성능 최적화)
		PacketHeader* header = reinterpret_cast<PacketHeader*>(&buffer[processLen]);

		// 보안 및 유효성 검사 (패킷 크기가 너무 작거나 최대 크기를 초과)
		if (header->size < sizeof(PacketHeader) || header->size > GetService()->GetConfig().recvBufferSize)
			return -1;

		// 헤더 크기만큼 데이터가 도착하지 않았다면 다음 수신을 기다림
		if (dataSize < header->size)
			break;

		// 콘텐츠단에서 패킷 처리
		OnRecvPacket(&buffer[processLen], header->size);

		processLen += header->size;
	}

	return processLen;
}

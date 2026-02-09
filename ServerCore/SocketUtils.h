#pragma once
#include "NetAddress.h"

/*----------------
	SocketUtils
-----------------*/

class SocketUtils
{
public:
	static LPFN_CONNECTEX		ConnectEx;
	static LPFN_DISCONNECTEX	DisconnectEx;
	static LPFN_ACCEPTEX		AcceptEx;

public:
	/*
	* @brief 윈도우 소켓 초기화 및 확장 함수 포인터 로드
	*/
	static void Init();

	static void Clear();

	static bool BindWindowsFunction(SOCKET socket, GUID guid, LPVOID* fn);

	/*
	* @brief 비동기 입출력 TCP 소켓 생성
	*/
	static SOCKET CreateSocket();

	/*
	* @brief 서버 종료 시 소켓 버퍼에 남은 데이터를 상대방에게 전송할지, 아니면 즉시 연결을 끊고 리소스를 반환할지 결정
	* @param onoff 0이면 즉시 종료, 1이면 잔여 데이터 전송 대기
	* @param linger 대기할 시간(초)
	*/
	static bool SetLinger(SOCKET socket, uint16 onoff, uint16 linger);

	/*
	* @brief 서버 재시작 시 포트가 TIME_WAIT 상태일지라도 즉시 재사용하기 위한 옵션
	* @param flag true일 경우 재사용을 허용
	*/
	static bool SetReuseAddress(SOCKET socket, bool flag);

	/*
	* @brief 소켓의 수신 버퍼 크기를 설정 
	* @param size 설정할 버퍼 크기(byte)
	* @details 커널이 관리하는 TCP 수신 버퍼 크기를 조정하여 대량의 패킷 수신 시 데이터 유실(Drop)을 방지하고 시스템 콜 오버헤드를 줄인다.
	*/
	static bool SetRecvBufferSize(SOCKET socket, int32 size);

	/*
	* @brief 소켓의 송신 버퍼 크기를 설정
	* @param size 설정할 버퍼 크기(byte)
	* @details 송신 버퍼가 가득 차서 발생하는 블로킹이나 성능 저하를 방지한다. 서비스 특성(MMORPG의 브로드캐스팅 등)에 따라 적절한 크기로 설정한다.
	*/
	static bool SetSendBufferSize(SOCKET socket, int32 size);

	/*
	* @brief 네이글 알고리즘 작동 여부 (전송 Latency 최소화)
	* 장점 : 작은 패킷이 불필요하게 많이 생성되는 일을 방지
	* 단점 : 반응 시간 손해
	* @param flag true일 경우 Nagle을 끄고, 데이터를 즉시 전송
	*/
	static bool SetTcpNoDelay(SOCKET socket, bool flag);

	/*
	* @brief AcceptEx로 접속된 소켓에 리슨 소켓의 속성을 복사
	* @param socket 새로 접속한 클라이언트 소켓
	* @param listenSocket 접속을 받아들인 Listen 소켓
	*/
	static bool SetUpdateAcceptSocket(SOCKET socket, SOCKET listenSocket);

	/*
	* @brief 소켓에 IP 주소와 Port를 Bind
	*/
	static bool Bind(SOCKET socket, NetAddress netAddr);

	/*
	* @brief 소켓에 SOCKADDR_IN 구조체를 사용하여 Bind
	*/
	static bool Bind(SOCKET socket, SOCKADDR_IN sockAddr);

	/*
	* @brief 특정 포트에 대해 모든 네트워크 인터페이스를 대상으로 Bind
	*/
	static bool BindAnyAddress(SOCKET socket, uint16 port);

	/*
	* @brief 소켓을 수신 대기 상태로 변경
	* @param socket 바인딩이 완료된 Listen 소켓
	* @param backlog 연결 요청 대기 큐의 크기 (기본 최대치)
	*/
	static bool Listen(SOCKET socket, int32 backlog = SOMAXCONN);

	/*
	* @brief 소켓 자원 해제 및 핸들 초기화
	* param socket 해제할 소켓 핸들 참조
	*/
	static void Close(SOCKET& socket);
};

template<typename T>
static inline bool SetSockOpt(SOCKET socket, int32 level, int32 optName, T optVal)
{
	return SOCKET_ERROR != ::setsockopt(socket, level, optName, reinterpret_cast<char*>(&optVal), sizeof(T));
}
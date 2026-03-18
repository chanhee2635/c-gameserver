#pragma once
#include "Login.pb.h"

using PacketHandlerFunc = std::function<bool(PacketSessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);
bool C_LoginAuthHandler(PacketSessionRef& session, Protocol::C_LoginAuth&pkt);
bool C_JoinHandler(PacketSessionRef& session, Protocol::C_Join&pkt);

class ClientPacketHandler
{
public:
	/*
	* @brief 패킷 ID별 처리 함수(Handler) 초기화
	* @details 배열 인덱스 접근 O(1)을 통해 패킷 라우팅 성능 극대화
	*/
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		GPacketHandler[Protocol::MsgId::C_LOGIN_AUTH] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_LoginAuth>(C_LoginAuthHandler, session, buffer, len); };
		GPacketHandler[Protocol::MsgId::C_JOIN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_Join>(C_JoinHandler, session, buffer, len); };
	}

	/*
	* @brief 수신된 패킷의 ID를 확인하여 등록된 처리 함수(Handler) 호출
	* @param session 패킷을 송신한 세션 객체
	* @param buffer	패킷 헤더를 포함한 전체 데이터 바이트 배열
	* @param len 패킷의 전체 길이
	* @return 미등록 패킷이나 패킷 처리 오류 시 false
	*/
	static bool HandlePacket(PacketSessionRef & session, BYTE * buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		if (header->id < 0 || header->id >= UINT16_MAX) return false;

		return GPacketHandler[header->id](session, buffer, len);
	}
	static SendBufferRef MakeSendBuffer(Protocol::S_LoginAuth& pkt) { return MakeSendBuffer(pkt, Protocol::MsgId::S_LOGIN_AUTH); }
	static SendBufferRef MakeSendBuffer(Protocol::S_Join& pkt) { return MakeSendBuffer(pkt, Protocol::MsgId::S_JOIN); }

private:

	/*
	* @brief 수신된 바이트 버퍼를 특정 패킷 타입으로 변환 후 Handler 함수 호출
	* @tparam PacketType Protobuf 메시지 클래스 타입
	* @tparam ProcessFunc 실행할 Handler 함수 타입
	* @param func 실제 로직을 수행할 Handler 함수
	* @param session 패킷을 보낸 세션 객체
	* @param buffer	패킷 헤더를 포함한 전체 데이터 바이트 배열
	* @param len 패킷의 전체 길이
	*/
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, PacketSessionRef & session, BYTE * buffer, int32 len)
	{
		PacketType pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
			return false;

		return func(session, pkt);
	}

	/*
	* @brief Protobuf 패킷을 직렬화하여 전송용 SendBuffer를 생성
	* @tparam T Protobuf 메시지 클래스 타입
	* @param pkt 직렬화할 패킷 객체
	* @param pktId 패킷 고유 ID
	* @return 생성된 SendBuffer의 참조
	*/
	template<typename T>
	static SendBufferRef MakeSendBuffer(T & pkt, uint16 pktId)
	{
		const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

		SendBufferRef sendBuffer = GSendBufferManager->Open(packetSize);

		PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
		header->size = packetSize;
		header->id = pktId;
		pkt.SerializeToArray(&header[1], dataSize);

		sendBuffer->Close(packetSize);

		return sendBuffer;
	}
};

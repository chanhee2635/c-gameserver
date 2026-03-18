#pragma once
#include "Chat.pb.h"

using PacketHandlerFunc = std::function<bool(PacketSessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);
bool C_ChatLoginHandler(PacketSessionRef& session, Protocol::C_ChatLogin&pkt);
bool C_ChatHandler(PacketSessionRef& session, Protocol::C_Chat&pkt);

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
		GPacketHandler[Protocol::MsgId::C_CHAT_LOGIN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_ChatLogin>(C_ChatLoginHandler, session, buffer, len); };
		GPacketHandler[Protocol::MsgId::C_CHAT] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_Chat>(C_ChatHandler, session, buffer, len); };
	}

	/*
	* @brief Protobuf 패킷을 직렬화하여 전송용 SendBuffer를 생성
	* @tparam T Protobuf 메시지 클래스 타입
	* @param pkt 직렬화할 패킷 객체
	* @param pktId 패킷 고유 ID
	* @return 생성된 SendBuffer의 참조
	*/
	static bool HandlePacket(PacketSessionRef & session, BYTE * buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		if (header->id < 0 || header->id >= UINT16_MAX) return false;

		return GPacketHandler[header->id](session, buffer, len);
	}
	static SendBufferRef MakeSendBuffer(Protocol::S_ChatLogin& pkt) { return MakeSendBuffer(pkt, Protocol::MsgId::S_CHAT_LOGIN); }
	static SendBufferRef MakeSendBuffer(Protocol::S_Chat& pkt) { return MakeSendBuffer(pkt, Protocol::MsgId::S_CHAT); }

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

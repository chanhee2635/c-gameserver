#pragma once
#include "Client.pb.h"

using PacketHandlerFunc = std::function<bool(PacketSessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);
bool S_LoginAuthHandler(PacketSessionRef& session, Protocol::S_LoginAuth&pkt);
bool S_JoinHandler(PacketSessionRef& session, Protocol::S_Join&pkt);
bool S_PlayerListHandler(PacketSessionRef& session, Protocol::S_PlayerList&pkt);
bool S_CreatePlayerHandler(PacketSessionRef& session, Protocol::S_CreatePlayer&pkt);
bool S_EnterGameHandler(PacketSessionRef& session, Protocol::S_EnterGame&pkt);
bool S_AttackHandler(PacketSessionRef& session, Protocol::S_Attack&pkt);
bool S_DieHandler(PacketSessionRef& session, Protocol::S_Die&pkt);
bool S_ChangeHpHandler(PacketSessionRef& session, Protocol::S_ChangeHp&pkt);
bool S_ChangeExpHandler(PacketSessionRef& session, Protocol::S_ChangeExp&pkt);
bool S_ChangeLevelHandler(PacketSessionRef& session, Protocol::S_ChangeLevel&pkt);
bool S_UpdateSceneHandler(PacketSessionRef& session, Protocol::S_UpdateScene&pkt);
bool S_ReviveHandler(PacketSessionRef& session, Protocol::S_Revive&pkt);
bool S_ChatLoginHandler(PacketSessionRef& session, Protocol::S_ChatLogin&pkt);
bool S_ChatHandler(PacketSessionRef& session, Protocol::S_Chat&pkt);

class ServerPacketHandler
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
		GPacketHandler[Protocol::MsgId::S_LOGIN_AUTH] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_LoginAuth>(S_LoginAuthHandler, session, buffer, len); };
		GPacketHandler[Protocol::MsgId::S_JOIN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_Join>(S_JoinHandler, session, buffer, len); };
		GPacketHandler[Protocol::MsgId::S_PLAYER_LIST] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_PlayerList>(S_PlayerListHandler, session, buffer, len); };
		GPacketHandler[Protocol::MsgId::S_CREATE_PLAYER] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CreatePlayer>(S_CreatePlayerHandler, session, buffer, len); };
		GPacketHandler[Protocol::MsgId::S_ENTER_GAME] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_EnterGame>(S_EnterGameHandler, session, buffer, len); };
		GPacketHandler[Protocol::MsgId::S_ATTACK] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_Attack>(S_AttackHandler, session, buffer, len); };
		GPacketHandler[Protocol::MsgId::S_DIE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_Die>(S_DieHandler, session, buffer, len); };
		GPacketHandler[Protocol::MsgId::S_CHANGE_HP] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_ChangeHp>(S_ChangeHpHandler, session, buffer, len); };
		GPacketHandler[Protocol::MsgId::S_CHANGE_EXP] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_ChangeExp>(S_ChangeExpHandler, session, buffer, len); };
		GPacketHandler[Protocol::MsgId::S_CHANGE_LEVEL] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_ChangeLevel>(S_ChangeLevelHandler, session, buffer, len); };
		GPacketHandler[Protocol::MsgId::S_UPDATE_SCENE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_UpdateScene>(S_UpdateSceneHandler, session, buffer, len); };
		GPacketHandler[Protocol::MsgId::S_REVIVE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_Revive>(S_ReviveHandler, session, buffer, len); };
		GPacketHandler[Protocol::MsgId::S_CHAT_LOGIN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_ChatLogin>(S_ChatLoginHandler, session, buffer, len); };
		GPacketHandler[Protocol::MsgId::S_CHAT] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_Chat>(S_ChatHandler, session, buffer, len); };
	}

	/*
	* @brief 수신된 패킷의 ID를 확인하여 등록된 처리 함수(Handler) 호출
	* @param session 패킷을 보낸 세션 객체
	* @param buffer	패킷 헤더를 포함한 전체 데이터 바이트 배열
	* @param len 패킷의 전체 길이
	* @return 미등록 패킷이나 패킷 처리 오류 시 false
	*/
	static bool HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		if (header->id < 0 || header->id >= UINT16_MAX) return false;

		return GPacketHandler[header->id](session, buffer, len);
	}
	static SendBufferRef MakeSendBuffer(Protocol::C_LoginAuth& pkt) { return MakeSendBuffer(pkt, Protocol::MsgId::C_LOGIN_AUTH); }
	static SendBufferRef MakeSendBuffer(Protocol::C_Join& pkt) { return MakeSendBuffer(pkt, Protocol::MsgId::C_JOIN); }
	static SendBufferRef MakeSendBuffer(Protocol::C_AuthToken& pkt) { return MakeSendBuffer(pkt, Protocol::MsgId::C_AUTH_TOKEN); }
	static SendBufferRef MakeSendBuffer(Protocol::C_CreatePlayer& pkt) { return MakeSendBuffer(pkt, Protocol::MsgId::C_CREATE_PLAYER); }
	static SendBufferRef MakeSendBuffer(Protocol::C_EnterGame& pkt) { return MakeSendBuffer(pkt, Protocol::MsgId::C_ENTER_GAME); }
	static SendBufferRef MakeSendBuffer(Protocol::C_LoadCompleted& pkt) { return MakeSendBuffer(pkt, Protocol::MsgId::C_LOAD_COMPLETED); }
	static SendBufferRef MakeSendBuffer(Protocol::C_Move& pkt) { return MakeSendBuffer(pkt, Protocol::MsgId::C_MOVE); }
	static SendBufferRef MakeSendBuffer(Protocol::C_Attack& pkt) { return MakeSendBuffer(pkt, Protocol::MsgId::C_ATTACK); }
	static SendBufferRef MakeSendBuffer(Protocol::C_Quit& pkt) { return MakeSendBuffer(pkt, Protocol::MsgId::C_QUIT); }
	static SendBufferRef MakeSendBuffer(Protocol::C_Revive& pkt) { return MakeSendBuffer(pkt, Protocol::MsgId::C_REVIVE); }
	static SendBufferRef MakeSendBuffer(Protocol::C_ChatLogin& pkt) { return MakeSendBuffer(pkt, Protocol::MsgId::C_CHAT_LOGIN); }
	static SendBufferRef MakeSendBuffer(Protocol::C_Chat& pkt) { return MakeSendBuffer(pkt, Protocol::MsgId::C_CHAT); }

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
	static bool HandlePacket(ProcessFunc func, PacketSessionRef& session, BYTE* buffer, int32 len)
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
	static SendBufferRef MakeSendBuffer(T& pkt, uint16 pktId)
	{
		// 패킷 데이터 크기
		const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
		// 전체 패킷 크기 (데이터 크기 + 헤더 크기)
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

		// 전송용 버퍼 풀에서 메모리 공간 할당
		SendBufferRef sendBuffer = GSendBufferManager->Open(packetSize);

		// 헤더 정보 기록
		PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
		header->size = packetSize;
		header->id = pktId;

		// 헤더 뒷부분부터 패킷 데이터를 직렬화하여 복사
		pkt.SerializeToArray(&header[1], dataSize);

		sendBuffer->Close(packetSize);

		return sendBuffer;
	}
};

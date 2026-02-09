#include "pch.h"
#include "LoginSession.h"
#include "ClientPacketHandler.h"
#include "DBTransaction.h"
#include "RedisManager.h"

void LoginSession::OnConnected()
{
	// 동접이 많아지면 ObjectPool 의 MakeShared 사용 고려
	_logicQueue = make_shared<JobQueue>();
	_dbQueue = make_shared<DBJobQueue>();
}

void LoginSession::OnDisconnected()
{
}

void LoginSession::HandleLoginStart(const string& id, const string& pw)
{
	// 유효성 검사
	if (id.size() < 2 || id.size() > 10 || pw.size() < 2)
	{
		Protocol::S_LoginAuth packet;
		packet.set_success(false);
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);
		Send(sendBuffer);
		return;
	}

	// RefCount를 늘려 생명주기 관리
	auto self = static_pointer_cast<LoginSession>(shared_from_this());

	// 스레드 전환 (Logic Thread -> DB Thread)
	_dbQueue->DoAsyncPush([self, id, pw]() {
		GDBTransaction->HandleLoginAuth(self, id, pw);
	});
}

void LoginSession::HandleLoginEnd(bool success, uint64 dbId)
{
	Protocol::S_LoginAuth packet;
	packet.set_success(success);

	if (success)
	{
		// 인증 토큰 생성
		std::string token = GenerateUUID();

		// Redis에 식별자 저장
		GRedisManager->Set(token, dbId);

		packet.set_auth_token(token);

		// Redis에서 활성화되어 있는 게임 서버 리스트 추출
		Vector<std::string> servers = GRedisManager->GetActiveServers();

		for (const std::string& info : servers)
		{
			// IP|Port|UserCount|Status
			Vector<std::string> parts = Utils::Split(info, '|');
			if (parts.size() < 4) continue;

			auto* server = packet.add_server_list();
			server->set_serverid(stoi(parts[0]));
			server->set_servername(parts[1]);
			server->set_ip(parts[2]);
			server->set_port(stoi(parts[3]));
			server->set_sessioncount(stoi(parts[4]));
			server->set_maxcount(stoi(parts[5]));
		}
	}

	// 패킷 직렬화 및 전송
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);
	Send(sendBuffer);
}

void LoginSession::HandleJoinStart(const string& id, const string& pw)
{
	// 유효성 검사
	if (id.size() < 2 || id.size() > 10 || pw.size() < 2)
	{
		Protocol::S_Join packet;
		packet.set_success(false);
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);
		Send(sendBuffer);
		return;
	}

	// RefCount를 늘려 생명주기 관리
	auto self = static_pointer_cast<LoginSession>(shared_from_this());

	// 스레드 전환 (Logic Thread -> DB Thread)
	_dbQueue->DoAsyncPush([self, id, pw]() {
		GDBTransaction->HandleJoin(self, id, pw);
	});
}

void LoginSession::HandleJoinEnd(bool success)
{
	// 성공 여부 패킷 전송
	Protocol::S_Join packet;
	packet.set_success(success);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(packet);
	Send(sendBuffer);
}

string LoginSession::GenerateUUID()
{
	char buf[64];
	GUID guid;
	if (::CoCreateGuid(&guid) == S_OK)
	{
		sprintf_s(buf, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
			guid.Data1, guid.Data2, guid.Data3,
			guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
			guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
	}

	return string(buf);
}

void LoginSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	if (session == nullptr) return;

	ClientPacketHandler::HandlePacket(session, buffer, len);
}

void LoginSession::OnSend(int32 len)
{
}


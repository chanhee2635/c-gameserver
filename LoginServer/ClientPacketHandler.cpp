#include "pch.h"
#include "ClientPacketHandler.h"
#include "DBTransaction.h"
#include "RedisManager.h"
#include "LoginSession.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	// TODO : log
	return false;
}

bool C_LoginAuthHandler(PacketSessionRef& session, Protocol::C_LoginAuth& pkt)
{
	LoginSessionRef loginSession = static_pointer_cast<LoginSession>(session);

	// IOCP Thread -> Logic Thread
	loginSession->GetLogicQueue()->DoAsyncPush([loginSession, id = pkt.id(), pw = pkt.pw()]() {
		loginSession->HandleLoginStart(id, pw);
	});

	return true;
}

bool C_JoinHandler(PacketSessionRef& session, Protocol::C_Join& pkt)
{
	LoginSessionRef loginSession = static_pointer_cast<LoginSession>(session);
	
	// IOCP Thread -> Logic Thread
	loginSession->GetLogicQueue()->DoAsyncPush([loginSession, id = pkt.id(), pw = pkt.pw()]() {
		loginSession->HandleJoinStart(id, pw);
	});

	return true;
}
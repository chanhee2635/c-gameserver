#include "pch.h"
#include "DBTransaction.h"
#include "DBBind.h"
#include "DBConnectionPool.h"
#include "ClientPacketHandler.h"
#include "LoginSession.h"

unique_ptr<DBTransaction> GDBTransaction = make_unique<DBTransaction>();

void DBTransaction::HandleJoin(LoginSessionRef session, string id, string pw)
{
	int32 resultCode = -1;

	DBConnectionRef dbConn = LDBConnection;
	if (dbConn == nullptr) return;

	DBBind<2, 1> dbBind(*dbConn, L"CALL sp_create_account(?, ?);");

	// TODO: 해싱
	wstring wId = Utils::s2ws(id);
	wstring wPw = Utils::s2ws(pw);

	dbBind.BindParam(0, wId.c_str());
	dbBind.BindParam(1, wPw.c_str());
	dbBind.BindCol(0, OUT resultCode);

	if (dbBind.Execute())
		dbConn->Fetch();

	// 스레드 전환 (DB Thread -> Logic Thread)
	session->GetLogicQueue()->DoAsyncPush([session, success = resultCode == 0]() {
		session->HandleJoinEnd(success);
	});
}

void DBTransaction::HandleLoginAuth(LoginSessionRef session, string id, string pw)
{
	bool success = false;
	uint64 dbId = 0;

	// 스레드 전용 DB Connection을 가져옴 (Lock-Free)
	DBConnectionRef dbConn = LDBConnection;
	if (dbConn == nullptr) return;

	DBBind<2, 1> dbBind(*dbConn, L"SELECT account_id FROM Accounts WHERE login_id = (?) AND password = (?)");

	// TODO: 해싱
	// string -> wstring
	wstring wId = Utils::s2ws(id);
	wstring wPw = Utils::s2ws(pw);

	dbBind.BindParam(0, wId.c_str());
	dbBind.BindParam(1, wPw.c_str());
	dbBind.BindCol(0, OUT dbId);

	if (dbBind.Execute())
	{
		if (dbConn->Fetch())
			success = true;
	}

	// 스레드 전환 (DB Thread -> Logic Thread)
	session->GetLogicQueue()->DoAsyncPush([session, success, dbId]() {
		session->HandleLoginEnd(success, dbId);
	});
}
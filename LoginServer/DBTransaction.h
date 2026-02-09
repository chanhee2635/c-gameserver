#pragma once

class DBTransaction
{
public:
	void HandleJoin(LoginSessionRef session, string id, string pw);
	void HandleLoginAuth(LoginSessionRef session, string id, string pw);
};

extern std::unique_ptr<DBTransaction>	GDBTransaction;

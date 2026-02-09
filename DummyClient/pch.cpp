#include "pch.h"

ClientServiceRef GGameService = nullptr;
ClientServiceRef GChatService = nullptr;
IocpCoreRef GIocpCore = MakeShared<IocpCore>();
vector<DummyUserRef> GUsers;
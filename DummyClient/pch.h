#pragma once

#include "CorePch.h"

#include "Enum.pb.h"
#include "Login.pb.h"
#include "Game.pb.h"

#ifdef _DEBUG
#pragma comment(lib, "ServerCore\\Debug\\ServerCore.lib")
#pragma comment(lib, "Protobuf\\Debug\\libprotobufd.lib")
#else
#pragma comment(lib, "ServerCore\\Release\\ServerCore.lib")
#pragma comment(lib, "Protobuf\\Release\\libprotobuf.lib")
#endif

#define USING_SHARED_PTR(name) using name##Ref = std::shared_ptr<class name>;
USING_SHARED_PTR(GameSession);
USING_SHARED_PTR(ChatSession);
USING_SHARED_PTR(DummyUser);


extern ClientServiceRef GGameService;
extern ClientServiceRef GChatService;
extern IocpCoreRef GIocpCore;
extern vector<DummyUserRef> GUsers;
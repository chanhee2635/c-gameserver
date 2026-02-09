#pragma once

#include "CorePch.h"

#include "Enum.pb.h"
#include "Login.pb.h"  

#ifdef _DEBUG
#pragma comment(lib, "ServerCore\\Debug\\ServerCore.lib")
#pragma comment(lib, "Protobuf\\Debug\\libprotobufd.lib")
#pragma comment(lib, "hiredis\\Debug\\hiredisd.lib")
#pragma comment(lib, "redis++\\Debug\\redis++.lib")
#else
#pragma comment(lib, "ServerCore\\Release\\ServerCore.lib")
#pragma comment(lib, "Protobuf\\Release\\libprotobuf.lib")
#pragma comment(lib, "hiredis\\Release\\hiredis.lib")
#pragma comment(lib, "redis++\\Release\\redis++.lib")
#endif

#define USING_SHARED_PTR(name) using name##Ref = std::shared_ptr<class name>;
USING_SHARED_PTR(LoginSession);
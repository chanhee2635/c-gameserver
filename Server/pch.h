#pragma once

#include "CorePch.h"

// 작은 프로젝트에선 상관없지만 규모가 커지면 빌드할 때 시간이 오래걸릴 수 있다.
#include "Enum.pb.h"
#include "Game.pb.h"

#ifdef _DEBUG
#pragma comment(lib, "RecastNDetour\\Debug\\Detour-d.lib")
#pragma comment(lib, "RecastNDetour\\Debug\\Recast-d.lib")
#pragma comment(lib, "ServerCore\\Debug\\ServerCore.lib")
#pragma comment(lib, "Protobuf\\Debug\\libprotobufd.lib")
#pragma comment(lib, "hiredis\\Debug\\hiredisd.lib")
#pragma comment(lib, "redis++\\Debug\\redis++.lib")
#else
#pragma comment(lib, "RecastNDetour\\Release\\Detour.lib")
#pragma comment(lib, "RecastNDetour\\Release\\Recast.lib")
#pragma comment(lib, "ServerCore\\Release\\ServerCore.lib")
#pragma comment(lib, "Protobuf\\Release\\libprotobuf.lib")
#pragma comment(lib, "hiredis\\Release\\hiredis.lib")
#pragma comment(lib, "redis++\\Release\\redis++.lib")
#endif

#include "Enums.h"
#include "Struct.h"

#define USING_SHARED_PTR(name) using name##Ref = std::shared_ptr<class name>;
USING_SHARED_PTR(GameSession);
USING_SHARED_PTR(GameObject);
USING_SHARED_PTR(Creature);
USING_SHARED_PTR(Player);
USING_SHARED_PTR(Monster);
USING_SHARED_PTR(Room);
USING_SHARED_PTR(World);
USING_SHARED_PTR(GameScene);
USING_SHARED_PTR(Zone);
USING_SHARED_PTR(SummaryData);
USING_SHARED_PTR(MoveResult);

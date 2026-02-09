#pragma once

#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"

struct NavMeshSetHeader
{
	int32 magic;			// 파일 식별자
	int32 version;			// 버전
	int32 numTiles;			// 타일 개수
	dtNavMeshParams params;	// 내비메쉬 설정 (위치, 크기 등)
};

struct NavMeshTileHeader
{
	dtTileRef tileRef;		// 타일 참조 ID
	int32 dataSize;			// 타일 바이너리 크기
};

static const int NAVMESHSET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T';  // 'MSET'
static const int NAVMESHSET_VERSION = 1;

class NavigationManager
{
public:
	NavigationManager();
	~NavigationManager();

	/*
	* @brief 내비게이션 매쉬 데이터를 파일에서 로드하여 초기화
	* @param path 내비메쉬 데이터 경로
	*/
	bool LoadNavMesh(const char* path);
	float GetHeight(Vector3 pos);
	bool FindPath(Vector3 start, Vector3 end, Vector<Vector3>& path);
	bool IsValidLocation(Vector3 pos, Vector3& outPos);
	bool CanMoveTo(Vector3 startPos, Vector3 endPos);

private:
	dtNavMesh* _navMesh = nullptr;
	dtNavMeshQuery* _navQuery = nullptr;
	dtQueryFilter _filter;
	float _halfExtents[3] = { 2.0f, 4.0f, 2.0f };  // 탐색 범위
};

extern std::unique_ptr<NavigationManager>	GNavigationManager;
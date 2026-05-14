#pragma once

#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"

struct NavMeshSetHeader
{
	int32 magic;
	int32 version;
	int32 numTiles;
	dtNavMeshParams params;
};

struct NavMeshTileHeader
{
	dtTileRef tileRef;
	int32 dataSize;
};

static const int32 NAVMESHSET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T';
static const int32 NAVMESHSET_VERSION = 1;

class NavigationManager
{
public:
	NavigationManager();
	~NavigationManager();

	bool LoadNavMesh(const char* path);
	float GetHeight(Vector3 pos);
	bool FindPath(Vector3 start, Vector3 end, Vector<Vector3>& path);
	bool IsValidLocation(Vector3 pos, Vector3& outPos);
	bool CanMoveTo(Vector3 startPos, Vector3 endPos);

private:
	dtNavMeshQuery* GetQuery();

	dtNavMesh* _navMesh = nullptr;
	dtQueryFilter _filter;
	float         _halfExtents[3] = { 1.0f, 2.0f, 1.0f };
};


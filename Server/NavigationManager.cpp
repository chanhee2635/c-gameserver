#include "pch.h"
#include "NavigationManager.h"

unique_ptr<NavigationManager> GNavigationManager = make_unique<NavigationManager>();

NavigationManager::NavigationManager() : _navMesh(nullptr), _navQuery(nullptr)
{
	// 기본 필터 설정 (오브젝트별 지형별 설정을 다르게 하려면 새로운 필터 생성)
	_filter.setIncludeFlags(0xFFFF);	// 통과 허용
	_filter.setExcludeFlags(0);			// 특정 지역 검색 제외
	_filter.setAreaCost(0, 1.0f);		// 특정 지형 비용 증가
}

NavigationManager::~NavigationManager()
{
	if (_navQuery != nullptr)
	{
		dtFreeNavMeshQuery(_navQuery);
		_navQuery = nullptr;
	}

	if (_navMesh != nullptr)
	{
		dtFreeNavMesh(_navMesh);
		_navMesh = nullptr;
	}
}

bool NavigationManager::LoadNavMesh(const char* path)
{
	// 파일 열기 (읽기 모드)
	FILE* fp = fopen(path, "rb");
	if (!fp) return false;

	// 헤더 읽기 및 매직 넘버 검증
	NavMeshSetHeader header;
	size_t readLen = fread(&header, sizeof(NavMeshSetHeader), 1, fp);
	if (readLen != 1 || header.magic != NAVMESHSET_MAGIC)
	{
		fclose(fp);
		return false;
	}

	_navMesh = dtAllocNavMesh();
	if (_navMesh == nullptr)
	{
		fclose(fp);
		return false;
	}

	// 헤더 파라미터로 내비메쉬 레이아웃 설정
	dtStatus status = _navMesh->init(&header.params);
	if (dtStatusFailed(status))
	{
		dtFreeNavMesh(_navMesh);
		fclose(fp);
		return false;
	}

	// 타일 데이터 로드
	for (int32 i = 0; i < header.numTiles; ++i)
	{
		NavMeshTileHeader tileHeader;
		fread(&tileHeader, sizeof(NavMeshTileHeader), 1, fp);

		if (!tileHeader.tileRef || !tileHeader.dataSize) break;

		unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
		if (!data) break;

		fread(data, tileHeader.dataSize, 1, fp);

		_navMesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, 0);
	}

	fclose(fp);

	_navQuery = dtAllocNavMeshQuery();
	status = _navQuery->init(_navMesh, 2048);

	if (dtStatusFailed(status))
	{
		dtFreeNavMeshQuery(_navQuery);
		dtFreeNavMesh(_navMesh);
		return false;
	}

	return true;
}

float NavigationManager::GetHeight(Vector3 pos)
{
	dtPolyRef polyRef;
	float closestPt[3];
	float inputPos[3] = { pos.x, pos.y, pos.z };

	dtStatus status = _navQuery->findNearestPoly(inputPos, _halfExtents, &_filter, &polyRef, closestPt);

	if (dtStatusFailed(status) || polyRef == 0)
		return -999.0f;

	float height = 0;
	status = _navQuery->getPolyHeight(polyRef, closestPt, &height);
	
	if (dtStatusSucceed(status))
		return height;

	return closestPt[1];
}

bool NavigationManager::FindPath(Vector3 start, Vector3 end, Vector<Vector3>& path)
{
	path.clear();

	float startPos[3] = { start.x, start.y, start.z };
	float endPos[3] = { end.x, end.y, end.z };

	dtPolyRef startPoly, endPoly;
	float startClosest[3], endClosest[3];

	_navQuery->findNearestPoly(startPos, _halfExtents, &_filter, &startPoly, startClosest);
	_navQuery->findNearestPoly(endPos, _halfExtents, &_filter, &endPoly, endClosest);

	if (startPoly == 0 || endPoly == 0)
		return false;

	static const int MAX_POLYS = 256;
	dtPolyRef polyPath[MAX_POLYS];
	int nPath = 0;

	dtStatus status = _navQuery->findPath(startPoly, endPoly, startClosest, endClosest, &_filter, polyPath, &nPath, MAX_POLYS);

	if (dtStatusFailed(status) || nPath == 0)
		return false;

	float straightPath[MAX_POLYS * 3];
	int nStraightPath = 0;

	status = _navQuery->findStraightPath(startClosest, endClosest, polyPath, nPath, straightPath, nullptr, nullptr, &nStraightPath, MAX_POLYS);

	if (dtStatusFailed(status) || nStraightPath == 0)
		return false;

	for (int i = 0; i < nStraightPath; ++i)
	{
		path.push_back(Vector3(
			straightPath[i * 3],
			straightPath[i * 3 + 1],
			straightPath[i * 3 + 2]
		));
	}

	return true;
}

bool NavigationManager::IsValidLocation(Vector3 pos, Vector3& outPos)
{
	dtPolyRef polyRef;
	float closestPt[3];
	float inputPos[3] = { pos.x, pos.y, pos.z };

	dtStatus status = _navQuery->findNearestPoly(inputPos, _halfExtents, &_filter, &polyRef, closestPt);

	if (dtStatusFailed(status) || polyRef == 0)
		return false;

	outPos = Vector3(closestPt[0], closestPt[1], closestPt[2]);
	return true;
}

bool NavigationManager::CanMoveTo(Vector3 startPos, Vector3 endPos)
{
	float distanceSq = Vector3::DistanceSquared(startPos, endPos);
	if (distanceSq < 0.0001f)
		return true;

	dtPolyRef startPoly;
	float startPt[3];
	float sPos[3] = { startPos.x, startPos.y, startPos.z };

	_navQuery->findNearestPoly(sPos, _halfExtents, &_filter, &startPoly, startPt);
	if (startPoly == 0) return false;

	float ePos[3] = { endPos.x ,endPos.y, endPos.z };
	float t = 0;
	float hitNormal[3];
	dtPolyRef polys[20];
	int nPolys;

	dtStatus status = _navQuery->raycast(startPoly, startPt, ePos, &_filter, &t, hitNormal, polys, &nPolys, 20);

	return dtStatusSucceed(status) && t >= 1.0f;
}

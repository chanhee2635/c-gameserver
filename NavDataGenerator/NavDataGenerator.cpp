#include "pch.h"
#include <iostream>
#include <Recast.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <InputGeom.h>

using namespace std;

struct NavMeshSetHeader
{
    int magic;			// 파일 식별자
    int version;			// 버전
    int numTiles;			// 타일 개수
    dtNavMeshParams params;	// 내비메쉬 설정 (위치, 크기 등)
};

struct NavMeshTileHeader
{
    dtTileRef tileRef;		// 타일 참조 ID
    int dataSize;			// 타일 바이너리 크기
};

static const int NAVMESHSET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T';  // 'MSET'
static const int NAVMESHSET_VERSION = 1;

rcPolyMesh* BuildRecastPolyMesh(rcContext* ctx, InputGeom* geom, rcPolyMeshDetail*& dmesh);

int main()
{
    rcContext ctx;
    InputGeom geom;
    if (!geom.load(&ctx, "../Client/Assets/Resources/Data/SceneNavMesh.obj"))
    {
        cout << "에러 : .obj 파일을 로드할 수 없습니다!" << endl;
        return -1;
    }

    rcPolyMeshDetail* dmesh = nullptr; 
    rcPolyMesh* pmesh = BuildRecastPolyMesh(&ctx, &geom, dmesh);

    if (!pmesh || !dmesh)
    {
        cout << "에러 : NavMesh 빌드 실패!" << endl;
        return -1;
    }

    dtNavMeshCreateParams params;
    memset(&params, 0, sizeof(params));

    params.verts = pmesh->verts;
    params.vertCount = pmesh->nverts;
    params.polys = pmesh->polys;
    params.polyAreas = pmesh->areas;
    params.polyFlags = pmesh->flags;
    params.polyCount = pmesh->npolys;
    params.nvp = pmesh->nvp;

    params.detailVerts = dmesh->verts;
    params.detailVertsCount = dmesh->nverts;
    params.detailTris = dmesh->tris;
    params.detailTriCount = dmesh->ntris;

    const float* bmin = geom.getMeshBoundsMin();
    const float* bmax = geom.getMeshBoundsMax();
    rcVcopy(params.bmin, bmin);
    rcVcopy(params.bmax, bmax);

    params.walkableHeight = 2.0f;
    params.walkableRadius = 0.5f;
    params.walkableClimb = 0.5f;
    params.cs = 0.1f;
    params.ch = 0.05f;
    params.buildBvTree = true;

    unsigned char* navData = nullptr;
    int navDataSize = 0;

    if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
    {
        cout << "네비메쉬 데이터 생성 실패!" << endl;
        return -1;
    }
    
    FILE* fp = fopen("SceneNavMesh.nav", "wb");
    if (fp)
    {
        NavMeshSetHeader header;
        header.magic = NAVMESHSET_MAGIC;
        header.version = NAVMESHSET_VERSION;
        header.numTiles = 1;
        rcVcopy(header.params.orig, bmin);
        header.params.tileWidth = params.bmax[0] - params.bmin[0];
        header.params.tileHeight = params.bmax[2] - params.bmin[2];
        header.params.maxTiles = 1;
        header.params.maxPolys = params.polyCount;
        fwrite(&header, sizeof(NavMeshSetHeader), 1, fp);

        NavMeshTileHeader tileHeader;
        tileHeader.tileRef = 1;
        tileHeader.dataSize = navDataSize;
        fwrite(&tileHeader, sizeof(NavMeshTileHeader), 1, fp);

        fwrite(navData, navDataSize, 1, fp);
        fclose(fp);
        cout << "Bake 완료! SceneNavMesh.nav 생성!" << endl;
    }

    dtFree(navData);
    rcFreePolyMesh(pmesh);
    rcFreePolyMeshDetail(dmesh);

    return 0;
}

rcPolyMesh* BuildRecastPolyMesh(rcContext* ctx, InputGeom* geom, rcPolyMeshDetail*& dmesh)
{
    const float* verts = geom->getMesh()->getVerts();      // 정점 배열
    const int nverts = geom->getMesh()->getVertCount();    // 정점 개수
    const int* tris = geom->getMesh()->getTris();          // 인덱스 배열
    const int ntris = geom->getMesh()->getTriCount();      // 삼각형 개수

    const float* bmin = geom->getMeshBoundsMin();
    const float* bmax = geom->getMeshBoundsMax();

    rcConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.cs = 0.1f;
    cfg.ch = 0.05f;
    cfg.walkableSlopeAngle = 60.0f;
    cfg.walkableHeight = (int)ceilf(2.0f / cfg.ch);
    cfg.walkableClimb = (int)floorf(0.5f / cfg.ch);
    cfg.walkableRadius = (int)ceilf(0.5f / cfg.cs);
    cfg.maxEdgeLen = (int)(12.0f / cfg.cs);
    cfg.maxSimplificationError = 0.5f;
    cfg.minRegionArea = (int)rcSqr(2);
    cfg.mergeRegionArea = (int)rcSqr(20);
    cfg.maxVertsPerPoly = 6;
    cfg.detailSampleDist = 6.0f;
    cfg.detailSampleMaxError = 0.2f;

    rcCalcGridSize(bmin, bmax, cfg.cs, &cfg.width, &cfg.height);

    rcHeightfield* hf = rcAllocHeightfield();
    rcCreateHeightfield(ctx, *hf, cfg.width, cfg.height, bmin, bmax, cfg.cs, cfg.ch);

    unsigned char* areas = new unsigned char[ntris];
    memset(areas, RC_WALKABLE_AREA, ntris);
    rcRasterizeTriangles(ctx, verts, nverts, tris, areas, ntris, *hf, cfg.walkableClimb);

    rcFilterLowHangingWalkableObstacles(ctx, cfg.walkableClimb, *hf);
    rcFilterLedgeSpans(ctx, cfg.walkableHeight, cfg.walkableClimb, *hf);
    rcFilterWalkableLowHeightSpans(ctx, cfg.walkableHeight, *hf);

    rcCompactHeightfield* chf = rcAllocCompactHeightfield();
    if (!rcBuildCompactHeightfield(ctx, cfg.walkableHeight, cfg.walkableClimb, *hf, *chf))
    {
        cout << "에러: Compact HeightField를 빌드할 수 없습니다!" << endl;
        return nullptr;
    }

    if (!rcErodeWalkableArea(ctx, cfg.walkableRadius, *chf)) {
        cout << "에러: Area 침식 실패!" << endl;
        return nullptr;
    }

    if (!rcBuildDistanceField(ctx, *chf))
    {
        cout << "에러: Distanc Field 생성 실패!" << endl;
        return nullptr;
    }

    if (!rcBuildRegions(ctx, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea))
    {
        cout << "에러: Regions 생성 실패!" << endl;
        return nullptr;
    }

    rcContourSet* cset = rcAllocContourSet();
    if (!rcBuildContours(ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset)) {
        cout << "에러: Contours 생성 실패!" << endl;
        return nullptr;
    }

    rcPolyMesh* pmesh = rcAllocPolyMesh();
    if (!rcBuildPolyMesh(ctx, *cset, cfg.maxVertsPerPoly, *pmesh))
    {
        cout << "에러: PolyMesh 생성 실패!" << endl;
        return nullptr;
    }

    dmesh = rcAllocPolyMeshDetail();
    if (!rcBuildPolyMeshDetail(ctx, *pmesh, *chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *dmesh))
    {
        cout << "에러: Detail Mesh 생성 실패!" << endl;
        return nullptr;
    }

    for (int i = 0; i < pmesh->npolys; ++i)
    {
        if (pmesh->areas[i] == RC_WALKABLE_AREA)
            pmesh->flags[i] = 1;
    }
    
    delete[] areas;

    rcFreeHeightField(hf);
    rcFreeCompactHeightfield(chf);
    rcFreeContourSet(cset);

    return pmesh;
}

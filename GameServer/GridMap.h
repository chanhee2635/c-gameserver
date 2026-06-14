#pragma once
#include "GameTypes.h"

class GridMap
{
public:
    bool Load(const char* path);
    bool IsLoaded() const { return !_cells.empty(); }

    bool IsWalkable(const Vector3& pos) const;
    // Player anti-phase gate. Looser than pathfinding's LineOfSight: tolerates the ~1-cell
    // over-block where the coarse grid disagrees with Unity's NavMesh at edges/corners (no
    // diagonal corner-cut rejection), so navmesh-valid corner moves aren't rubber-banded,
    // while a cross of a wall thicker than PLAYER_PHASE_TOLERANCE_CELLS still blocks.
    bool IsPlayerPathClear(const Vector3& from, const Vector3& to) const;

    bool FindPath(Vector3 start, Vector3 end, Vector<Vector3>& outPath) const;

    int32 Cols() const { return _cols; }
    int32 Rows() const { return _rows; }

private:
    bool    InBounds(int32 cx, int32 cz) const { return cx >= 0 && cx < _cols && cz >= 0 && cz < _rows; }
    bool    CellWalkable(int32 cx, int32 cz) const
    {
        return InBounds(cx, cz) && _cells[static_cast<size_t>(cz) * _cols + cx] == 0;
    }
    void    ToCell(const Vector3& pos, int32& cx, int32& cz) const;
    Vector3 CellCenter(int32 cx, int32 cz) const;
    bool    LineOfSight(int32 x0, int32 z0, int32 x1, int32 z1) const;

    // Player moves tolerate grazing this many consecutive blocked cells, absorbing the
    // GridMap-vs-NavMesh quantization mismatch at walls/corners. Keep small: a wall thinner
    // than this many cells becomes player-passable. (Monster pathfinding ignores this.)
    static constexpr int32 PLAYER_PHASE_TOLERANCE_CELLS = 1;

    static constexpr int32 MAGIC = 0x57414C4B;

    int32 _cols = 0;
    int32 _rows = 0;
    float _cellSize = 1.f;
    float _originX  = 0.f;
    float _originZ  = 0.f;
    Vector<uint8> _cells;  
};

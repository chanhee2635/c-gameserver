#pragma once
#include "GameTypes.h"

class GridMap
{
public:
    bool Load(const char* path);
    bool IsLoaded() const { return !_cells.empty(); }

    bool IsWalkable(const Vector3& pos) const;
    bool IsPathWalkable(const Vector3& from, const Vector3& to) const;

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

    static constexpr int32 MAGIC = 0x57414C4B; 

    int32 _cols = 0;
    int32 _rows = 0;
    float _cellSize = 1.f;
    float _originX  = 0.f;
    float _originZ  = 0.f;
    Vector<uint8> _cells;  
};

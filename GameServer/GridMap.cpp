#include "pch.h"
#include "GridMap.h"
#include <fstream>
#include <unordered_map>
#include <queue>
#include <cmath>
#include <algorithm>

namespace
{
    constexpr float SQRT2 = 1.41421356f;
}

bool GridMap::Load(const char* path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    int32 magic = 0;
    f.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != MAGIC) return false;

    f.read(reinterpret_cast<char*>(&_cols),     sizeof(_cols));
    f.read(reinterpret_cast<char*>(&_rows),     sizeof(_rows));
    f.read(reinterpret_cast<char*>(&_cellSize), sizeof(_cellSize));
    f.read(reinterpret_cast<char*>(&_originX),  sizeof(_originX));
    f.read(reinterpret_cast<char*>(&_originZ),  sizeof(_originZ));

    if (_cols <= 0 || _rows <= 0 || _cellSize <= 0.f) return false;

    const size_t n = static_cast<size_t>(_cols) * _rows;
    if (n == 0 || n > 100'000'000) return false;

    _cells.resize(n);
    f.read(reinterpret_cast<char*>(_cells.data()), static_cast<std::streamsize>(n));
    return static_cast<size_t>(f.gcount()) == n;
}

void GridMap::ToCell(const Vector3& pos, int32& cx, int32& cz) const
{
    cx = static_cast<int32>(std::floor((pos.x - _originX) / _cellSize));
    cz = static_cast<int32>(std::floor((pos.z - _originZ) / _cellSize));
}

Vector3 GridMap::CellCenter(int32 cx, int32 cz) const
{
    return Vector3{
        _originX + (static_cast<float>(cx) + 0.5f) * _cellSize,
        0.f,
        _originZ + (static_cast<float>(cz) + 0.5f) * _cellSize
    };
}

bool GridMap::IsWalkable(const Vector3& pos) const
{
    if (_cells.empty()) return true;  
    int32 cx, cz;
    ToCell(pos, cx, cz);
    return CellWalkable(cx, cz);
}

bool GridMap::IsPathWalkable(const Vector3& from, const Vector3& to) const
{
    if (_cells.empty()) return true;
    int32 x0, z0, x1, z1;
    ToCell(from, x0, z0);
    ToCell(to,   x1, z1);
    // LineOfSight checks every cell along the segment (incl. both endpoints) and blocks
    // diagonal corner-cutting, so this subsumes the destination-only IsWalkable check.
    return LineOfSight(x0, z0, x1, z1);
}

bool GridMap::LineOfSight(int32 x0, int32 z0, int32 x1, int32 z1) const
{
    int32 dx = std::abs(x1 - x0);
    int32 dz = std::abs(z1 - z0);
    int32 sx = (x0 < x1) ? 1 : -1;
    int32 sz = (z0 < z1) ? 1 : -1;
    int32 err = dx - dz;
    int32 x = x0, z = z0;

    while (true)
    {
        if (!CellWalkable(x, z)) return false;
        if (x == x1 && z == z1)  return true;

        int32 e2 = 2 * err;
        bool steppedX = false, steppedZ = false;
        if (e2 > -dz) { err -= dz; x += sx; steppedX = true; }
        if (e2 <  dx) { err += dx; z += sz; steppedZ = true; }

        if (steppedX && steppedZ &&
            (!CellWalkable(x - sx, z) || !CellWalkable(x, z - sz)))
            return false;
    }
}

bool GridMap::FindPath(Vector3 start, Vector3 end, Vector<Vector3>& outPath) const
{
    outPath.clear();
    if (_cells.empty()) return false;

    int32 sx, sz, gx, gz;
    ToCell(start, sx, sz);
    ToCell(end,   gx, gz);

    if (!CellWalkable(sx, sz) || !CellWalkable(gx, gz)) return false;

    const int32 cols = _cols;
    const int32 startCell = sz * cols + sx;
    const int32 goalCell  = gz * cols + gx;

    if (startCell == goalCell)
    {
        outPath.push_back(start);
        outPath.push_back(end);
        return true;
    }

    auto heuristic = [&](int32 x, int32 z) -> float
    {
        float dx = static_cast<float>(std::abs(x - gx));
        float dz = static_cast<float>(std::abs(z - gz));
        return (dx + dz) + (SQRT2 - 2.f) * std::min(dx, dz);  
    };

    std::unordered_map<int32, float> gScore;
    std::unordered_map<int32, int32> cameFrom;

    using PQItem = std::pair<float, int32>;  
    std::priority_queue<PQItem, std::vector<PQItem>, std::greater<PQItem>> open;

    gScore[startCell] = 0.f;
    open.push({ heuristic(sx, sz), startCell });

    static const int32 DX[8] = { 1, -1, 0, 0,  1,  1, -1, -1 };
    static const int32 DZ[8] = { 0, 0, 1, -1,  1, -1,  1, -1 };

    constexpr int32 MAX_EXPAND = 20000;   
    int32 expanded = 0;
    bool  found    = false;

    while (!open.empty())
    {
        auto [f, cur] = open.top();
        open.pop();

        if (cur == goalCell) { found = true; break; }
        if (++expanded > MAX_EXPAND) break;

        const int32 cx = cur % cols;
        const int32 cz = cur / cols;
        const float curG = gScore[cur];

        for (int i = 0; i < 8; ++i)
        {
            const int32 nx = cx + DX[i];
            const int32 nz = cz + DZ[i];
            if (!CellWalkable(nx, nz)) continue;

            if (i >= 4 && (!CellWalkable(cx + DX[i], cz) || !CellWalkable(cx, cz + DZ[i])))
                continue;

            const float stepCost = (i >= 4) ? SQRT2 : 1.f;
            const int32 nCell = nz * cols + nx;
            const float tentative = curG + stepCost;

            auto it = gScore.find(nCell);
            if (it == gScore.end() || tentative < it->second)
            {
                gScore[nCell]   = tentative;
                cameFrom[nCell] = cur;
                open.push({ tentative + heuristic(nx, nz), nCell });
            }
        }
    }

    if (!found) return false;

    Vector<int32> cellPath;
    for (int32 c = goalCell; ; c = cameFrom[c])
    {
        cellPath.push_back(c);
        if (c == startCell) break;
    }

    Vector<int32> fwd;
    fwd.reserve(cellPath.size());
    for (int32 i = static_cast<int32>(cellPath.size()) - 1; i >= 0; --i)
        fwd.push_back(cellPath[i]);

    Vector<int32> corners;
    corners.push_back(fwd.front());
    int32 anchor = 0;
    for (int32 i = 2; i < static_cast<int32>(fwd.size()); ++i)
    {
        int32 ax = fwd[anchor] % cols, az = fwd[anchor] / cols;
        int32 bx = fwd[i]      % cols, bz = fwd[i]      / cols;
        if (!LineOfSight(ax, az, bx, bz))
        {
            corners.push_back(fwd[i - 1]);  
            anchor = i - 1;
        }
    }
    corners.push_back(fwd.back());

    outPath.push_back(start);
    for (int32 i = 1; i < static_cast<int32>(corners.size()) - 1; ++i)
        outPath.push_back(CellCenter(corners[i] % cols, corners[i] / cols));
    outPath.push_back(end);
    return true;
}

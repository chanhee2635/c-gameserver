#pragma once
#include "DbConnection.h"

template<int32 ParamCount, int32 ColumnCount>
class DbBind
{
public:
    DbBind(DbConnection& conn, const WCHAR* query)
        : _conn(conn), _query(query)
    {
        ::memset(_paramIndex,  0, sizeof(_paramIndex));
        ::memset(_columnIndex, 0, sizeof(_columnIndex));
    }

    // ── Input parameters ─────────────────────────────────────────────────────

    template<typename T>
    void BindParam(T& value)
    {
        _conn.BindParam(_paramIdx + 1, value, &_paramIndex[_paramIdx]);
        _paramIdx++;
    }

    void BindParam(WCHAR* str)
    {
        _conn.BindParam(_paramIdx + 1, str, &_paramIndex[_paramIdx]);
        _paramIdx++;
    }

    void BindParam(BYTE* bin, int32 size)
    {
        _conn.BindParam(_paramIdx + 1, bin, size, &_paramIndex[_paramIdx]);
        _paramIdx++;
    }

    // ── Output columns ───────────────────────────────────────────────────────

    template<typename T>
    void BindCol(T& value)
    {
        _conn.BindCol(_columnIdx + 1, value, &_columnIndex[_columnIdx]);
        _columnIdx++;
    }

    void BindCol(WCHAR* str, int32 size)
    {
        _conn.BindCol(_columnIdx + 1, str, size, &_columnIndex[_columnIdx]);
        _columnIdx++;
    }

    void BindCol(BYTE* bin, int32 size)
    {
        _conn.BindCol(_columnIdx + 1, bin, size, &_columnIndex[_columnIdx]);
        _columnIdx++;
    }

    // ── Execute ──────────────────────────────────────────────────────────────

    bool Execute()
    {
        ASSERT_CRASH(_paramIdx  == ParamCount);
        ASSERT_CRASH(_columnIdx == ColumnCount);

        if (!_conn.Prepare(_query)) return false;
        return _conn.Execute();
    }

private:
    DbConnection& _conn;
    const WCHAR*  _query;

    int32  _paramIdx  = 0;
    int32  _columnIdx = 0;

    SQLLEN _paramIndex [ParamCount  == 0 ? 1 : ParamCount];
    SQLLEN _columnIndex[ColumnCount == 0 ? 1 : ColumnCount];
};

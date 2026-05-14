#include "pch.h"
#include "DbConnectionPool.h"

void DbConnectionPool::Init(const WCHAR* connectionString, int32 count)
{
    _connectionString = connectionString;

    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_env);
    SQLSetEnvAttr(_env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);

    for (int32 i = 0; i < count; i++)
    {
        DbConnection* conn = new DbConnection();
        if (conn->Connect(_env, connectionString))
            _connections.push_back(conn);
        else
        {
            LOG_ERROR(L"DbConnection Connect Failed index=" + std::to_wstring(i));
            delete conn;
        }
    }

    LOG_INFO(L"DbConnectionPool initialized ("
        + std::to_wstring(_connections.size()) + L"/"
        + std::to_wstring(count) + L" connections)");
}

void DbConnectionPool::Clear()
{
    WRITE_LOCK;
    for (DbConnection* conn : _connections)
    {
        conn->Clear();
        delete conn;
    }
    _connections.clear();

    if (_env != SQL_NULL_HANDLE)
    {
        SQLFreeHandle(SQL_HANDLE_ENV, _env);
        _env = SQL_NULL_HANDLE;
    }
}

DbConnection* DbConnectionPool::Pop()
{
    WRITE_LOCK;
    if (_connections.empty())
        return nullptr;

    DbConnection* conn = _connections.back();
    _connections.pop_back();
    return conn;
}

void DbConnectionPool::Push(DbConnection* conn)
{
    WRITE_LOCK;
    _connections.push_back(conn);
}
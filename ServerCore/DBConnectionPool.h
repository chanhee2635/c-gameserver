#pragma once
#include "DbConnection.h"

class DbConnectionPool
{
public:
    void        Init(const WCHAR* connectionString, int32 count);
    void        Clear();

    DbConnection* Pop();
    void            Push(DbConnection* conn);

private:
    USE_LOCK;

    SQLHENV                     _env = SQL_NULL_HANDLE;
    wstring                     _connectionString;
    Vector<DbConnection*>       _connections;
};


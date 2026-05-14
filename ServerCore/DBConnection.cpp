#include "pch.h"
#include "DbConnection.h"

bool DbConnection::Connect(SQLHENV env, const WCHAR* connectionString)
{
    if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_DBC, env, &_connection)))
        return false;

    WCHAR outBuf[1024];
    SQLSMALLINT outLen = 0;
    SQLRETURN ret = SQLDriverConnectW(
        _connection, nullptr,
        (SQLWCHAR*)connectionString, SQL_NTS,
        outBuf, 1024, &outLen,
        SQL_DRIVER_NOPROMPT);

    if (!SQL_SUCCEEDED(ret))
    {
        HandleError(SQL_HANDLE_DBC, _connection);
        return false;
    }

    if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, _connection, &_statement)))
        return false;

    _env = env;
    return true;
}

void DbConnection::Clear()
{
    if (_statement != SQL_NULL_HANDLE)
    {
        SQLFreeHandle(SQL_HANDLE_STMT, _statement);
        _statement = SQL_NULL_HANDLE;
    }
    if (_connection != SQL_NULL_HANDLE)
    {
        SQLDisconnect(_connection);
        SQLFreeHandle(SQL_HANDLE_DBC, _connection);
        _connection = SQL_NULL_HANDLE;
    }
}

bool DbConnection::Prepare(const WCHAR* query)
{
    if (!SQL_SUCCEEDED(SQLPrepareW(_statement, (SQLWCHAR*)query, SQL_NTS)))
    {
        HandleError(SQL_HANDLE_STMT, _statement);
        return false;
    }
    return true;
}

bool DbConnection::Execute()
{
    if (!SQL_SUCCEEDED(SQLExecute(_statement)))
    {
        HandleError(SQL_HANDLE_STMT, _statement);
        return false;
    }
    return true;
}

bool DbConnection::Execute(const WCHAR* query)
{
    SQLRETURN ret = SQLExecDirectW(_statement, (SQLWCHAR*)query, SQL_NTS);
    if (!SQL_SUCCEEDED(ret))
    {
        HandleError(SQL_HANDLE_STMT, _statement);
        return false;
    }
    return true;
}

bool DbConnection::Fetch()
{
    SQLRETURN ret = SQLFetch(_statement);
    return (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO);
}

int32 DbConnection::GetRowCount()
{
    SQLLEN rowCount = 0;
    SQLRowCount(_statement, &rowCount);
    return static_cast<int32>(rowCount);
}

void DbConnection::Unbind()
{
    SQLFreeStmt(_statement, SQL_RESET_PARAMS);
    SQLFreeStmt(_statement, SQL_UNBIND);
    SQLFreeStmt(_statement, SQL_CLOSE);
}

void DbConnection::HandleError(SQLSMALLINT handleType, SQLHANDLE handle)
{
    SQLWCHAR   sqlState[6];
    SQLINTEGER nativeError;
    SQLWCHAR   message[1024];
    SQLSMALLINT msgLen;

    SQLRETURN ret = SQLGetDiagRecW(handleType, handle, 1,
        sqlState, &nativeError, message, 1024, &msgLen);

    if (SQL_SUCCEEDED(ret))
        LOG_ERROR(wstring(L"[ODBC] ") + sqlState + L" " + message);
}

// ── BindParam ────────────────────────────────────────────────────────────────

void DbConnection::BindParam(int32 idx, bool& value, SQLLEN* len)
{
    *len = sizeof(bool);
    SQLBindParameter(_statement, idx, SQL_PARAM_INPUT, SQL_C_TINYINT, SQL_TINYINT, 0, 0, &value, 0, len);
}
void DbConnection::BindParam(int32 idx, float& value, SQLLEN* len)
{
    *len = sizeof(float);
    SQLBindParameter(_statement, idx, SQL_PARAM_INPUT, SQL_C_FLOAT, SQL_REAL, 0, 0, &value, 0, len);
}
void DbConnection::BindParam(int32 idx, double& value, SQLLEN* len)
{
    *len = sizeof(double);
    SQLBindParameter(_statement, idx, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DOUBLE, 0, 0, &value, 0, len);
}
void DbConnection::BindParam(int32 idx, int8& value, SQLLEN* len)
{
    *len = sizeof(int8);
    SQLBindParameter(_statement, idx, SQL_PARAM_INPUT, SQL_C_TINYINT, SQL_TINYINT, 0, 0, &value, 0, len);
}
void DbConnection::BindParam(int32 idx, int16& value, SQLLEN* len)
{
    *len = sizeof(int16);
    SQLBindParameter(_statement, idx, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_SMALLINT, 0, 0, &value, 0, len);
}
void DbConnection::BindParam(int32 idx, int32& value, SQLLEN* len)
{
    *len = sizeof(int32);
    SQLBindParameter(_statement, idx, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &value, 0, len);
}
void DbConnection::BindParam(int32 idx, int64& value, SQLLEN* len)
{
    *len = sizeof(int64);
    SQLBindParameter(_statement, idx, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &value, 0, len);
}
void DbConnection::BindParam(int32 idx, uint64& value, SQLLEN* len)
{
    *len = sizeof(uint64);
    SQLBindParameter(_statement, idx, SQL_PARAM_INPUT, SQL_C_UBIGINT, SQL_BIGINT, 0, 0, &value, 0, len);
}
void DbConnection::BindParam(int32 idx, WCHAR* str, SQLLEN* len)
{
    *len = SQL_NTS;
    SQLBindParameter(_statement, idx, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR,
        wcslen(str), 0, str, 0, len);
}
void DbConnection::BindParam(int32 idx, BYTE* bin, int32 size, SQLLEN* len)
{
    *len = size;
    SQLBindParameter(_statement, idx, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_BINARY,
        size, 0, bin, size, len);
}

// ── BindCol ──────────────────────────────────────────────────────────────────

void DbConnection::BindCol(int32 idx, bool& value, SQLLEN* len)
{
    SQLBindCol(_statement, idx, SQL_C_TINYINT, &value, sizeof(bool), len);
}
void DbConnection::BindCol(int32 idx, float& value, SQLLEN* len)
{
    SQLBindCol(_statement, idx, SQL_C_FLOAT, &value, sizeof(float), len);
}
void DbConnection::BindCol(int32 idx, double& value, SQLLEN* len)
{
    SQLBindCol(_statement, idx, SQL_C_DOUBLE, &value, sizeof(double), len);
}
void DbConnection::BindCol(int32 idx, int8& value, SQLLEN* len)
{
    SQLBindCol(_statement, idx, SQL_C_TINYINT, &value, sizeof(int8), len);
}
void DbConnection::BindCol(int32 idx, int16& value, SQLLEN* len)
{
    SQLBindCol(_statement, idx, SQL_C_SHORT, &value, sizeof(int16), len);
}
void DbConnection::BindCol(int32 idx, int32& value, SQLLEN* len)
{
    SQLBindCol(_statement, idx, SQL_C_LONG, &value, sizeof(int32), len);
}
void DbConnection::BindCol(int32 idx, int64& value, SQLLEN* len)
{
    SQLBindCol(_statement, idx, SQL_C_SBIGINT, &value, sizeof(int64), len);
}
void DbConnection::BindCol(int32 idx, uint64& value, SQLLEN* len)
{
    SQLBindCol(_statement, idx, SQL_C_UBIGINT, &value, sizeof(uint64), len);
}
void DbConnection::BindCol(int32 idx, WCHAR* str, int32 size, SQLLEN* len)
{
    SQLBindCol(_statement, idx, SQL_C_WCHAR, str, (SQLLEN)size * sizeof(WCHAR), len);
}
void DbConnection::BindCol(int32 idx, BYTE* bin, int32 size, SQLLEN* len)
{
    SQLBindCol(_statement, idx, SQL_C_BINARY, bin, size, len);
}

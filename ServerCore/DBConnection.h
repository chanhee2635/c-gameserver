#pragma once

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#pragma comment(lib, "odbc32.lib")

class DbConnection
{
public:
    bool        Connect(SQLHENV env, const WCHAR* connectionString);
    void        Clear();

    bool        Prepare(const WCHAR* query);
    bool        Execute();    
    bool        Execute(const WCHAR* query);
    bool        Fetch();
    int32       GetRowCount();
    void        Unbind();
    void        HandleError(SQLSMALLINT handleType, SQLHANDLE handle);


    // Input parameters
    void        BindParam(int32 idx, bool& value, SQLLEN* len);
    void        BindParam(int32 idx, float& value, SQLLEN* len);
    void        BindParam(int32 idx, double& value, SQLLEN* len);
    void        BindParam(int32 idx, int8& value, SQLLEN* len);
    void        BindParam(int32 idx, int16& value, SQLLEN* len);
    void        BindParam(int32 idx, int32& value, SQLLEN* len);
    void        BindParam(int32 idx, int64& value, SQLLEN* len);
    void        BindParam(int32 idx, uint64& value, SQLLEN* len);
    void        BindParam(int32 idx, WCHAR* str, SQLLEN* len);
    void        BindParam(int32 idx, BYTE* bin, int32 size, SQLLEN* len);

    // Output columns
    void        BindCol(int32 idx, bool& value, SQLLEN* len);
    void        BindCol(int32 idx, float& value, SQLLEN* len);
    void        BindCol(int32 idx, double& value, SQLLEN* len);
    void        BindCol(int32 idx, int8& value, SQLLEN* len);
    void        BindCol(int32 idx, int16& value, SQLLEN* len);
    void        BindCol(int32 idx, int32& value, SQLLEN* len);
    void        BindCol(int32 idx, int64& value, SQLLEN* len);
    void        BindCol(int32 idx, uint64& value, SQLLEN* len);
    void        BindCol(int32 idx, WCHAR* str, int32 size, SQLLEN* len);
    void        BindCol(int32 idx, BYTE* bin, int32 size, SQLLEN* len);

private:
    SQLHENV     _env = SQL_NULL_HANDLE;
    SQLHDBC     _connection = SQL_NULL_HANDLE;
    SQLHSTMT    _statement = SQL_NULL_HANDLE;
};
#pragma once
#include <sstream>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

namespace MemoryUtils
{
    inline uint32 AlignUp(uint32 size, uint32 align)
    {
        return (size + align - 1) & ~(align - 1);
    }
}

class Utils
{
public:
    static wstring ToWString(const string& str)
    {
        if (str.empty()) return L"";

        int32 len  = static_cast<int32>(str.length());
        int32 wlen = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), len, NULL, 0);
        wstring wstr(wlen, 0);
        ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), len, &wstr[0], wlen);
        return wstr;
    }

    static string ToString(const wstring& wstr)
    {
        if (wstr.empty()) return "";

        int32 len  = static_cast<int32>(wstr.length());
        int32 alen = ::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), len, NULL, 0, NULL, NULL);
        string str(alen, 0);
        ::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), len, &str[0], alen, NULL, NULL);
        return str;
    }
};

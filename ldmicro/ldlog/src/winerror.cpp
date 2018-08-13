/*******************************************************************************
*  file    : winerror.cpp
*  created : 08.08.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#include "winerror.hpp"
#include "fmt/format.h"
#include <windows.h>
#include <tchar.h>

namespace ldlog
{

std::string strwinerr()
{
    DWORD dw = GetLastError();
    LPSTR lpMsgBuf;

    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
        (LPSTR) &lpMsgBuf,
        0, NULL );

    lpMsgBuf[strcspn(lpMsgBuf,"\r\n")] = 0;
    std::string res;
    try {
        res = fmt::format("{}[#{}]", lpMsgBuf, dw);
    } catch (...) {
        res = "Internal error. Invalid format.";
    }

    LocalFree(lpMsgBuf);
    return res;
}

LDLOG_API std::wstring wstrwinerr()
{
    LPWSTR lpMsgBuf;
    DWORD dw = GetLastError();

    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR) &lpMsgBuf,
        0, NULL );

    lpMsgBuf[wcscspn(lpMsgBuf,L"\r\n")] = 0;
    std::wstring res;
    try {
        res = fmt::format(L"{}[#{}]", lpMsgBuf, dw);
    } catch (...) {
        res = L"Internal error. Invalid format.";
    }

    return  res;
}

} //namespace ldlog

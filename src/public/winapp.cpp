#ifdef _WIN32

#include <string>
#include <Windows.h>

#include "unicode.hpp"

std::string getWinapiErrorMessage() {
    wchar_t* errorMsg;
    ::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorMsg,
        0, NULL);
    std::string finalMsg{ narrow(errorMsg) };
    ::LocalFree(errorMsg);
    return finalMsg;
}

#else
#error
#endif

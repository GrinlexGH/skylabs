#ifdef PLATFORM_WINDOWS
    #include "platform.hpp"
    #include "unicode.hpp"
    #include <Windows.h>
    #include <string>

PLATFORM_CLASS std::string getWinapiErrorMessage() {
    wchar_t* errorMsg = nullptr;
    ::FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        errorMsg, 0, nullptr
    );
    std::string finalMsg = narrow(errorMsg);
    ::LocalFree(errorMsg);
    return finalMsg;
}

#else
    #error
#endif

#include "dll_export.hpp"
#include "launcher.hpp"

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <nowide/convert.hpp>
#include <format>
#include <iostream>

namespace {
std::string GetLastErrorMessage() {
    wchar_t* errorMsg = nullptr;
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&errorMsg), 0,
        nullptr
    );

    std::string finalMsg { nowide::narrow(errorMsg) };

    LocalFree(errorMsg);

    return finalMsg;
}

void EnableAnsiEscapeSequences() {
    const HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (stdoutHandle == INVALID_HANDLE_VALUE) {
        std::cout << std::format("Invalid handle: {}\n", GetLastErrorMessage());
        return;
    }

    DWORD mode = 0;
    if (!GetConsoleMode(stdoutHandle, &mode)) {
        std::cout << std::format("Failed to get console mode: {}\n", GetLastErrorMessage());
        return;
    }

    const DWORD desiredFlags = ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if ((mode & desiredFlags) == desiredFlags) {
        return;
    }

    mode |= desiredFlags;
    if (!SetConsoleMode(stdoutHandle, mode)) {
        std::cout << std::format("Failed to set console mode: {}\n", GetLastErrorMessage());
        return;
    }
}
}
#endif

extern "C" DLL_EXPORT int CoreMain(const int /*argc*/, char* /*argv*/[]) {
#ifdef PLATFORM_WINDOWS
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    EnableAnsiEscapeSequences();
#endif

    CLauncher launcher;
    launcher.Run();

    return 0;
}

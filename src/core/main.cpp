#include "dll_export.hpp"
#include "launcher.hpp"

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <nowide/convert.hpp>
#include <format>
#include <iostream>

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
#endif

extern "C" DLL_EXPORT int CoreMain(const int /*argc*/, char* /*argv*/[]) {
#ifdef PLATFORM_WINDOWS
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    // Making allow ansi escape
    DWORD mode = 0;
    const HANDLE cmdOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleMode(cmdOutputHandle, &mode)) { std::cout << std::format("Cant get console mode! {}", GetLastErrorMessage()); };
    mode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(cmdOutputHandle, mode)) { std::cout << std::format("Cant set console mode! {}", GetLastErrorMessage()); };
#endif

    CLauncher launcher;
    launcher.Run();

    return 0;
}

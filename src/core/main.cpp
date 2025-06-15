#include "command_line.hpp"
#include "launcher.hpp"
#include "dll_export.hpp"
#include "logging.hpp"

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <cstdio>
#include <iostream>

#include <stc.hpp>

namespace {
BOOL WINAPI CtrlHandler(DWORD /*fdwCtrlType*/) {
    return FALSE;
}

void SetupConsole() {
    FreeConsole();
    AllocConsole();

    std::FILE* dummy;
    if (freopen_s(&dummy, "CONOUT$", "w", stdout)) { OutputDebugStringW(L"Cannot open CONOUT$ for write!"); }
    if (freopen_s(&dummy, "CONOUT$", "w", stderr)) { OutputDebugStringW(L"Cannot open CONOUT$ for write!"); }
    if (freopen_s(&dummy, "CONIN$", "r", stdin)) { OutputDebugStringW(L"Cannot open CONIN$ for read!"); }
    std::cout.clear();
    std::clog.clear();
    std::cerr.clear();
    std::cin.clear();

    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    const HANDLE cmdOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(cmdOutputHandle, &mode)) {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT;
        SetConsoleMode(cmdOutputHandle, mode);
    }

    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    std::cout << stc::true_color;
}
}
#endif

extern "C" DLL_EXPORT int CoreMain(const int /*argc*/, char* /*argv*/[]) {
#ifdef PLATFORM_WINDOWS
    SetupConsole();
#endif

    CLauncher launcher;
    launcher.Run();

    Log::Info("Press enter to exit.");
    std::cin.ignore();

    return 0;
}

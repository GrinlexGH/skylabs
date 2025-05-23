#include "command_line.hpp"
#include "launcher.hpp"
#include "dll_export.hpp"
#include "logging.hpp"
#include "os.hpp"

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <cstdio>
#include <iostream>

#include "stc.hpp"

namespace {
BOOL CtrlHandler(DWORD /*fdwCtrlType*/) {
    return FALSE;
}

void SetupConsole() {
    AllocConsole();
    std::FILE* dummy;
    if (freopen_s(&dummy, "CONOUT$", "w", stdout)) { OutputDebugStringA("Cannot open CONOUT$ for write!"); }
    if (freopen_s(&dummy, "CONOUT$", "w", stderr)) { OutputDebugStringA("Cannot open CONOUT$ for write!"); }
    if (freopen_s(&dummy, "CONIN$", "r", stdin)) { OutputDebugStringA("Cannot open CONIN$ for read!"); }
    std::cout.clear();
    std::clog.clear();
    std::cerr.clear();
    std::cin.clear();

    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    // Making allow ansi escape
    DWORD mode = 0;
    const HANDLE cmdOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(cmdOutputHandle, &mode);
    mode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(cmdOutputHandle, mode);

    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    std::cout << stc::true_color;
}
}
#endif

extern "C" DLL_EXPORT int CoreMain(const int argc, char* argv[]) {
    CommandLine::ParseArguments(argc, argv);

#ifdef PLATFORM_WINDOWS
    SetupConsole();
#endif

    CLauncher launcher;
    launcher.Run();

    Log::Info("Press any key to exit.");
    OS::WaitAnyKey();

    return 0;
}

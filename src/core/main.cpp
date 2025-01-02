#include "commandline.hpp"
#include "launcher.hpp"
#include "publicapi.hpp"
#include "console.hpp"
#include "SDL/SDL.hpp"

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <cstdio>
#include <iostream>

#include <stc.hpp>

BOOL CtrlHandler([[maybe_unused]] DWORD fdwCtrlType) {
    return FALSE;
}

namespace
{
void SetupConsole() {
    AllocConsole();
    std::FILE* dummy;
    if (freopen_s(&dummy, "CONOUT$", "w", stdout)) {}
    if (freopen_s(&dummy, "CONOUT$", "w", stderr)) {}
    if (freopen_s(&dummy, "CONIN$", "r", stdin)) {}
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

DLL_EXPORT int CoreInit(const int argc, char* argv[]) {
    CommandLine()->CreateCmdLine(
        std::vector<std::string>(argv, argv + argc)
    );

#ifdef PLATFORM_WINDOWS
    SetupConsole();
#endif

    CLauncher launcher;
    launcher.Run();

    Msg << "Press Enter to exit.";
    std::cin.ignore();

    return 0;
}

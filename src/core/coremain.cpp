#include "commandline.hpp"
#include "launcher.hpp"
#include "platform.hpp"
#include "console.hpp"
#include "unicode.hpp"
#include "SDL/SDL.hpp"

#include <vector>

#ifdef PLATFORM_WINDOWS
    #include <windows.h>
    #include <stdio.h>
    #include <iostream>

    #include "stc.hpp"

BOOL CtrlHandler(DWORD /*fdwCtrlType*/) {
    return FALSE;
}

static void SetupConsole() {
    FILE* dummy;
    ::AllocConsole();
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);
    freopen_s(&dummy, "CONIN$", "r", stdin);
    std::cout.clear();
    std::clog.clear();
    std::cerr.clear();
    std::cin.clear();

    ::SetConsoleCP(CP_UTF8);
    ::SetConsoleOutputCP(CP_UTF8);

    // Making allow ansi escape
    DWORD dwMode = 0;
    HANDLE cmdOutputHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    ::GetConsoleMode(cmdOutputHandle, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    ::SetConsoleMode(cmdOutputHandle, dwMode);
    ::SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);

    std::cout << stc::true_color;
}

DLL_EXPORT int CoreInit(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int /*nShowCmd*/) {
#else
DLL_EXPORT int CoreInit(int argc, char** argv) {
#endif
#ifdef PLATFORM_WINDOWS
    {
        int argc = 0;
        wchar_t** wcharArgList = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
        std::vector<std::string> charArgList(argc);
        for (int i = 0; i < argc; ++i) {
            charArgList[i] = narrow(wcharArgList[i]);
        }
        CommandLine()->CreateCmdLine(charArgList);
        LocalFree(wcharArgList);
    }

    SetupConsole();
#else
    CommandLine()->CreateCmdLine(
        std::vector<std::string>(argv, argv + argc)
    );
#endif
    CLauncher launcher;
    launcher.Run();
    return 0;
}

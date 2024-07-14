#include <cstdio>
#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>

#include "commandline.hpp"
#include "launcher.hpp"
#include "platform.hpp"
#include "console.hpp"
#include "unicode.hpp"
#include "stc.hpp"

#ifdef PLATFORM_WINDOWS
#include <Windows.h>

void InitConsole() {
    FILE *fDummy;
    ::AllocConsole();
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
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

    std::cout << stc::true_color;
}

DLL_EXPORT int CoreInit(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                        LPWSTR lpCmdLine, int nShowCmd) {
#else
DLL_EXPORT int CoreInit(int argc, char **argv) {
#endif
    try {
#ifdef PLATFORM_WINDOWS
        UNUSED(hInstance);
        UNUSED(hPrevInstance);
        UNUSED(lpCmdLine);
        UNUSED(nShowCmd);
        {
            int argc;
            wchar_t **wchar_arg_list{
                ::CommandLineToArgvW(::GetCommandLineW(), &argc) };

            std::vector<std::string> char_arg_list(argc);
            for (int i = 0; i < argc; ++i) {
                char_arg_list[i] = narrow(wchar_arg_list[i]);
            }

            CommandLine()->CreateCmdLine(char_arg_list);
            LocalFree(wchar_arg_list);
        }

        if (CommandLine()->FindParam("-console")) {
            InitConsole();
        }
#else
        CommandLine()->CreateCmdLine(std::vector<std::string>(argv, argv + argc));
#endif
        CLauncher launcher;
        Warning << "Warning! пошёл нахуй короче хуй" << std::endl;
        launcher.Run();
        return 0;
    } catch (const std::exception &e) {
        //todo: get rid of ifdefs
#ifdef PLATFORM_WINDOWS
        ::MessageBoxW(nullptr, widen(e.what()).c_str(), L"Error!",
                      MB_OK | MB_ICONERROR);
#else
        Error << e.what() << "!\n\n";
#endif
        return 1;
    }
}

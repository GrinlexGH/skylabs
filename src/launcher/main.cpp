#ifdef _WIN32
#include <Windows.h>
#elif defined(__linux__)
#include <dlfcn.h>
#endif

#include <filesystem>
#include <iostream>
#include <string>
#include <cassert>
#include <cstdlib>
#include <vector>
#include "unicode.hpp"
#include "baseapplication.hpp"
#include "console.hpp"

#ifdef WIN32
using CoreMain_t = int(*)(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd);
#elif defined(__linux__)
using CoreMain_t = int(*)(int argc, char** argv);
#else
#error
#endif

#ifdef _WIN32

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    try {
        {
            wchar_t** wchar_arg_list;
            int arg_count;
            wchar_arg_list = CommandLineToArgvW(GetCommandLine(), &arg_count);
            if (wchar_arg_list == NULL)
                throw std::runtime_error("Unable to get argv!");

            std::vector<std::string> arg_list(arg_count);
            for (int i = 0; i < arg_count; ++i) {
                arg_list[i] = Utf16ToUtf8(wchar_arg_list[i]);
            }
            LocalFree(wchar_arg_list);
            CConsole::SetArgs(arg_count, arg_list);
        }

        if (CConsole::CheckParam("-debug")) {
            CBaseApplication::switchDebugMode();
        }
        CBaseApplication::Init();
        CBaseApplication::AddLibSearchPath(CBaseApplication::rootDir.string() + "\\bin");
        std::string corePath = CBaseApplication::rootDir.parent_path().string() + "\\bin\\core.dll";
        void* core = CBaseApplication::LoadLib(corePath);
        auto main = (CoreMain_t)(void*)GetProcAddress((HINSTANCE)core, "CoreInit");
        int ret = main(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
        FreeLibrary((HMODULE)core);
        CConsole::Destroy();
        return ret;
    }
    catch (const std::exception& e) {
        MessageBox(nullptr, Utf8ToUtf16(e.what()).c_str(), L"Error!", MB_OK | MB_ICONERROR);
        return 1;
    }
}

#elif defined(__linux__)
int main(int argc, char** argv) {
    try {
        CConsole::SetArgs(argc, argv);
        if (CConsole::CheckParam("-debug")) {
            CBaseApplication::switchDebugMode();
            if(!getenv("TERM")) {
                std::string path = "xterm -e ";
                path += argv[0];
                std::system(path.c_str());
            }
        }
        CBaseApplication::Init();
        CBaseApplication::AddLibSearchPath(u8"/bin");
        std::u8string corePath = CBaseApplication::rootDir.parent_path().u8string() + u8"/bin/libcore.so";
        void* core = CBaseApplication::LoadLib(corePath);
        auto main = (CoreMain_t)dlsym(core, "CoreInit");
        if (!main) {
            CConsole::PrintLn("Failed to load the launcher entry proc\n");
            return 0;
        }
        int ret = main(argc, argv);
        dlclose(core);
        return ret;
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
}
#endif


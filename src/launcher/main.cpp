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
            wchar_t** wcharArgList;
            int argCount;
            wcharArgList = CommandLineToArgvW(GetCommandLine(), &argCount);
            if (wcharArgList == nullptr) {
                throw std::runtime_error("Unable to parse command line!");
            }
            std::vector<std::string> argList(argCount);
            argList.reserve(argCount);
            for (int i = 0; i < argCount; ++i) {
                argList[i] = utf16_to_utf8(wcharArgList[i]);
            }
            LocalFree(wcharArgList);
            CConsole::SetArgs(argCount, argList);
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
        MessageBox(nullptr, utf8_to_utf16(e.what()).c_str(), L"Error!", MB_OK | MB_ICONERROR);
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


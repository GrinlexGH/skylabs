#ifdef _WIN32
#include <Windows.h>

#elif defined(__linux__)
#include <dlfcn.h>

#endif

#include <filesystem>
#include <iostream>
#include <string>
#include "charconverters.hpp"
#include "baseapplication.hpp"
#include "exceptions.hpp"

#ifdef WIN32
typedef int (*CoreMain_t)(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow);
#elif defined(__linux__)
typedef int (*CoreMain_t)(int argc, char** argv);
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
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(nShowCmd);

    try {
        BaseApplication::Init();
        BaseApplication::AddLibSearchPath(BaseApplication::rootDir.u8string() + u8"/bin");
        std::u8string corePath = BaseApplication::rootDir.parent_path().u8string() + u8"/bin/core.dll";
        void* core = BaseApplication::LoadLib(corePath);
        UNUSED(core);
        return 0;
    }
    catch (const std::exception& e) {
        MessageBox(NULL, CharConverters::UTF8ToWideStr<std::string>(std::string(e.what())).c_str(), L"", MB_OK);
        return 1;
    }
}

#elif defined(__linux__)
int main(int argc, char** argv) {
    UNUSED(argv);
    UNUSED(argc);
    try {
        BaseApplication::Init();
        BaseApplication::AddLibSearchPath(u8"/bin");
        std::u8string corePath = BaseApplication::rootDir.parent_path().u8string() + u8"/bin/libcore.so";
        void* core = BaseApplication::LoadLib(corePath);
        while(1);
        dlclose(core);
        return 0;
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
}
#endif


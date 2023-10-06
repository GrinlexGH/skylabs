#ifdef _WIN32
#include <Windows.h>

#elif defined(__linux__)
#include <dlfcn.h>

#endif

#include <filesystem>
#include <iostream>
#include <string>
#include <assert.h>
#include "charconverters.hpp"
#include "baseapplication.hpp"
#include "exceptions.hpp"

#ifdef WIN32
typedef int (*CoreMain_t)(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine, int nShowCmd);
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
    try {
        BaseApplication::Init();
        BaseApplication::AddLibSearchPath(BaseApplication::rootDir.u8string() + u8"\\bin");
        std::u8string corePath = BaseApplication::rootDir.parent_path().u8string() + u8"\\bin\\core.dll";
        void* core = BaseApplication::LoadLib(corePath);
        CoreMain_t main = (CoreMain_t)GetProcAddress((HINSTANCE)core, "CoreInit");
        
        int ret = main(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
        FreeLibrary((HMODULE)core);
        return ret;
    }
    catch (const std::exception& e) {
        MessageBox(NULL, CharConverters::UTF8ToWideStr<std::string>(std::string(e.what())).c_str(), L"Error!", MB_OK | MB_ICONERROR);
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
        dlclose(core);
        return 0;
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
}
#endif


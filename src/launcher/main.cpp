#include <filesystem>
#include <iostream>
#include "charconverters.hpp"
#include "baseapplication.hpp"
#include "exceptions.hpp"

#ifdef _WIN32
#include <Windows.h>

#ifdef WIN32
typedef int (*CoreMain_t)(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow);
#elif POSIX
typedef int (*CoreMain_t)(int argc, char** argv);
#else
#error
#endif

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
        BaseApplication::AddLibSearchPath(BaseApplication::rootDir.string() + "/bin");
        std::wstring corePath = BaseApplication::rootDir.wstring() + L"/bin/core.dll";

        HINSTANCE core = LoadLibrary(corePath.c_str());
        //return 0;
    }
    catch (const std::exception& e) {
        MessageBox(NULL, CharConverters::UTF8ToWideStr<std::string>(std::string(e.what())).c_str(), L"", MB_OK);
        std::cout << e.what() << std::endl;
        return 1;
    }
}

#elif defined(__linux__)
int main(int argc, char** argv) {
    UNUSED(argv);
    UNUSED(argc);
    try {
        BaseApplication::Init();
        BaseApplication::AddLibSearchPath("/bin");
        //std::wstring corePath = BaseApplication::rootDir.wstring() + L"/bin/core.dll";
        //HINSTANCE core = LoadLibrary(corePath.c_str());
        //return 0;
        std::cout << getenv("LD_LIBRARY_PATH");
        throw localized_exception(u8"тебе пизда");
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
}
#endif


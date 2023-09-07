#ifdef _WIN32

#include <Windows.h>
#include <filesystem>
#include <iostream>
#include "charconverters.hpp"
#include "baseapplication.hpp"
#include "exceptions.hpp"

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
        BaseApplication::AddToEnvPATH(BaseApplication::rootDir.string() + "/bin");
#pragma warning(disable: 4996)
        MessageBox(NULL, _wgetenv(L"PATH"), L"", MB_OK);
        return 0;
    }
    catch (const localized_exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
}

#endif


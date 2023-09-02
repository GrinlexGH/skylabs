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
        throw localized_exception("");
        //return 0;
    }
    catch (const localized_exception& e) {
        std::cout << e.what() << std::endl;
        //MessageBox(NULL, CharConverters::UTF8ToWideStr(e.what()).c_str(), L"", MB_OK);
        //OutputDebugString(CharConverters::UTF8ToWideStr(e.what()).c_str());
        return 1;
    }
}

#endif


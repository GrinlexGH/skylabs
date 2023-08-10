#ifdef _WIN32

#include "appframework/appframework.hpp"
#include "tier0/exception.hpp"
#include <Windows.h>

int LauncherMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);
    UNREFERENCED_PARAMETER(hInstance);
    /*CApplication::SetInstance(hInstance);
    try {
        CApplication::Run();
    }
    catch (CException &ex) {
        MessageBox(NULL, L"ERROR", ex.what().c_str(), 1);
    }*/
    return 0;
}

#endif


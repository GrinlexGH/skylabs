//#include "macros.hpp"
#include <Windows.h>
#include <stdexcept>

extern "C" __declspec(dllexport) int CoreInit(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) {
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(nShowCmd);
    if (hInstance != NULL)
        throw std::runtime_error("dsds");
    else
        return 3;
}


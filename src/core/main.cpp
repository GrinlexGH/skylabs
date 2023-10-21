#include "macros.hpp"
#include <SDL3/SDL.h>

#ifdef _WIN32
#include <Windows.h>
DllExport int CoreInit(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) {
    UNUSED(hInstance);
    UNUSED(hPrevInstance);
    UNUSED(lpCmdLine);
    UNUSED(nShowCmd);
    return 14;
}
#else
DllExport int CoreInit(int argc, char** argv) {
    UNUSED(argc);
    UNUSED(argv);
    return 15;
}
#endif


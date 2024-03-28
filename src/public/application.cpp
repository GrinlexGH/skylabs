#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif
#include <cstring>
#include <filesystem>
#include <bit>
#include "unicode.hpp"
#include "application.hpp"

std::filesystem::path Application::rootDir;
bool Application::debugMode = false;

void Application::switchDebugMode() {
#ifdef WIN32
    if (!debugMode) {
        if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
            AllocConsole();
            freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        }
        else {
            freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        }
    }
    else {
        FreeConsole();
    }
#endif
    debugMode = !debugMode;
}

bool Application::isDebugMode() {
    return debugMode;
}


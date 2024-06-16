#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif
#include "application.hpp"
#include "unicode.hpp"
#include <bit>
#include <cstring>
#include <filesystem>

std::filesystem::path Application::rootDir;
bool Application::debugMode = false;

void Application::switchDebugMode() {
#ifdef _WIN32
  if (!debugMode) {
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
      AllocConsole();
      freopen_s((FILE **)stdout, "CONOUT$", "w", stdout);
    } else {
      freopen_s((FILE **)stdout, "CONOUT$", "w", stdout);
    }
  } else {
    FreeConsole();
  }
#endif
  debugMode = !debugMode;
}

bool Application::isDebugMode() { return debugMode; }

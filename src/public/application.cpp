#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif
#include "application.hpp"
#include <bit>
#include <cstring>
#include <filesystem>

std::filesystem::path Application::rootDir;
bool Application::debugMode = false;

bool Application::isDebugMode() { return debugMode; }

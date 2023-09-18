#include <cstring>
#include <filesystem>
#include <system_error>
#include "macros.hpp"
#include "exceptions.hpp"
#include "charconverters.hpp"
#include "baseapplication.hpp"

std::filesystem::path BaseApplication::rootDir;

void BaseApplication::Init() {
    rootDir = std::filesystem::current_path();
}

void BaseApplication::AddLibSearchPath(const std::string_view path) {
    if (path.empty())
        return;

#ifdef _WIN32
    size_t currentPathLen;
    // Getting length of PATH
    getenv_s(&currentPathLen, nullptr, 0, "PATH");
    if (currentPathLen == 0) {
        throw current_func_exception("getenv_s() failed\n\nCannot find PATH");
    }
    auto currentPath = std::make_unique<wchar_t[]>(currentPathLen);
    if (errno_t err = _wgetenv_s(&currentPathLen, currentPath.get(), currentPathLen, L"PATH")) {
        throw current_func_exception("_wgetenv_s() failed\n\nerror code: " + err);
    }
    std::wstring newPath = currentPath.get();
    newPath += L";" + CharConverters::UTF8ToWideStr(path) + L";";
    if (errno_t err = _wputenv_s(L"PATH", newPath.c_str())) {
        throw current_func_exception("_wputenv_s() failed\n\nerror code: " + err);
    }
#else
    std::string newPath;

    if(const char* currentLibEnv = getenv("LD_LIBRARY_PATH")) {
        newPath = currentLibEnv + ':';
    } else {
        newPath = path;
    }

    setenv("LD_LIBRARY_PATH", newPath.c_str(), 1);
#endif
}

void BaseApplication::LoadLib(const std::string_view path) {
    UNUSED(path);
}


#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif
#include <cstring>
#include <filesystem>
#include <bit>
#include "macros.hpp"
#include "exceptions.hpp"
#include "charconverters.hpp"
#include "baseapplication.hpp"

std::filesystem::path BaseApplication::rootDir;

void BaseApplication::Init() {
#ifdef _WIN32
    wchar_t buffer[MAX_PATH];

    if (GetModuleFileName(nullptr, buffer, MAX_PATH) == MAX_PATH) {
        wchar_t* errorMsg;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorMsg, 0, nullptr);
        throw func_exception(errorMsg);
    }

    rootDir = buffer;
#else
    rootDir = std::filesystem::canonical("/proc/self/exe");
#endif
}

void BaseApplication::AddLibSearchPath(const std::u8string_view path) {
    if (path.empty())
        return;

#ifdef _WIN32
    size_t currentPathLen;
    std::wstring newPath;
    // Getting length of PATH
    getenv_s(&currentPathLen, nullptr, 0, "PATH");
    if (currentPathLen != 0) {
        auto currentPath = std::make_unique<wchar_t[]>(currentPathLen);
        if (errno_t err = _wgetenv_s(&currentPathLen, currentPath.get(), currentPathLen, L"PATH")) {
            throw func_exception("_wgetenv_s() failed with code: " + std::to_string(err));
        }
        newPath = currentPath.get();
    }
    newPath += L";" + CharConverters::UTF8ToWideStr(path) + L";";
    if (errno_t err = _wputenv_s(L"PATH", newPath.c_str())) {
        throw func_exception("_wputenv_s() failed with code: " + std::to_string(err));
    }
#else
    std::string newPath;

    if(const char* currentLibEnv = getenv("LD_LIBRARY_PATH")) {
        newPath = currentLibEnv + ':';
        newPath += std::bit_cast<const char*>(path.data());
    } else {
        newPath = std::bit_cast<const char*>(path.data());
    }

    setenv("LD_LIBRARY_PATH", newPath.c_str(), 1);
#endif
}

void* BaseApplication::LoadLib(const std::u8string_view path) {
#ifdef _WIN32
    if (path.find('/') != std::string::npos)
        throw func_exception("Don't use '/' in path on windows.");

    void* lib = LoadLibraryEx(CharConverters::UTF8ToWideStr<std::u8string>(std::u8string(path.data())).c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);

    if (!lib) {
        wchar_t* errorMsg;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorMsg, 0, nullptr);
        throw func_exception(errorMsg);
    }

    return lib;
#else
    void* lib = dlopen(std::bit_cast<const char*>(path.data()), RTLD_NOW);

    if(!lib) {
        throw func_exception(std::string("failed open library:\n\n") + dlerror());
    }

    return lib;
#endif
}


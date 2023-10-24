#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif
#include <cstring>
#include <filesystem>
#include <bit>
#include "macros.hpp"
#include "console.hpp"
#include "exceptions.hpp"
#include "charconverters.hpp"
#include "baseapplication.hpp"

std::filesystem::path CBaseApplication::rootDir;
bool CBaseApplication::debugMode = false;

void CBaseApplication::Init() {
    CConsole::PrintLn("Initializing CBaseApplication...");
#ifdef _WIN32
    if (CBaseApplication::isDebugMode()) {
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
        CConsole::PrintLn("Console code page: %d", CP_UTF8);
    }
    wchar_t buffer[MAX_PATH];

    if (GetModuleFileName(nullptr, buffer, MAX_PATH) == MAX_PATH) {
        wchar_t* errorMsg;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorMsg, 0, nullptr);
        throw CCurrentFuncExcept(errorMsg);
    }

    rootDir = buffer;
    CConsole::PrintLn("rootDir == %s", rootDir.string().c_str());
#else
    rootDir = std::filesystem::canonical("/proc/self/exe");
    CConsole::PrintLn("rootDir == %s", rootDir.string().c_str());
#endif
    CConsole::PrintLn("Initializing finished.\n");
}

void CBaseApplication::AddLibSearchPath(const std::u8string_view path) {
    CConsole::PrintLn("Adding library serach path...");
    if (path.empty()) {
        CConsole::PrintLn("Library search not path added.\n");
        return;
    }
    CConsole::PrintLn(u8"Path to add: %s", path.data());
#ifdef _WIN32
    size_t currentPathLen;
    std::wstring newPath;
    // Getting length of PATH
    getenv_s(&currentPathLen, nullptr, 0, "PATH");
    if (currentPathLen != 0) {
        auto currentPath = std::make_unique<wchar_t[]>(currentPathLen);
        if (errno_t err = _wgetenv_s(&currentPathLen, currentPath.get(), currentPathLen, L"PATH")) {
            throw CCurrentFuncExcept("_wgetenv_s() failed with code: " + std::to_string(err));
        }
        newPath = currentPath.get();
    }
    newPath += L";" + CharConverters::UTF8ToWideStr(path) + L";";
    if (errno_t err = _wputenv_s(L"PATH", newPath.c_str())) {
        throw CCurrentFuncExcept("_wputenv_s() failed with code: " + std::to_string(err));
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
    CConsole::PrintLn("Library search path added.\n");
}

void* CBaseApplication::LoadLib(const std::u8string_view path) {
    CConsole::PrintLn("Loading library...");
    CConsole::PrintLn(u8"library to add: %s", path.data());
#ifdef _WIN32
    if (path.find('/') != std::string::npos)
        throw CCurrentFuncExcept("Don't use '/' in path on windows.");

    void* lib = LoadLibraryEx(CharConverters::UTF8ToWideStr<std::u8string>(std::u8string(path.data())).c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);

    if (!lib) {
        wchar_t* errorMsg;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorMsg, 0, nullptr);
        throw CCurrentFuncExcept(errorMsg);
    }
    CConsole::PrintLn("Library loaded.\n");
    return lib;
#else
    void* lib = dlopen(std::bit_cast<const char*>(path.data()), RTLD_NOW);
    if(!lib) {
        throw CCurrentFuncExcept(std::string("failed open library:\n\n") + dlerror());
    }
    CConsole::PrintLn("\x1B[38;2;64;224;208mLibrary loaded.\n\x1B[0m");
    return lib;
#endif
}

void CBaseApplication::switchDebugMode() {
    debugMode = !debugMode;
}

bool CBaseApplication::isDebugMode() {
    return debugMode;
}


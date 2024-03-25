#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif
#include <cstring>
#include <filesystem>
#include <bit>
#include "console.hpp"
#include "unicode.hpp"
#include "baseapplication.hpp"

std::filesystem::path CBaseApplication::rootDir;
bool CBaseApplication::debugMode = false;

void CBaseApplication::Init() {
    CConsole::PrintLn("Initializing CBaseApplication...");
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    CConsole::PrintLn("Console code page: %d", CP_UTF8);

    wchar_t buffer[MAX_PATH];

    if (GetModuleFileName(nullptr, buffer, MAX_PATH) == MAX_PATH) {
        wchar_t* errorMsg;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorMsg, 0, nullptr);
        throw std::runtime_error(Utf16ToUtf8(errorMsg).c_str());
    }

    rootDir = buffer;
    rootDir.remove_filename();
    CConsole::PrintLn("rootDir == %s", rootDir.string().c_str());
#else
    rootDir = std::filesystem::canonical("/proc/self/exe");
    CConsole::PrintLn("rootDir == %s", rootDir.string().c_str());
#endif
    CConsole::PrintLn("Initializing finished.\n");
}

void CBaseApplication::AddLibSearchPath(const std::string_view path) {
    CConsole::PrintLn("Adding library serach path...");
    if (path.empty()) {
        CConsole::PrintLn("Library search not path added.\n");
        return;
    }
    CConsole::PrintLn("Path to add: %s", path.data());
#ifdef _WIN32
    size_t currentPathLen;
    std::wstring newPath;
    // Getting length of PATH
    getenv_s(&currentPathLen, nullptr, 0, "PATH");
    if (currentPathLen != 0) {
        wchar_t* currentPath = new wchar_t[currentPathLen];
        if (errno_t err = _wgetenv_s(&currentPathLen, currentPath, currentPathLen, L"PATH")) {
            delete[] currentPath;
            throw std::runtime_error("_wgetenv_s() failed with code: " + std::to_string(err));
        }
        newPath = currentPath;
        delete[] currentPath;
    }
    newPath += L";" + Utf8ToUtf16(path.data()) + L";";
    if (errno_t err = _wputenv_s(L"PATH", newPath.c_str())) {
        throw std::runtime_error("_wputenv_s() failed with code: " + std::to_string(err));
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

void* CBaseApplication::LoadLib(std::string path) {
    CConsole::PrintLn("Loading library...");
    CConsole::PrintLn("library to add: %s", path.data());
#ifdef _WIN32
    if (path.find('/') != std::string::npos) {
        CConsole::PrintLn("Don't use '/' in path on windows.");
        while (true) {
            size_t pos = path.find('/');
            if (pos == std::string::npos)
                break;
            path.replace(pos, 1, "\\");
        }
    }

    void* lib = LoadLibraryEx(Utf8ToUtf16(path.data()).c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);

    if (!lib) {
        wchar_t* errorMsg;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorMsg, 0, nullptr);
        throw std::runtime_error(Utf16ToUtf8(errorMsg));
    }
    CConsole::PrintLn("Library loaded.\n");
    return lib;
#else
    void* lib = dlopen(std::bit_cast<const char*>(path.data()), RTLD_NOW);
    if(!lib) {
        throw std::runtime_error(std::string("failed open library:\n\n") + dlerror());
    }
    CConsole::PrintLn("\x1B[38;2;64;224;208mLibrary loaded.\n\x1B[0m");
    return lib;
#endif
}

void CBaseApplication::switchDebugMode() {
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

bool CBaseApplication::isDebugMode() {
    return debugMode;
}


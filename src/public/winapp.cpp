#ifdef POSIX
#error
#else
#include <Windows.h>
#include "application.hpp"
//#include "console.hpp"
#include "unicode.hpp"

void Application::Init() {
    //console::Msg("Initializing application...\n");

    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    //console::Msg("Console code page: %d\n", CP_UTF8);

    wchar_t buffer[MAX_PATH];

    if (GetModuleFileName(nullptr, buffer, MAX_PATH) == MAX_PATH) {
        wchar_t* errorMsg;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorMsg, 0, nullptr);
        throw std::runtime_error(Utf16ToUtf8(errorMsg).c_str());
    }

    rootDir = buffer;
    rootDir.remove_filename();
    //console::Msg("rootDir == %s\n", rootDir.string().c_str());
    //console::Msg("Initializing finished.\n\n");
}

void Application::AddLibSearchPath(const std::string_view path) {
    //console::Msg("Adding library serach path...\n");
    if (path.empty()) {
        //console::Msg("Library search not path added.\n\n");
        return;
    }
    //console::Msg("Path to add: %s\n", path.data());

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
    //console::Msg("Library search path added.\n\n");
}

void* Application::LoadLib(std::string path) {
    //console::Msg("Loading library...\n");
    //console::Msg("library to add: %s\n", path.data());

    if (path.find('/') != std::string::npos) {
        //console::Msg("Don't use '/' in path on windows.\n");
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

    //console::Msg("Library loaded.\n\n");
    return lib;
}

/*void console::Destroy() {
    argv.clear();
    if (Application::isDebugMode()) {
        FreeConsole();
    }
}*/
#endif


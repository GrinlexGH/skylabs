#ifdef _WIN32
#include "console.hpp"
#include "utilities.hpp"
#include "unicode.hpp"
#include <stdexcept>
#include <string>
#include <Windows.h>

void AddLibSearchPath(const std::string_view path)
{
    Msg("Adding library serach path...\n");
    if (path.empty())
        return;
    Msg("Path to add: %s\n", path.data());

    size_t currentPathLen;
    std::wstring newPath;
    _wgetenv_s(&currentPathLen, NULL, 0, L"PATH");
    if (currentPathLen > 0) {
        wchar_t *currentPath = new wchar_t[currentPathLen];
        if (_wgetenv_s(&currentPathLen, currentPath, currentPathLen, L"PATH")) {
            delete[] currentPath;
            throw std::runtime_error("Failed to add lib search path: _wgetenv_s() failed.");
        }
        newPath = currentPath;
        delete[] currentPath;
        newPath += L';' + widen(path.data()) + L';';
    } else {
        newPath += widen(path.data()) + L";";
    }
    if (errno_t err = _wputenv_s(L"PATH", newPath.c_str()))
    {
        throw std::runtime_error("_wputenv_s() failed with code: " +
                                 std::to_string(err));
    }
    Msg("Library search path added.\n\n");
}

void *LoadLib(std::string path)
{
    Error << "HUI" << std::endl;
    Msg("Loading library...\n");
    Msg("library to add: %s\n", path.data());

    if (path.find('/') != std::string::npos)
    {
        Msg("Don't use '/' in path on windows.\n");
        while (true)
        {
            size_t pos = path.find('/');
            if (pos == std::string::npos)
                break;
            path.replace(pos, 1, "\\");
        }
    }

    void *lib = LoadLibraryEx(widen(path.data()).c_str(), nullptr,
                              LOAD_WITH_ALTERED_SEARCH_PATH);

    if (!lib)
    {
        wchar_t *errorMsg;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                          FORMAT_MESSAGE_IGNORE_INSERTS,
                      nullptr, GetLastError(),
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorMsg,
                      0, nullptr);
        throw std::runtime_error(narrow(errorMsg));
    }

    Msg("Library loaded.\n\n");
    return lib;
}
#else
#error
#endif

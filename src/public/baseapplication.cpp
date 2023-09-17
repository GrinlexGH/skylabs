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

void BaseApplication::AddToEnvPATH(const std::string_view path) {
    if (path.empty())
        return;

#ifdef _WIN32
    size_t currentPathLen;
    getenv_s( &currentPathLen, nullptr, 0, "PATH");
    if (currentPathLen == 0) {
        throw current_func_exception("getenv_s() failed\n\nCannot find PATH");
    }
    auto currentPath = std::make_unique<wchar_t[]>(currentPathLen);
    if (errno_t err = _wgetenv_s(&currentPathLen, currentPath.get(), currentPathLen, L"PATH")) {
        switch (err) {
        case EINVAL: throw current_func_exception("_wdupenv_s() failed\n\nEINVAL");
        case ERANGE: throw current_func_exception("_wdupenv_s() failed\n\nERANGE");
        }
    }
    std::wstring newPath = currentPath.get();
    newPath += L";" + CharConverters::UTF8ToWideStr(path) + L";";
    _wputenv_s(L"PATH", newPath.c_str());
#else
    if(char* currentPath = getenv("PATH") == nullptr);
        throw current_func_exception("failed to do getenv()\n\nCannot find PATH");
    std::string newPath = "PATH=" + currentPath + ";" + path;
    putenv(newPath.c_str());
#endif
}


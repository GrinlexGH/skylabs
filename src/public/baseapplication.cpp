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
        throw localized_exception(CurrentFunction + ": failed to do getenv_s()\n\nCannot find PATH");
    }
    auto currentPath = std::make_unique<wchar_t[]>(currentPathLen);
    if (errno_t err = _wgetenv_s(&currentPathLen, currentPath.get(), currentPathLen, L"PATH")) {
        switch (err) {
        case EINVAL: throw localized_exception(CurrentFunction + ": failed to do _wdupenv_s()\n\nEINVAL");
        case ENOMEM: throw localized_exception(CurrentFunction + ": failed to do _wdupenv_s()\n\nENOMEM");
        default:     throw std::system_error(
                        std::error_code(err, std::system_category()),
                        "BaseApplication::AddToEnvPATH: failed to do _wdupenv_s()!"
                     );
        }
    }
    std::wstring newPath = currentPath.get();
    newPath += L";" + CharConverters::UTF8ToWideStr(path) + L";";
    _wputenv_s(L"PATH", newPath.c_str());
#else
    if(char* currentPath = getenv("PATH") == nullptr);
        throw localized_exception(CurrentFunction + ": failed to do getenv()\n\nCannot find PATH var");
    std::string newPath = "PATH=" + currentPath + ";" + path;
    putenv(newPath.c_str());
#endif
}


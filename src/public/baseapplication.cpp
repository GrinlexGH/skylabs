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
    wchar_t* currentPath;
    size_t currentPathLen;
    if (errno_t err = _wdupenv_s(&currentPath, &currentPathLen, L"PATH")) {
        switch (err) {
        case EINVAL: throw localized_exception(CurrentFunction + ": failed to do _wdupenv_s()\n\nEINVAL");
        case ENOMEM: throw localized_exception(CurrentFunction + ": failed to do _wdupenv_s()\n\nENOMEM");
        default:     throw std::system_error(
                        std::error_code(err, std::system_category()),
                        "BaseApplication::AddToEnvPATH: failed to do _wdupenv_s()!"
                     );
        }
    }
    std::wstring newPath = currentPath;
    newPath += L";" + CharConverters::UTF8ToWideStr(path);
    _wputenv_s(L"PATH", newPath.c_str());
#else
    if(char* currentPath = getenv("PATH") == nullptr);
        throw localized_exception(CurrentFunction + ": failed to do getenv()\n\nCannot find PATH var");
    std::string newPath = "PATH=" + currentPath + ";" + path;
    putenv(newPath.cstr());
#endif


    /*
    if (envPathLen > 0) {
        std::string envPath(envPathLen, '\0');

        // Second call to actually retrieve the PATH value
        if (!getenv_s(&envPathLen, envPath.data(), envPathLen, "PATH")) {
            throw Exception("Failed to get PATH");
        }

        // Construct the new PATH value
        std::string newEnvPath = "PATH=";
        newEnvPath.append(envPath).append(path.empty() ? "" : ";").append(path);

        // Set the new PATH value
        if (!_putenv(newEnvPath.c_str())) {
            throw Exception("Failed to set PATH");
        }
    }
    else {
        throw Exception("Failed to find PATH");
    }*/
}


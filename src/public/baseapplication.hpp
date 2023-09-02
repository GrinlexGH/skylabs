#pragma once

#include <filesystem>
#include <string>

class BaseApplication
{
public:
    /**
    * @throws std::bad_alloc by std::filesystem::current_path(),
    *   if memory cannot be allocated.
    * @throws std::filesystem::filesystem_error by std::filesystem::current_path(),
    *   if underlying OS API errors occur and constructed with the OS error code as
    *   the error code argument.
    */
    static void Init();
    /**
    * Adds the given value to the PATH environment variable.
    * @param The absolute path to the folder.
    * @throws
    */
    static void AddToEnvPATH(const std::string_view path);
    static std::filesystem::path rootDir;
};


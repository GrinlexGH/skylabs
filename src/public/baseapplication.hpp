#pragma once

#include <filesystem>
#include <string>
#include "macros.hpp"

class BaseApplication
{
public:
    /**
    * @brief Initializes BaseApplication variables
    * @throws std::bad_alloc by std::filesystem::current_path(),
    *   if memory cannot be allocated.
    * @throws std::filesystem::filesystem_error by std::filesystem::current_path(),
    *   if underlying OS API errors occur and constructed with the OS error code as
    *   the error code argument.
    */
    static void Init();
    /**
    * @brief Adds the given value to the PATH environment variable.
    * @param path - The absolute path to the folder.
    * @throws current_func_exception if getenv_s() cannot find PATH.
    * @throws current_func_exception with error code if _wgetenv_s failed.
    */
    static void AddLibSearchPath(const std::u8string_view path);
    static void* LoadLib(const std::u8string_view path);
    static std::filesystem::path rootDir;
};


#pragma once

#include <filesystem>
#include <string>
#include "macros.hpp"

class CBaseApplication
{
    static bool debugMode;
public:
    /**
    * @brief Initializes BaseApplication variables
    * @throws func_exception with windows's description of error
    *   if GetModuleFileName failed
    * 
    * @throws std::bad_alloc by std::filesystem::canonical(),
    *   if memory cannot be allocated.
    * @throws std::filesystem::filesystem_error by
    *   std::filesystem::canonical(), in cases where there are
    *   underlying OS API errors, and it's instantiated with the
    *   OS error code as the error code argument.
    */
    static void Init();
    /**
    * @brief Adds the given value to the PATH environment variable.
    * @param path - The absolute path to the folder.
    * @throws current_func_exception with error code if _wgetenv_s failed.
    * 
    */
    static void AddLibSearchPath(const std::u8string_view path);
    static void* LoadLib(const std::u8string_view path);
    static void switchDebugMode();
    static bool isDebugMode();
    static std::filesystem::path rootDir;
};


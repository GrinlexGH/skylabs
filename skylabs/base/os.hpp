#pragma once
#include <filesystem>

#ifdef PLATFORM_WINDOWS
#include <cstdint>
#endif

#include "sk_base_export.h"

namespace sk::os {
SK_BASE_PUBLIC_CLASS std::string GetExecutableDirectory();

#ifdef PLATFORM_WINDOWS
SK_BASE_PUBLIC_CLASS std::string GetWindowsError(std::uint32_t errorCode);
#endif

template <typename... Args>
[[nodiscard]] std::string JoinPath(Args&&... args) {
    std::filesystem::path result;
    ((result /= std::forward<Args>(args)), ...);
    return result.string();
}
}

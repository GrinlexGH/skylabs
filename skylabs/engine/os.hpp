#pragma once
#include <filesystem>

#ifdef PLATFORM_WINDOWS
#include <cstdint>
#endif

namespace sk::os {
std::string GetAppRoot();

#ifdef PLATFORM_WINDOWS
std::string GetWindowsError(std::uint32_t errorCode);
#endif

template <typename... Args>
[[nodiscard]] std::string JoinPath(Args&&... args) {
    std::filesystem::path result;
    ((result /= std::forward<Args>(args)), ...);
    return result.string();
}
}

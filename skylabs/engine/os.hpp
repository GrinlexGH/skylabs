#pragma once
#include <array>
#include <chrono>
#include <ctime>
#include <filesystem>

#ifdef PLATFORM_WINDOWS
#include <cstdint>
#endif

namespace sk::os {
struct LocalTime {
    std::tm tm { };
    std::array<char, 6> tzOffset { "+0000" };
};

[[nodiscard]] LocalTime GetLocalTime(
    const std::chrono::system_clock::time_point tp = std::chrono::system_clock::now()) noexcept;

[[nodiscard]] std::string GetAppRoot();

#ifdef PLATFORM_WINDOWS
[[nodiscard]] std::string GetWindowsError(std::uint32_t errorCode);
#endif

template <typename... Args>
[[nodiscard]] std::string JoinPath(Args&&... args) {
    std::filesystem::path result;
    ((result /= std::forward<Args>(args)), ...);
    return result.string();
}
}

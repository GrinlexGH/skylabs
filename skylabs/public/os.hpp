#pragma once
#include <skylabs/public/pch.hpp>

namespace OS {
PUBLIC_CLASS std::string GetExecutableDirectory();

#ifdef PLATFORM_WINDOWS
PUBLIC_CLASS std::string GetWindowsError(DWORD errorCode);
#endif

template <typename... Args>
[[nodiscard]] std::string PathJoin(Args&&... args) {
    std::filesystem::path result;
    ((result /= std::forward<Args>(args)), ...);
    return result.string();
}
}

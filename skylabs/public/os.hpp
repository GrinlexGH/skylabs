#pragma once
#include <skylabs/public/pch.hpp>

namespace OS {
PUBLIC_CLASS std::string GetExecutableDirectory();

template <typename... Args>
[[nodiscard]] std::string PathJoin(Args&&... args) {
    std::filesystem::path result;
    ((result /= std::forward<Args>(args)), ...);
    return result.string();
}
}

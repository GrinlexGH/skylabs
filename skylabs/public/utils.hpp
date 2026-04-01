#pragma once
#include <skylabs/public/pch.hpp>

namespace Utils {
struct Extent2D {
    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;
};

struct StringHash {
    using is_transparent = void;

    [[nodiscard]] size_t operator()(const char* cc) const {
        return std::hash<std::string_view>{}(cc);
    }

    [[nodiscard]] size_t operator()(const std::string_view sv) const {
        return std::hash<std::string_view>{}(sv);
    }

    [[nodiscard]] size_t operator()(const std::string& s) const {
        return std::hash<std::string>{}(s);
    }
};

using StringUnorderedSet = std::unordered_set<std::string, StringHash, std::equal_to<>>;
}

#pragma once
#include <skylabs/public/pch.hpp>

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

using UnorderedStringSet =
    std::unordered_set<std::string, StringHash, std::equal_to<>>;

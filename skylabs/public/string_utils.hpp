#pragma once
#include <string_view>
#include <string>
#include <unordered_set>

struct StringHash {
    using is_transparent = void;

    size_t operator()(std::string_view sv) const noexcept {
        return std::hash<std::string_view>{}(sv);
    }

    size_t operator()(const std::string& s) const noexcept {
        return std::hash<std::string>{}(s);
    }
};

struct StringEq {
    using is_transparent = void;

    bool operator()(std::string_view a, std::string_view b) const noexcept {
        return a == b;
    }
};

using UnorderedStringSet = std::unordered_set<std::string, StringHash, StringEq>;

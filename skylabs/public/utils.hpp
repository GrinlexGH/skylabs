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

template <typename T>
class Range {
public:
    Range(T end) : m_start(0), m_end(end) {}
    Range(T start, T end) : m_start(start), m_end(end) {}

    struct iterator {
        T val;
        T operator*() const { return val; }
        iterator& operator++() { ++val; return *this; }
        bool operator!=(const iterator& other) const { return val != other.val; }
    };

    iterator begin() const { return { m_start }; }
    iterator end() const { return { m_end }; }

private:
    T m_start, m_end;
};
}

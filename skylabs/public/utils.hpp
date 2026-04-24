#pragma once
#include <skylabs/public/pch.hpp>

namespace Utils {
struct Extent2D {
    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;
};

template<std::integral T>
auto Range(T start, T end) {
    return boost::irange(start, end);
}

template<std::integral T>
auto Range(T end) {
    return Range(static_cast<T>(0), end);
}

template<std::integral T, std::integral U>
auto Range(T start, T end, U step = 1) {
    return boost::irange(start, end, step);
}
}

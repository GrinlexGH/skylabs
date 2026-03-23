#pragma once
#include <cstdint>
#include <compare>
#include <type_traits>

namespace Utils {
struct Extent2D {
    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;
};

enum class Requirement : std::uint8_t { eOptional, eRequired };
}

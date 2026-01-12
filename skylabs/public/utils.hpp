#pragma once
#include <cstdint>

namespace Utils {
struct CExtent2D {
    std::uint32_t width;
    std::uint32_t height;
};

enum class Requirement : std::uint8_t { eOptional, eRequired };
}

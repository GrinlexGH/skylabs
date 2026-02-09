#pragma once
#include <cstdint>
#include <type_traits>

namespace Utils {
struct Extent2D {
    std::uint32_t width;
    std::uint32_t height;
};

enum class Requirement : std::uint8_t { eOptional, eRequired };

template <typename BitType>
class Flags
{
public:
    using BitsType = BitType;
    using MaskType = typename std::underlying_type<BitType>::type;

    constexpr Flags() noexcept : m_mask(0) {}
    constexpr Flags(BitType bit) noexcept : m_mask(static_cast<MaskType>(bit)) {}
    constexpr Flags(Flags<BitType> const& rhs) noexcept = default;
    constexpr explicit Flags(MaskType flags) noexcept : m_mask(flags) {}

    auto operator<=>(Flags<BitType> const&) const = default;

    constexpr bool operator!() const noexcept { return !m_mask; }

    constexpr Flags<BitType> operator&(Flags<BitType> const& rhs) const noexcept { return Flags<BitType>(m_mask & rhs.m_mask); }
    constexpr Flags<BitType> operator|(Flags<BitType> const& rhs) const noexcept { return Flags<BitType>(m_mask | rhs.m_mask); }
    constexpr Flags<BitType> operator^(Flags<BitType> const& rhs) const noexcept { return Flags<BitType>(m_mask ^ rhs.m_mask); }

    constexpr Flags<BitType> & operator=(Flags<BitType> const& rhs) noexcept = default;
    constexpr Flags<BitType> & operator|=(Flags<BitType> const& rhs) noexcept { m_mask |= rhs.m_mask; return *this; }
    constexpr Flags<BitType> & operator&=(Flags<BitType> const& rhs) noexcept { m_mask &= rhs.m_mask; return *this; }
    constexpr Flags<BitType> & operator^=(Flags<BitType> const& rhs) noexcept { m_mask ^= rhs.m_mask; return *this; }

    explicit constexpr operator bool() const noexcept { return !!m_mask; }
    explicit constexpr operator MaskType() const noexcept { return m_mask; }

private:
    MaskType m_mask;
};
}

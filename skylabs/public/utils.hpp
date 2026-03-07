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

template <typename FlagBitsType>
struct FlagTraits
{
    static constexpr bool isBitmask = false;
};

template <typename BitType>
class Flags
{
public:
    using BitsType = BitType;
    using MaskType = std::underlying_type_t<BitType>;

    constexpr Flags() noexcept = default;
    constexpr explicit Flags(MaskType flags) noexcept : m_mask(flags) {}
    constexpr Flags(BitType bit) noexcept : m_mask(static_cast<MaskType>(bit)) {}
    constexpr Flags(Flags<BitType>&& rhs) noexcept = default;
    constexpr Flags(Flags<BitType> const& rhs) noexcept = default;
    constexpr ~Flags() = default;

    auto operator<=>(Flags<BitType> const&) const = default;

    constexpr bool operator!() const noexcept { return !m_mask; }

    constexpr Flags<BitType> operator~() const noexcept { return Flags<BitType>(m_mask ^ FlagTraits<BitType>::allFlags.m_mask); }

    constexpr Flags<BitType> operator&(Flags<BitType> const& rhs) const noexcept { return Flags<BitType>(m_mask & rhs.m_mask); }
    constexpr Flags<BitType> operator|(Flags<BitType> const& rhs) const noexcept { return Flags<BitType>(m_mask | rhs.m_mask); }
    constexpr Flags<BitType> operator^(Flags<BitType> const& rhs) const noexcept { return Flags<BitType>(m_mask ^ rhs.m_mask); }

    constexpr Flags<BitType>& operator=(Flags<BitType>&& rhs) noexcept = default;
    constexpr Flags<BitType>& operator=(Flags<BitType> const& rhs) noexcept = default;
    constexpr Flags<BitType>& operator|=(Flags<BitType> const& rhs) noexcept { m_mask |= rhs.m_mask; return *this; }
    constexpr Flags<BitType>& operator&=(Flags<BitType> const& rhs) noexcept { m_mask &= rhs.m_mask; return *this; }
    constexpr Flags<BitType>& operator^=(Flags<BitType> const& rhs) noexcept { m_mask ^= rhs.m_mask; return *this; }

    explicit constexpr operator bool() const noexcept { return !!m_mask; }
    explicit constexpr operator MaskType() const noexcept { return m_mask; }

private:
    MaskType m_mask = 0;
};
}

template <typename BitType>
constexpr Utils::Flags<BitType> operator&(BitType bit, Utils::Flags<BitType> const& flags) noexcept {
    return flags.operator&(bit);
}

template <typename BitType>
constexpr Utils::Flags<BitType> operator|(BitType bit, Utils::Flags<BitType> const& flags) noexcept {
    return flags.operator|(bit);
}

template <typename BitType>
constexpr Utils::Flags<BitType> operator^(BitType bit, Utils::Flags<BitType> const& flags) noexcept {
    return flags.operator^(bit);
}

template <typename BitType>
constexpr Utils::Flags<BitType> operator&(BitType lhs, BitType rhs) noexcept
requires Utils::FlagTraits<BitType>::isBitmask {
    return Utils::Flags<BitType>(lhs) & rhs;
}

template <typename BitType>
constexpr Utils::Flags<BitType> operator|(BitType lhs, BitType rhs) noexcept
requires Utils::FlagTraits<BitType>::isBitmask {
    return Utils::Flags<BitType>(lhs) | rhs;
}

template <typename BitType>
constexpr Utils::Flags<BitType> operator^(BitType lhs, BitType rhs) noexcept requires Utils::FlagTraits<BitType>::isBitmask {
    return Utils::Flags<BitType>(lhs) ^ rhs;
}

template <typename BitType>
constexpr Utils::Flags<BitType> operator~(BitType bit) noexcept requires Utils::FlagTraits<BitType>::isBitmask {
    return ~(Utils::Flags<BitType>(bit));
}

#pragma once

#include <format>
#include <cstdint>

struct CVersion
{
    std::uint32_t m_major = 0;
    std::uint32_t m_minor = 0;
    std::uint32_t m_patch = 0;
};

template <>
struct std::formatter<CVersion> : std::formatter<string_view> {
    auto format(const CVersion& version, std::format_context& ctx) const {
        std::string_view formatString;
        if (version.m_patch) {
            formatString = "{}.{}.{}";
        } else {
            formatString = "{}.{}";
        }

        std::string temp;
        std::format_to(std::back_inserter(temp), formatString,
            version.m_major,
            version.m_minor,
            version.m_patch
        );
        return std::formatter<string_view>::format(temp, ctx);
    }
};

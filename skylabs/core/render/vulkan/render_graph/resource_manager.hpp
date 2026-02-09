#pragma once
#include <skylabs/public/utils.hpp>
#include <variant>

namespace Vulkan::RG {
struct AbsoluteMemorySize
{
    std::uint32_t m_width  = 1;
    std::uint32_t m_height = 1;
    std::uint32_t m_depth  = 1;
};

struct RelativeMemorySize
{
    float m_scaleX = 1.0f;
    float m_scaleY = 1.0f;
    std::uint32_t m_depth = 1;
};

enum class TextureFormat : std::uint8_t
{
    eRGBA8888Srgb = 0,
    eDepthOptimal
};

enum class TextureUsageBits : std::uint8_t
{
    eAttachment = 1 << 0,
    eDepthAttachment = 1 << 1,
    eSampled = 1 << 2
};

using TextureUsage = Utils::Flags<TextureUsageBits>;

struct TextureDescirption
{
    using TextureExtent = std::variant<AbsoluteMemorySize, RelativeMemorySize>;

    TextureFormat m_format;
    TextureUsage m_usage;
    TextureExtent m_extent;
    bool m_sampled;
};

class CResourceManager
{

};
}

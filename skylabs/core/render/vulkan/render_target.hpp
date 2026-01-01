#pragma once
#include <skylabs/core/render/vulkan/memory/image.hpp>

namespace Vulkan {
enum class ImageUsage : std::uint8_t
{
    eShaderRead,
    eDepth,
    eSwapchainPresent,
};

struct CRenderTarget
{
    CImage* m_image;
    ImageUsage m_usage;
};
}

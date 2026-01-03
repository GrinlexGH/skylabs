#pragma once
#include <skylabs/core/render/vulkan/memory/image.hpp>

namespace Vulkan {
enum class ImageUsage : std::uint8_t
{
    eMSAAWrite,
    eShaderRead,
    eSwapchainPresent,
};

struct CRenderTarget
{
    CImage* m_image;
    ImageUsage m_usage;
};

struct CRenderPassDescription
{
    std::span<const CRenderTarget> m_colorImages;
    std::span<const CRenderTarget> m_resolveImages;
    CImage* m_depthImage = nullptr;
};
}

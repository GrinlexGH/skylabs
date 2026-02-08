#pragma once
#include <vulkan/vulkan.hpp>
#include <variant>

namespace Vulkan {
struct AbsoluteMemorySize
{
    std::uint32_t m_width  = 1;
    std::uint32_t m_height = 1;
    std::uint32_t m_depth  = 1;
};

// Relative to viewport
struct RelativeMemorySize
{
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    std::uint32_t depth = 1;
};

struct CAttachmentDescirption
{
    using AttachmentExtent = std::variant<AbsoluteMemorySize, RelativeMemorySize>;

    vk::Format format;
    vk::ImageUsageFlags usage;
    vk::ImageAspectFlags aspect;

    AttachmentExtent extent;

    vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
};

class CRGResourceManager
{

};
}

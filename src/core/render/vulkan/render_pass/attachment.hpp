#pragma once
#include <vulkan/vulkan.hpp>

namespace Vulkan {
class CAttachment
{
public:
    explicit CAttachment(const vk::AttachmentDescription& description, const vk::AttachmentReference& reference);
    static auto ColorAttachment(vk::Format swapchainFormat) -> CAttachment;

//private:
    vk::AttachmentDescription m_description;
    vk::AttachmentReference m_reference;
};
}

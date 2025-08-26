#pragma once
#include <vulkan/vulkan.hpp>

namespace Vulkan {
class CAttachment
{
public:
    explicit CAttachment(vk::AttachmentDescription description, vk::AttachmentReference reference);
    static auto ColorAttachment(vk::Format swapchainFormat) -> CAttachment;

//private:
    vk::AttachmentDescription m_description;
    vk::AttachmentReference m_reference;
};
}

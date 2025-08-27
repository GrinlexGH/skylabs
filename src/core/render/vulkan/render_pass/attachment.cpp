#include "attachment.hpp"

namespace Vulkan {
CAttachment::CAttachment(const vk::AttachmentDescription& description, const vk::AttachmentReference& reference) :
    m_description(description), m_reference(reference)
{}

auto CAttachment::ColorAttachment(vk::Format swapchainFormat) -> CAttachment {
    return CAttachment {
        {
            {},
            swapchainFormat,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::ePresentSrcKHR,
        },
        { 0, vk::ImageLayout::eColorAttachmentOptimal, }
    };
}
}

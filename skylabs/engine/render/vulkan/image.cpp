#include "skylabs/engine/render/vulkan/image.hpp"

namespace {
vk::ImageAspectFlags DetermineAspect(const vk::Format format) noexcept {
    switch (format) {
        case vk::Format::eD32Sfloat:
        case vk::Format::eD16Unorm:
            return vk::ImageAspectFlagBits::eDepth;
        case vk::Format::eD24UnormS8Uint:
        case vk::Format::eD32SfloatS8Uint:
            return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        default:
            return vk::ImageAspectFlagBits::eColor;
    }
}
}

namespace sk::render::vulkan {
Image::Image(const vma::raii::Allocator& allocator, const ImageCreateInfo& options)
    : m_format(options.format),
      m_extent(options.extent),
      m_mipLevels(options.mipLevels),
      m_arrayLevels(options.arrayLevels),
      m_samples(options.samples) {
    vk::ImageCreateInfo imageInfo { };
    imageInfo.format = m_format;
    imageInfo.extent = m_extent;
    imageInfo.imageType = options.type;
    imageInfo.mipLevels = m_mipLevels;
    imageInfo.arrayLayers = m_arrayLevels;
    imageInfo.samples = m_samples;
    imageInfo.tiling = options.tiling;
    imageInfo.usage = options.usage;
    imageInfo.sharingMode = options.sharingMode;
    imageInfo.setQueueFamilyIndices(options.queueFamilies);
    imageInfo.initialLayout = options.initialLayout;

    vma::AllocationCreateInfo allocInfo { };
    allocInfo.usage = vma::MemoryUsage::eAuto;
    allocInfo.requiredFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;

    m_handle = vma::raii::Image { allocator, imageInfo, allocInfo };
}

Image::Image(vk::Image imported, vk::Format format, vk::Extent3D extent, std::uint32_t mipLevels,
             std::uint32_t arrayLevels, vk::SampleCountFlagBits sampleCount)
    : m_handle(imported),
      m_format(format),
      m_extent(extent),
      m_mipLevels(mipLevels),
      m_arrayLevels(arrayLevels),
      m_samples(sampleCount) { }

vk::ImageAspectFlags Image::AspectMask() const noexcept { return DetermineAspect(m_format); }

vk::Image Image::Handle() const {
    if (auto const* img = std::get_if<vk::Image>(&m_handle)) {
        return *img;
    }
    return *std::get<vma::raii::Image>(m_handle);
}

Attachment::Attachment(const vk::raii::Device& device, const vma::raii::Allocator& allocator,
                       const ImageCreateInfo& options, const vk::ImageViewType viewType)
    : Image(allocator, options) {
    CreateView(device, viewType);
}

Attachment::Attachment(const vk::raii::Device& device, const vk::Image imported, const vk::Format format,
                       const vk::Extent3D extent, const std::uint32_t mipLevels,
                       const std::uint32_t arrayLevels, const vk::SampleCountFlagBits sampleCount,
                       const vk::ImageViewType viewType)
    : Image(imported, format, extent, mipLevels, arrayLevels, sampleCount) {
    CreateView(device, viewType);
}

void Attachment::CreateView(const vk::raii::Device& device, const vk::ImageViewType viewType) {
    vk::ImageViewCreateInfo viewInfo { };
    viewInfo.image = Handle();
    viewInfo.viewType = viewType;
    viewInfo.format = m_format;
    viewInfo.subresourceRange = { AspectMask(), 0, m_mipLevels, 0, m_arrayLevels };
    m_view = vk::raii::ImageView { device, viewInfo };
}
}

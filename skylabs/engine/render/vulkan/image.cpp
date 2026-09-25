#include "skylabs/engine/render/vulkan/image.hpp"

namespace {
vk::ImageViewType DetermineViewType(const vk::Extent3D extent, const std::uint32_t layers) {
    if (extent.height == 1 && extent.depth == 1) {
        return (layers == 1) ? vk::ImageViewType::e1D : vk::ImageViewType::e1DArray;
    }
    if (extent.depth == 1) {
        return (layers == 1) ? vk::ImageViewType::e2D : vk::ImageViewType::e2DArray;
    }
    assert(layers == 1);
    return vk::ImageViewType::e3D;
}

vk::ImageType DetermineType(const vk::Extent3D extent, [[maybe_unused]] const std::uint32_t layers) {
    if (extent.height == 1 && extent.depth == 1) {
        return vk::ImageType::e1D;
    }
    if (extent.depth == 1) {
        return vk::ImageType::e2D;
    }
    assert(layers == 1);
    return vk::ImageType::e3D;
}

vk::ImageAspectFlags DetermineAspect(const vk::Format format) {
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
Image::Image(const vk::raii::Device& device, const vma::raii::Allocator& allocator,
             const ImageCreateInfo& options)
    : m_format(options.format),
      m_extent(options.extent),
      m_mipLevels(options.mipLevels),
      m_arrayLevels(options.arrayLevels),
      m_sampleCount(options.sampleCount),
      m_aspectFlags(DetermineAspect(m_format)) {
    vk::ImageCreateInfo imageInfo { };
    imageInfo.format = m_format;
    imageInfo.extent = m_extent;

    imageInfo.imageType = DetermineType(m_extent, m_arrayLevels);

    imageInfo.mipLevels = m_mipLevels;
    imageInfo.arrayLayers = m_arrayLevels;
    imageInfo.samples = m_sampleCount;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.usage = options.usageFlags;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.queueFamilyIndexCount = 0;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;

    vma::AllocationCreateInfo allocInfo { };
    allocInfo.usage = vma::MemoryUsage::eAuto;
    allocInfo.requiredFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;

    m_handle = vma::raii::Image { allocator, imageInfo, allocInfo };

    CreateView(device, DetermineViewType(m_extent, m_arrayLevels));
}

Image::Image(const vk::raii::Device& device, vk::Image imported, const vk::Extent3D extent,
             const vk::Format format, const std::uint32_t mipLevels, const std::uint32_t arrayLevels,
             const vk::SampleCountFlagBits sampleCount)
    : m_handle(imported),
      m_format(format),
      m_extent(extent),
      m_mipLevels(mipLevels),
      m_arrayLevels(arrayLevels),
      m_sampleCount(sampleCount),
      m_aspectFlags(DetermineAspect(m_format)) {
    CreateView(device, DetermineViewType(m_extent, m_arrayLevels));
}

void Image::CreateView(const vk::raii::Device& device, const vk::ImageViewType viewType) {
    vk::ImageViewCreateInfo viewInfo { };
    viewInfo.image = Handle();
    viewInfo.viewType = viewType;
    viewInfo.format = m_format;
    viewInfo.subresourceRange = { m_aspectFlags, 0, m_mipLevels, 0, m_arrayLevels };
    m_view = vk::raii::ImageView { device, viewInfo };
}
}

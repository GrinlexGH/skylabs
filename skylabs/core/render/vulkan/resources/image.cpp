#include <skylabs/core/render/vulkan/resources/image.hpp>

namespace {
vk::ImageViewType DetermineViewType(vk::Extent3D extent, std::uint32_t layers) {
    if (extent.height == 1 && extent.depth == 1) {
        return (layers == 1) ? vk::ImageViewType::e1D : vk::ImageViewType::e1DArray;
    }
    if (extent.depth == 1) {
        return (layers == 1) ? vk::ImageViewType::e2D : vk::ImageViewType::e2DArray;
    }
    assert(layers == 1);
    return vk::ImageViewType::e3D;
}

[[nodiscard]] vk::ImageType DetermineType(vk::Extent3D extent, std::uint32_t layers) {
    if (extent.height == 1 && extent.depth == 1) {
        return vk::ImageType::e1D;
    }
    if (extent.depth == 1) {
        return vk::ImageType::e2D;
    }
    assert(layers == 1);
    return vk::ImageType::e3D;
}

vk::ImageAspectFlags DetermineAspect(vk::Format format) {
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

namespace Vulkan {
CImage::CImage(
    const CContext& context,
    const ImageCreateInfo options
) : m_format(options.m_format), m_extent(options.m_extent),
    m_mipLevels(options.m_mipLevels), m_arrayLevels(options.m_arrayLevels), m_sampleCount(options.m_sampleCount),
    m_aspectFlags(DetermineAspect(m_format))
{
    vk::ImageCreateInfo imageInfo {};
    imageInfo.format = m_format;
    imageInfo.extent = m_extent;

    imageInfo.imageType = DetermineType(m_extent, m_arrayLevels);

    imageInfo.mipLevels = m_mipLevels;
    imageInfo.arrayLayers = m_arrayLevels;
    imageInfo.samples = m_sampleCount;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.usage = options.m_usageFlags;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.queueFamilyIndexCount = 0;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;

    vma::AllocationCreateInfo allocInfo {};
    allocInfo.usage = vma::MemoryUsage::eAuto;
    allocInfo.requiredFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;

    m_handle = vma::raii::Image { *context.Allocator(), imageInfo, allocInfo };
    m_rawHandle = *m_handle;

    CreateView(*context.Device(), DetermineViewType(m_extent, m_arrayLevels));
}

CImage::CImage(
    const CContext& context,
    vk::Image imported,
    vk::Extent3D extent, vk::Format format,
    std::uint32_t mipLevels, std::uint32_t arrayLevels,
    vk::SampleCountFlagBits sampleCount
) : m_rawHandle(imported),
    m_format(format), m_extent(extent),
    m_mipLevels(mipLevels), m_arrayLevels(arrayLevels), m_sampleCount(sampleCount),
    m_aspectFlags(DetermineAspect(m_format))
{
    CreateView(*context.Device(), DetermineViewType(m_extent, m_arrayLevels));
}

void CImage::CreateView(const vk::raii::Device& device, vk::ImageViewType viewType) {
    vk::ImageViewCreateInfo viewInfo {};
    viewInfo.image = m_rawHandle;
    viewInfo.viewType = viewType;
    viewInfo.format = m_format;
    viewInfo.subresourceRange = { m_aspectFlags, 0, m_mipLevels, 0, m_arrayLevels };
    m_view = vk::raii::ImageView { device, viewInfo };
}

void CImage::Clear() {
    m_handle.clear();
    m_view.clear();
    m_format = vk::Format::eUndefined;
    m_extent = vk::Extent3D {};
    m_mipLevels = 1;
    m_sampleCount = vk::SampleCountFlagBits::e1;
    m_aspectFlags = vk::ImageAspectFlagBits::eNone;
}
}

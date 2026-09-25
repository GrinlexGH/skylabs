#pragma once
#include <variant>

#include <vk_mem_alloc_raii.hpp>

namespace sk::render::vulkan {
struct ImageCreateInfo {
    vk::Extent3D extent { };
    vk::Format format = vk::Format::eR8G8B8A8Snorm;
    std::uint32_t mipLevels = 1;
    std::uint32_t arrayLevels = 1;
    vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;
    vk::ImageUsageFlags usageFlags;
};

class Image {
public:
    explicit Image(std::nullptr_t) { }
    explicit Image(const vk::raii::Device& device, const vma::raii::Allocator& allocator,
                   const ImageCreateInfo& options = { });
    explicit Image(const vk::raii::Device& device, vk::Image imported, vk::Extent3D extent,
                   vk::Format format, std::uint32_t mipLevels, std::uint32_t arrayLevels,
                   vk::SampleCountFlagBits sampleCount);
    Image(const Image&) = delete;
    Image(Image&&) noexcept = default;
    Image& operator=(const Image&) = delete;
    Image& operator=(Image&&) noexcept = default;
    ~Image() = default;

    [[nodiscard]] vk::Image operator*() const noexcept { return Handle(); }

    [[nodiscard]] const vk::raii::ImageView& View() const noexcept { return m_view; }
    [[nodiscard]] vk::Format Format() const noexcept { return m_format; }
    [[nodiscard]] vk::Extent3D Extent() const noexcept { return m_extent; }
    [[nodiscard]] vk::Extent2D Extent2D() const noexcept {
        return vk::Extent2D { m_extent.width, m_extent.height };
    }
    [[nodiscard]] std::uint32_t MipLevels() const noexcept { return m_mipLevels; }
    [[nodiscard]] std::uint32_t ArrayLevels() const noexcept { return m_arrayLevels; }
    [[nodiscard]] vk::SampleCountFlagBits SampleCount() const noexcept { return m_sampleCount; }
    [[nodiscard]] vk::ImageAspectFlags AspectFlags() const noexcept { return m_aspectFlags; }
    [[nodiscard]] vk::ImageSubresourceRange FullRange() const noexcept {
        return { m_aspectFlags, 0, m_mipLevels, 0, m_arrayLevels };
    }

private:
    void CreateView(const vk::raii::Device& device, vk::ImageViewType viewType);
    vk::Image Handle() const {
        if (auto const* img = std::get_if<vk::Image>(&m_handle)) {
            return *img;
        }
        return *std::get<vma::raii::Image>(m_handle);
    }

    std::variant<vk::Image, vma::raii::Image> m_handle;
    vk::raii::ImageView m_view { nullptr };

    vk::Format m_format = vk::Format::eUndefined;
    vk::Extent3D m_extent { };
    std::uint32_t m_mipLevels = 1;
    std::uint32_t m_arrayLevels = 1;
    vk::SampleCountFlagBits m_sampleCount = vk::SampleCountFlagBits::e1;
    vk::ImageAspectFlags m_aspectFlags = vk::ImageAspectFlagBits::eNone;
};
}

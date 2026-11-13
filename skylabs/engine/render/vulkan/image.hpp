#pragma once
#include <variant>

#include <vk_mem_alloc_raii.hpp>

namespace sk::render::vulkan {
struct ImageCreateInfo {
    vk::ImageType type = vk::ImageType::e2D;
    vk::Format format = vk::Format::eR8G8B8A8Snorm;
    vk::Extent3D extent { 1, 1, 1 };
    std::uint32_t mipLevels = 1;
    std::uint32_t arrayLevels = 1;
    vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
    vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
    vk::ImageUsageFlags usage;
    vk::SharingMode sharingMode = vk::SharingMode::eExclusive;
    std::span<const uint32_t> queueFamilies;
    vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined;
};

class Image {
public:
    explicit Image(std::nullptr_t) { }
    explicit Image(const vma::raii::Allocator& allocator, const ImageCreateInfo& options = { });
    explicit Image(vk::Image imported, vk::Format format, vk::Extent3D extent, std::uint32_t mipLevels,
                   std::uint32_t arrayLevels, vk::SampleCountFlagBits sampleCount);

    [[nodiscard]] vk::Image operator*() const noexcept { return Handle(); }

    [[nodiscard]] vk::Format Format() const noexcept { return m_format; }
    [[nodiscard]] vk::Extent3D Extent() const noexcept { return m_extent; }
    [[nodiscard]] vk::Extent2D Extent2D() const noexcept { return { m_extent.width, m_extent.height }; }
    [[nodiscard]] std::uint32_t MipLevels() const noexcept { return m_mipLevels; }
    [[nodiscard]] std::uint32_t ArrayLevels() const noexcept { return m_arrayLevels; }
    [[nodiscard]] vk::SampleCountFlagBits SampleCount() const noexcept { return m_samples; }
    [[nodiscard]] vk::ImageAspectFlags AspectMask() const noexcept;
    [[nodiscard]] vk::ImageSubresourceRange FullRange() const noexcept {
        return { AspectMask(), 0, m_mipLevels, 0, m_arrayLevels };
    }

protected:
    vk::Image Handle() const;

    std::variant<vk::Image, vma::raii::Image> m_handle;

    vk::Format m_format { };
    vk::Extent3D m_extent { };
    std::uint32_t m_mipLevels = 1;
    std::uint32_t m_arrayLevels = 1;
    vk::SampleCountFlagBits m_samples = vk::SampleCountFlagBits::e1;
};

class Attachment : public Image {
public:
    explicit Attachment(std::nullptr_t) : Image(nullptr) { }
    explicit Attachment(const vk::raii::Device& device, const vma::raii::Allocator& allocator,
                        const ImageCreateInfo& options = { },
                        vk::ImageViewType viewType = vk::ImageViewType::e2D);
    explicit Attachment(const vk::raii::Device& device, vk::Image imported, vk::Format format,
                        vk::Extent3D extent, std::uint32_t mipLevels, std::uint32_t arrayLevels,
                        vk::SampleCountFlagBits sampleCount,
                        vk::ImageViewType viewType = vk::ImageViewType::e2D);
    Attachment(const Attachment&) = delete;
    Attachment(Attachment&&) noexcept = default;
    Attachment& operator=(const Attachment&) = delete;
    Attachment& operator=(Attachment&&) noexcept = default;
    ~Attachment() = default;

    [[nodiscard]] const vk::raii::ImageView& View() const noexcept { return m_view; }

private:
    void CreateView(const vk::raii::Device& device, vk::ImageViewType viewType);

    vk::raii::ImageView m_view { nullptr };
};
}

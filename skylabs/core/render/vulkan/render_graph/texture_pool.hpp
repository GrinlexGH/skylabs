#pragma once
#include <skylabs/core/render/vulkan/resources/image.hpp>

#include <variant>

namespace Vulkan {
struct RelativeTextureSize
{
    float m_scaleX = 1.0f;
    float m_scaleY = 1.0f;
    std::uint32_t m_depth = 1;
};

using TextureExtent = std::variant<vk::Extent3D, RelativeTextureSize>;

struct TextureDescirption
{
    TextureExtent m_extent = RelativeTextureSize {};
    vk::Format m_format = vk::Format::eR8G8B8A8Srgb;
    vk::ImageUsageFlags m_usage = vk::ImageUsageFlagBits::eSampled;
    vk::SampleCountFlagBits m_sampleCount = vk::SampleCountFlagBits::e1;
    std::uint32_t m_mipLevels = 1;
    std::uint32_t m_arrayLevels = 1;
};

struct TextureHandle
{
    unsigned int m_id = ~0u;
};

class CTexturePool
{
public:
    explicit CTexturePool(std::nullptr_t) {}
    explicit CTexturePool(const CContext& context, vk::Extent2D viewportExtent, std::uint32_t inFlightCount);
    CTexturePool(const CTexturePool&) = delete;
    CTexturePool(CTexturePool&&) noexcept = default;
    CTexturePool& operator=(const CTexturePool&) = delete;
    CTexturePool& operator=(CTexturePool&&) noexcept = default;
    ~CTexturePool() = default;

    [[nodiscard]] TextureHandle CreateTexture(const char* debugName, const TextureDescirption& description);
    [[nodiscard]] TextureHandle ImportTexture(const char* debugName, CImage image);

    void GenerateTextures();
    [[nodiscard]] CImage& GetTexture(TextureHandle handle, int index = -1);

    void Resize(vk::Extent2D newViewportExtent);
    void SetFrameIndex(std::uint32_t newFrameIndex) { m_frameIndex = newFrameIndex; }

private:
    const CContext* m_context = nullptr;
    vk::Extent2D m_viewportExtent;
    std::uint32_t m_inFlightCount = 0;
    std::uint32_t m_frameIndex = 0;

    // TODO: too much info duplication
    struct TextureMeta
    {
        std::string m_debugName;
        TextureDescirption m_description;
    };

    struct Texture
    {
        TextureMeta m_info;
        std::vector<CImage> m_images;
    };

    std::vector<Texture> m_textures;
    std::vector<unsigned int> m_dirtyIndices;

    CImage CreateImage(const TextureMeta& meta);
};
}

#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/resources/image.hpp>
#include <skylabs/public/utils.hpp>

#include <variant>

namespace Vulkan {
struct RelativeTextureSize
{
    float m_scaleX = 1.0f;
    float m_scaleY = 1.0f;
    std::uint32_t m_depth = 1;
};

using TextureExtent = std::variant<vk::Extent3D, RelativeTextureSize>;

enum class TextureFormat : std::uint8_t
{
    eRGBA8888Srgb = 0,
    eRGBA8888Unorm,
    eDepthOptimal
};

enum class TextureUsageBits : std::uint8_t
{
    eAttachment = 1 << 0,
    eDepthAttachment = 1 << 1,
    eSampled = 1 << 2,
    eStorage = 1 << 3
};

using TextureUsage = Utils::Flags<TextureUsageBits>;

struct TextureDescirption
{
    TextureFormat m_format = TextureFormat::eRGBA8888Srgb;
    TextureUsage m_usage = TextureUsageBits::eSampled;
    TextureExtent m_extent = RelativeTextureSize {};
    bool m_sampled = false;
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
    explicit CTexturePool(const CContext& context, Utils::Extent2D viewportExtent, std::uint32_t inFlightCount);
    CTexturePool(const CTexturePool&) = delete;
    CTexturePool(CTexturePool&&) noexcept = default;
    CTexturePool& operator=(const CTexturePool&) = delete;
    CTexturePool& operator=(CTexturePool&&) noexcept = default;
    ~CTexturePool() = default;

    [[nodiscard]] TextureHandle CreateTexture(const char* debugName, const TextureDescirption& description);
    [[nodiscard]] TextureHandle ImportTexture(const char* debugName, CImage image);

    void GenerateTextures();
    [[nodiscard]] CImage& GetTexture(TextureHandle handle, int index = -1);

    void Resize(Utils::Extent2D newViewportExtent);
    void SetFrameIndex(std::uint32_t newFrameIndex) { m_frameIndex = newFrameIndex; }

private:
    const CContext* m_context = nullptr;
    Utils::Extent2D m_viewportExtent;
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

template <>
struct Utils::FlagTraits<Vulkan::TextureUsageBits>
{
    static constexpr bool isBitmask = true;
    static constexpr Vulkan::TextureUsage allFlags =
        Vulkan::TextureUsageBits::eAttachment |
        Vulkan::TextureUsageBits::eDepthAttachment |
        Vulkan::TextureUsageBits::eSampled;
};

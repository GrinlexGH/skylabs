#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/memory/image.hpp>
#include <skylabs/core/render/vulkan/memory/host_buffer.hpp>
#include <skylabs/public/utils.hpp>

#include <variant>
#include <filesystem>

namespace Vulkan::RG {
struct AbsoluteTextureSize
{
    std::uint32_t m_width = 1;
    std::uint32_t m_height = 1;
    std::uint32_t m_depth = 1;
};

struct RelativeTextureSize
{
    float m_scaleX = 1.0f;
    float m_scaleY = 1.0f;
    std::uint32_t m_depth = 1;
};

using TextureExtent = std::variant<AbsoluteTextureSize, RelativeTextureSize>;

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
};

struct TextureHandle
{
    unsigned int m_id = ~0;
};

class CTextureManager
{
public:
    explicit CTextureManager(std::nullptr_t) {}
    explicit CTextureManager(const CContext& context, Utils::Extent2D viewportExtent, std::uint32_t inFlightCount);
    CTextureManager(const CTextureManager&) = delete;
    CTextureManager(CTextureManager&&) noexcept = default;
    CTextureManager& operator=(const CTextureManager&) = delete;
    CTextureManager& operator=(CTextureManager&&) noexcept = default;
    ~CTextureManager() = default;

    [[nodiscard]] TextureHandle CreateEmptyTexture(const char* debugName, const TextureDescirption& description);

    void GenerateTextures();
    [[nodiscard]] CImage& GetTexture(TextureHandle handle, int index = -1);

    void Resize(Utils::Extent2D newViewportExtent);
    void SetFrameIndex(std::uint32_t newFrameIndex) { m_frameIndex = newFrameIndex; }

private:
    const CContext* m_context = nullptr;
    Utils::Extent2D m_viewportExtent;
    std::uint32_t m_inFlightCount = 0;
    std::uint32_t m_frameIndex = 0;


    struct TextureMeta
    {
        std::string m_debugName;
        TextureFormat m_format;
        TextureUsage m_usage;

        bool m_sampled;
        bool m_dirty = true;

        TextureExtent m_extent;
        std::uint32_t m_mipLevels;
    };

    struct Texture
    {
        TextureMeta m_info;
        std::vector<CImage> m_images;
    };

    std::vector<Texture> m_textures;

    CImage CreateImage(const TextureMeta& desc);
};
}

template <>
struct Utils::FlagTraits<Vulkan::RG::TextureUsageBits>
{
    static constexpr bool isBitmask = true;
    static constexpr Vulkan::RG::TextureUsage allFlags =
        Vulkan::RG::TextureUsageBits::eAttachment |
        Vulkan::RG::TextureUsageBits::eDepthAttachment |
        Vulkan::RG::TextureUsageBits::eSampled;
};

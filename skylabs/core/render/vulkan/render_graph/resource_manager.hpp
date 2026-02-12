#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/memory/image.hpp>
#include <skylabs/public/utils.hpp>

#include <variant>
#include <filesystem>

namespace Vulkan::RG {
struct AbsoluteMemorySize
{
    std::uint32_t m_width  = 1;
    std::uint32_t m_height = 1;
    std::uint32_t m_depth  = 1;
};

struct RelativeMemorySize
{
    float m_scaleX = 1.0f;
    float m_scaleY = 1.0f;
    std::uint32_t m_depth = 1;
};

using TextureExtent = std::variant<AbsoluteMemorySize, RelativeMemorySize>;

enum class TextureFormat : std::uint8_t
{
    eRGBA8888Srgb = 0,
    eDepthOptimal
};

enum class TextureUsageBits : std::uint8_t
{
    eAttachment = 1 << 0,
    eDepthAttachment = 1 << 1,
    eSampled = 1 << 2
};

using TextureUsage = Utils::Flags<TextureUsageBits>;

struct TextureDescirption
{
    TextureFormat m_format;
    TextureUsage m_usage;
    TextureExtent m_extent;

    bool m_sampled;
    std::uint32_t m_mipLevels = 1;
};

struct AssetTextureDescirption
{
    std::string m_assetFileName;

    TextureFormat m_format;
    bool m_sampled;
    bool m_mipmapped;
};

struct TextureHandle
{
    unsigned int m_id;
};

class CResourceManager
{
public:
    CResourceManager(std::nullptr_t) {}
    explicit CResourceManager(const CContext& context);
    CResourceManager(const CResourceManager&) = delete;
    CResourceManager(CResourceManager&&) noexcept = default;
    CResourceManager& operator=(const CResourceManager&) = delete;
    CResourceManager& operator=(CResourceManager&&) noexcept = default;
    ~CResourceManager() = default;

    TextureHandle CreateEmptyTexture(TextureDescirption description);
    TextureHandle CreateAssetTexture(AssetTextureDescirption description);

    void GenerateTextures();
    CImage& GetTexture(TextureHandle handle);

    void SetFameIndex(unsigned int newIndex) { m_frameIndex = newIndex; }

private:
    struct TextureMeta
    {
        TextureFormat m_format;
        TextureUsage m_usage;

        bool m_sampled;

        struct EmptySource {
            TextureExtent m_extent;
            std::uint32_t m_mipLevels = 1;
        };

        struct AssetSource {
            std::string m_assetFileName;
            bool m_mipmapped;
        };

        std::variant<EmptySource, AssetSource> m_source;
    };

    const CContext* m_context = nullptr;

    std::unordered_map<unsigned int, TextureMeta> m_creationPendingTextures;
    std::unordered_map<unsigned int, std::vector<CImage>> m_textures;

    unsigned int m_frameIndex = 0;
};
}

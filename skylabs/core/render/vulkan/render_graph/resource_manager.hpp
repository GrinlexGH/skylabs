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
    std::uint32_t m_width  = 1;
    std::uint32_t m_height = 1;
    std::uint32_t m_depth  = 1;
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
    TextureExtent m_extent;

    bool m_sampled = false;
    std::uint32_t m_mipLevels = 1;
};

struct AssetTextureDescirption
{
    std::string m_assetFileName;

    TextureFormat m_format = TextureFormat::eRGBA8888Unorm;
    bool m_sampled = false;
    bool m_mipmapped = true;
};

struct TextureHandle
{
    unsigned int m_id = ~0;
};

struct DescriptorHandle
{
    unsigned int m_id = ~0;
};

class CResourceManager
{
public:
    explicit CResourceManager(std::nullptr_t) {}
    explicit CResourceManager(const CContext& context, Utils::Extent2D viewportExtent, std::uint32_t inFlightCount);
    CResourceManager(const CResourceManager&) = delete;
    CResourceManager(CResourceManager&&) noexcept = default;
    CResourceManager& operator=(const CResourceManager&) = delete;
    CResourceManager& operator=(CResourceManager&&) noexcept = default;
    ~CResourceManager() = default;

    [[nodiscard]] TextureHandle CreateEmptyTexture(const char* debugName, const TextureDescirption& description);
    [[nodiscard]] TextureHandle CreateAssetTexture(const char* debugName, const AssetTextureDescirption& description);

    void GenerateTextures();
    CImage& GetTexture(TextureHandle handle);


    [[nodiscard]] DescriptorHandle CreateUniformBuffer(const char* debugName, std::size_t size);

    void GenerateDescriptorObjects();
    CHostBuffer& GetUniformBuffer(DescriptorHandle handle, int index = -1);


    void Resize(Utils::Extent2D newViewportExtent);
    void SetFrameIndex(std::uint32_t newFrameIndex) { m_frameIndex = newFrameIndex; }

    friend class CRenderPass;

private:
    struct TextureMeta
    {
        std::string m_debugName;
        TextureFormat m_format;
        TextureUsage m_usage;

        bool m_sampled;
        bool m_dirty = true;

        struct EmptySource {
            TextureExtent m_extent;
            std::uint32_t m_mipLevels;
        };

        struct AssetSource {
            std::string m_assetFileName;
            bool m_mipmapped;
        };

        std::variant<EmptySource, AssetSource> m_source;
    };

    struct UniformBufferMeta
    {
        std::string m_debugName;
        std::size_t m_size;
    };

    const CContext* m_context = nullptr;
    Utils::Extent2D m_viewportExtent;
    std::uint32_t m_inFlightCount = 0;
    std::uint32_t m_frameIndex = 0;

    std::unordered_map<unsigned int, TextureMeta> m_creationPendingTextures;
    std::unordered_map<unsigned int, std::vector<CImage>> m_textures;

    CImage CreateImage(const TextureMeta& desc);


    struct DescriptorRequirements {
        uint32_t uniformBuffers = 0;
        uint32_t combinedSamplers = 0;
        uint32_t storageImages = 0;
        uint32_t totalSets = 0;
    };

    DescriptorRequirements m_descriptorRequirements;
    vk::raii::DescriptorPool m_descriptorPool { nullptr };

    std::unordered_map<unsigned int, UniformBufferMeta> m_creationPendingUniformBuffers;
    std::unordered_map<unsigned int, std::vector<CHostBuffer>> m_uniformBuffers;

    void BuildDescriptorPool();
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

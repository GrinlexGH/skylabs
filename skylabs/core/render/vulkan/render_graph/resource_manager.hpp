#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/image.hpp>
#include <skylabs/core/render/vulkan/buffer.hpp>
#include <skylabs/core/render/vulkan/shader.hpp>
#include <skylabs/core/render/vulkan/sampler.hpp>
#include <skylabs/public/utils.hpp>

#include <variant>
#include <filesystem>

namespace Vulkan::RG {
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
};

struct TextureHandle
{
    unsigned int m_id = ~0u;
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


enum class BufferUsageFlagBits : std::uint8_t
{
    eUniformBuffer = 1 << 0,
};

using BufferUsageFlags = Utils::Flags<BufferUsageFlagBits>;

struct BufferDescirption
{
    std::size_t m_size = 0;
    MemoryLocation m_location = MemoryLocation::eDeviceOnly;
    BufferUsageFlags m_usage;
    bool m_isInFlight = false;
};

struct BufferHandle
{
    unsigned int m_id = ~0u;
};

class CBufferManager
{
public:
    explicit CBufferManager(std::nullptr_t) {}
    explicit CBufferManager(const CContext& context, std::uint32_t inFlightCount);
    CBufferManager(const CBufferManager&) = delete;
    CBufferManager(CBufferManager&&) noexcept = default;
    CBufferManager& operator=(const CBufferManager&) = delete;
    CBufferManager& operator=(CBufferManager&&) noexcept = default;
    ~CBufferManager() = default;

    [[nodiscard]] BufferHandle CreateBuffer(const char* debugName, const BufferDescirption& description);
    [[nodiscard]] BufferHandle ImportBuffer(const char* debugName, CBuffer buffer);

    void GenerateBuffers();
    [[nodiscard]] CBuffer& GetBuffer(BufferHandle handle, int index = -1);

    void SetFrameIndex(std::uint32_t newFrameIndex) { m_frameIndex = newFrameIndex; }

private:
    const CContext* m_context = nullptr;
    std::uint32_t m_inFlightCount = 0;
    std::uint32_t m_frameIndex = 0;

    // TODO: too much info duplication
    struct BufferMeta
    {
        std::string m_debugName;
        BufferDescirption m_description;
    };

    struct Buffer
    {
        BufferMeta m_meta;
        std::vector<CBuffer> m_buffers;
    };

    std::vector<Buffer> m_buffers;
};


enum class DescriptorType : std::uint8_t
{
    eUniformBuffer = 0,
    eStorageImage,
    eCombinedImageSampler
};

struct BufferDescriptorInfo {
    BufferHandle m_buffer;
};

struct SampledImageDescriptorInfo {
    TextureHandle m_image;
    const vk::raii::Sampler* m_sampler = nullptr;
};

struct StorageImageDescriptorInfo {
    TextureHandle m_image;
};

struct DescriptorDescription
{
    DescriptorType m_type;
    vk::ShaderStageFlags m_shaderStages;
    std::variant<BufferDescriptorInfo, SampledImageDescriptorInfo, StorageImageDescriptorInfo> m_info;
};

struct DescriptorSetHandle
{
    unsigned int m_id = ~0u;
};

class CDescriptorManager
{
public:
    explicit CDescriptorManager(std::nullptr_t) {}
    explicit CDescriptorManager(const CContext& context, std::uint32_t inFlightCount);
    CDescriptorManager(const CDescriptorManager&) = delete;
    CDescriptorManager(CDescriptorManager&&) noexcept = default;
    CDescriptorManager& operator=(const CDescriptorManager&) = delete;
    CDescriptorManager& operator=(CDescriptorManager&&) noexcept = default;
    ~CDescriptorManager() = default;

    [[nodiscard]] DescriptorSetHandle CreateDescriptorSet(std::span<const DescriptorDescription> descriptors);

    void CreateDescriptorPool();
    void CreateDescriptorSets();
    void UpdateDescriptorSets(CBufferManager& bufferManager, CTextureManager& textureManager);

    [[nodiscard]] vk::DescriptorSet GetDescriptorSet(DescriptorSetHandle handle, int index = -1);
    [[nodiscard]] const vk::raii::DescriptorSetLayout& GetDescriptorSetLayout(DescriptorSetHandle handle);

    void SetFrameIndex(std::uint32_t newFrameIndex) { m_frameIndex = newFrameIndex; }

private:
    const CContext* m_context = nullptr;
    std::uint32_t m_inFlightCount = 0;
    std::uint32_t m_frameIndex = 0;

    struct DescriptorSetMeta
    {
        std::vector<DescriptorDescription> m_descriptors;
    };

    struct DescriptorSet
    {
        DescriptorSetMeta meta;
        vk::raii::DescriptorSetLayout m_layout { nullptr };
        std::vector<vk::DescriptorSet> m_descriptorSets;
    };

    std::vector<DescriptorSet> m_descriptorSets;

    vk::raii::DescriptorPool m_pool { nullptr };
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

template <>
struct Utils::FlagTraits<Vulkan::RG::BufferUsageFlagBits>
{
    static constexpr Vulkan::RG::BufferUsageFlags allFlags = Vulkan::RG::BufferUsageFlagBits::eUniformBuffer;
};

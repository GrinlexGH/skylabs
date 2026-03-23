#pragma once
#include <skylabs/core/render/vulkan/pipeline/descriptor_layout_cache.hpp>
#include <skylabs/core/render/vulkan/render_graph/buffer_pool.hpp>
#include <skylabs/core/render/vulkan/render_graph/texture_pool.hpp>

namespace Vulkan {
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
    const vk::Sampler m_sampler = nullptr;
};

struct StorageImageDescriptorInfo {
    TextureHandle m_image;
};

struct DescriptorDescription
{
    vk::DescriptorType m_type;
    vk::ShaderStageFlags m_shaderStages;
    std::variant<BufferDescriptorInfo, SampledImageDescriptorInfo, StorageImageDescriptorInfo> m_info;
};

struct DescriptorSetHandle
{
    unsigned int m_id = ~0u;
};

class CDescriptorPool
{
public:
    explicit CDescriptorPool(std::nullptr_t) {}
    explicit CDescriptorPool(const CContext& context, std::uint32_t inFlightCount);
    CDescriptorPool(const CDescriptorPool&) = delete;
    CDescriptorPool(CDescriptorPool&&) noexcept = default;
    CDescriptorPool& operator=(const CDescriptorPool&) = delete;
    CDescriptorPool& operator=(CDescriptorPool&&) noexcept = default;
    ~CDescriptorPool() = default;

    [[nodiscard]] DescriptorSetHandle CreateDescriptorSet(std::initializer_list<const DescriptorDescription> descriptors);

    void CreateDescriptorPool();
    void CreateDescriptorSets();
    void UpdateDescriptorSets(CBufferPool& bufferManager, CTexturePool& textureManager);

    [[nodiscard]] vk::DescriptorSet GetDescriptorSet(DescriptorSetHandle handle, int index = -1);
    [[nodiscard]] const vk::raii::DescriptorSetLayout* GetDescriptorSetLayout(DescriptorSetHandle handle);

    void SetFrameIndex(std::uint32_t newFrameIndex) { m_frameIndex = newFrameIndex; }

private:
    const CContext* m_context = nullptr;
    std::uint32_t m_inFlightCount = 0;
    std::uint32_t m_frameIndex = 0;

    CDescriptorLayoutCache m_layoutCache { nullptr };

    struct DescriptorSetMeta
    {
        std::vector<DescriptorDescription> m_descriptors;
    };

    struct DescriptorSet
    {
        DescriptorSetMeta meta;
        const vk::raii::DescriptorSetLayout* m_layout = nullptr;
        std::vector<vk::DescriptorSet> m_descriptorSets;
    };

    std::vector<DescriptorSet> m_descriptorSets;

    vk::raii::DescriptorPool m_pool { nullptr };
};
}

#include <skylabs/core/render/vulkan/pipeline/descriptor_allocator.hpp>

namespace {
constexpr std::array g_types {
    vk::DescriptorType::eSampler,
    vk::DescriptorType::eCombinedImageSampler,
    vk::DescriptorType::eSampledImage,
    vk::DescriptorType::eStorageImage,
    vk::DescriptorType::eUniformBuffer,
    vk::DescriptorType::eStorageBuffer,
    vk::DescriptorType::eUniformBufferDynamic,
    vk::DescriptorType::eStorageBufferDynamic,
    vk::DescriptorType::eInputAttachment,
};
}

namespace Vulkan {
CDescriptorAllocator::CDescriptorAllocator(const CContext& context) : m_device(&*context.Device()) { }

vk::raii::DescriptorPool CDescriptorAllocator::CreatePool(std::uint32_t count) {
    std::vector<vk::DescriptorPoolSize> sizes;
    sizes.reserve(g_types.size());

    for (const auto& type : g_types) {
        sizes.emplace_back(type, count);
    }

    vk::DescriptorPoolCreateInfo poolInfo {};
    poolInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    poolInfo.setMaxSets(count);
    poolInfo.setPoolSizes(sizes);

    return { *m_device, poolInfo };
}

vk::raii::DescriptorPool CDescriptorAllocator::GrabPool() {
    if (!m_freePools.empty()) {
        vk::raii::DescriptorPool pool = std::move(m_freePools.back());
        m_freePools.pop_back();
        return pool;
    }

    return CreatePool(1024);
}

std::expected<std::vector<vk::raii::DescriptorSet>, vk::Result> CDescriptorAllocator::Allocate(const vk::DescriptorSetAllocateInfo& allocInfo) const {
    std::vector<vk::DescriptorSet> descriptorSets(allocInfo.descriptorSetCount);
    vk::Result result = static_cast<vk::Result>(m_device->getDispatcher()->vkAllocateDescriptorSets(
        static_cast<VkDevice>(**m_device),
        reinterpret_cast<VkDescriptorSetAllocateInfo const*>(&allocInfo),
        reinterpret_cast<VkDescriptorSet*>(descriptorSets.data()))
    );

    if (result != vk::Result::eSuccess) {
        return std::unexpected(result);
    }

    std::vector<vk::raii::DescriptorSet> descriptorSetsRAII;
    descriptorSetsRAII.reserve(descriptorSets.size());
    for (auto& descriptorSet : descriptorSets) {
        descriptorSetsRAII.emplace_back(
            *m_device, *reinterpret_cast<VkDescriptorSet*>(&descriptorSet), static_cast<VkDescriptorPool>(allocInfo.descriptorPool)
        );
    }

    return descriptorSetsRAII;
}

std::vector<vk::raii::DescriptorSet> CDescriptorAllocator::Allocate(const vk::ArrayProxy<const vk::DescriptorSetLayout>& layouts) {
    if (m_currentPool == nullptr) {
        m_currentPool = GrabPool();
    }

    vk::DescriptorSetAllocateInfo allocInfo {};
    allocInfo.setDescriptorPool(m_currentPool);
    allocInfo.setSetLayouts(layouts);

    auto result = Allocate(allocInfo);
    if (!result) {
        if (result.error() != vk::Result::eErrorOutOfPoolMemory && result.error() != vk::Result::eErrorFragmentedPool) {
            throw std::runtime_error("Failed to allocate descriptor set");
        }

        m_usedPools.emplace_back(std::move(m_currentPool));

        m_currentPool = GrabPool();
        allocInfo.setDescriptorPool(m_currentPool);

        result = Allocate(allocInfo);
        if (!result) {
            throw std::runtime_error("Failed to allocate descriptor set");
        }
    }

    return std::move(result.value());
}

void CDescriptorAllocator::ResetPools() {
    for (auto& pool : m_usedPools) {
        pool.reset();
        m_freePools.emplace_back(std::move(pool));
    }

    if (m_currentPool != nullptr) {
        m_currentPool.reset();
        m_freePools.emplace_back(std::move(m_currentPool));
    }

    m_usedPools.clear();
}
}

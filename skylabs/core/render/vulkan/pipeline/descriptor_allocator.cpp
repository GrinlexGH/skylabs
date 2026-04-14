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

std::vector<vk::raii::DescriptorSet> CDescriptorAllocator::Allocate(const vk::ArrayProxy<const vk::DescriptorSetLayout>& layouts) {
    if (m_currentPool == nullptr) {
        m_currentPool = GrabPool();
    }

    vk::DescriptorSetAllocateInfo allocInfo {};
    allocInfo.setDescriptorPool(m_currentPool);
    allocInfo.setSetLayouts(layouts);

    try { return m_device->allocateDescriptorSets(allocInfo); }
    catch (const vk::SystemError& e) {
        if (static_cast<vk::Result>(e.code().value()) != vk::Result::eErrorOutOfPoolMemory &&
            static_cast<vk::Result>(e.code().value()) != vk::Result::eErrorFragmentedPool
        ) { throw; }

        m_usedPools.emplace_back(std::move(m_currentPool));

        m_currentPool = GrabPool();
        allocInfo.setDescriptorPool(m_currentPool);

        return m_device->allocateDescriptorSets(allocInfo);
    }
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

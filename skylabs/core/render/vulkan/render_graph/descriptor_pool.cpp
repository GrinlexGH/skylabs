#include <skylabs/core/render/vulkan/render_graph/descriptor_pool.hpp>

#include <frozen/map.h>
#include <deque>

namespace {
constexpr frozen::map<Vulkan::DescriptorType, vk::DescriptorType, 3> g_descriptorTypeMap
{
    { Vulkan::DescriptorType::eUniformBuffer, vk::DescriptorType::eUniformBuffer },
    { Vulkan::DescriptorType::eStorageImage, vk::DescriptorType::eStorageImage },
    { Vulkan::DescriptorType::eCombinedImageSampler, vk::DescriptorType::eCombinedImageSampler },
};
}

namespace Vulkan {
CDescriptorPool::CDescriptorPool(
    const CContext& context,
    std::uint32_t inFlightCount
) : m_context(&context), m_inFlightCount(inFlightCount), m_layoutCache(context)
{ }

DescriptorSetHandle CDescriptorPool::CreateDescriptorSet(std::initializer_list<const DescriptorDescription> descriptors) {
    DescriptorSetHandle handle { static_cast<unsigned int>(m_descriptorSets.size()) };
    m_descriptorSets.emplace_back(DescriptorSetMeta { .m_descriptors = std::vector(descriptors.begin(), descriptors.end()) });
    return handle;
}

void CDescriptorPool::CreateDescriptorPool() {
    std::array<uint32_t, 3> counts {};
    std::uint32_t totalSets = 0;

    for (const auto& set : m_descriptorSets) {
        totalSets += m_inFlightCount;
        for (const auto& desc : set.meta.m_descriptors) {
            counts[static_cast<std::size_t>(desc.m_type)] += m_inFlightCount;
        }
    }

    std::vector<vk::DescriptorPoolSize> poolSizes;
    poolSizes.reserve(counts.size());

    for (auto const& [rgType, vkType] : g_descriptorTypeMap) {
        uint32_t count = counts[static_cast<size_t>(rgType)];
        if (count > 0) {
            poolSizes.emplace_back(vkType, count);
        }
    }

    if (poolSizes.empty()) return;

    vk::DescriptorPoolCreateInfo poolInfo {};
    poolInfo.maxSets = totalSets;
    poolInfo.poolSizeCount = static_cast<std::uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    m_pool = vk::raii::DescriptorPool { *m_context->Device(), poolInfo };
}

void CDescriptorPool::CreateDescriptorSets() {
    auto& device = *m_context->Device();

    for (auto& set : m_descriptorSets) {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        for (std::uint32_t i = 0; i < set.meta.m_descriptors.size(); ++i) {
            const auto& desc = set.meta.m_descriptors[i];

            vk::DescriptorSetLayoutBinding binding {};
            binding.binding = i;
            binding.descriptorType = g_descriptorTypeMap.at(desc.m_type);
            binding.descriptorCount = 1;
            binding.stageFlags = desc.m_shaderStages;

            bindings.push_back(binding);
        }

        set.m_layout = &m_layoutCache.GetLayout({ std::move(bindings) });

        std::vector<vk::DescriptorSetLayout> layouts(m_inFlightCount, **set.m_layout);

        vk::DescriptorSetAllocateInfo allocInfo {};
        allocInfo.descriptorPool = *m_pool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
        allocInfo.pSetLayouts = layouts.data();

        set.m_descriptorSets = (*device).allocateDescriptorSets(allocInfo);
    }
}

void CDescriptorPool::UpdateDescriptorSets(
    CBufferPool& bufferManager,
    CTexturePool& textureManager
) {
    auto& device = *m_context->Device();

    std::vector<vk::WriteDescriptorSet> descriptorWrites;

    std::deque<vk::DescriptorBufferInfo> bufferInfos;
    std::deque<vk::DescriptorImageInfo> imageInfos;

    for (auto& set : m_descriptorSets) {
        for (std::uint32_t frameIdx = 0; frameIdx < m_inFlightCount; ++frameIdx) {
            for (std::uint32_t bindingIdx = 0; bindingIdx < set.meta.m_descriptors.size(); ++bindingIdx) {
                const auto& desc = set.meta.m_descriptors[bindingIdx];

                vk::WriteDescriptorSet write {};
                write.dstSet = set.m_descriptorSets[frameIdx];
                write.dstBinding = bindingIdx;
                write.dstArrayElement = 0;
                write.descriptorType = g_descriptorTypeMap.at(desc.m_type);
                write.descriptorCount = 1;

                if (std::holds_alternative<BufferDescriptorInfo>(desc.m_info)) {
                    auto info = std::get<BufferDescriptorInfo>(desc.m_info);
                    CBuffer& buffer = bufferManager.GetBuffer(info.m_buffer, frameIdx);

                    vk::DescriptorBufferInfo bInfo {};
                    bInfo.buffer = *buffer;
                    bInfo.offset = 0;
                    bInfo.range = VK_WHOLE_SIZE;

                    bufferInfos.push_back(bInfo);
                    write.pBufferInfo = &bufferInfos.back();
                } else if (std::holds_alternative<SampledImageDescriptorInfo>(desc.m_info)) {
                    auto info = std::get<SampledImageDescriptorInfo>(desc.m_info);
                    CImage& image = textureManager.GetTexture(info.m_image, frameIdx);

                    vk::DescriptorImageInfo iInfo {};
                    iInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                    iInfo.imageView = image.View();
                    iInfo.sampler = info.m_sampler;

                    imageInfos.push_back(iInfo);
                    write.pImageInfo = &imageInfos.back();
                } else if (std::holds_alternative<StorageImageDescriptorInfo>(desc.m_info)) {
                    auto info = std::get<StorageImageDescriptorInfo>(desc.m_info);
                    CImage& image = textureManager.GetTexture(info.m_image, frameIdx);

                    vk::DescriptorImageInfo iInfo {};
                    iInfo.imageLayout = vk::ImageLayout::eGeneral;
                    iInfo.imageView = image.View();

                    imageInfos.push_back(iInfo);
                    write.pImageInfo = &imageInfos.back();
                }

                descriptorWrites.push_back(write);
            }
        }
    }

    if (!descriptorWrites.empty()) {
        device.updateDescriptorSets(descriptorWrites, nullptr);
    }
}

vk::DescriptorSet CDescriptorPool::GetDescriptorSet(DescriptorSetHandle handle, int index) {
    return m_descriptorSets.at(handle.m_id).m_descriptorSets.at((index == -1) ? m_frameIndex : static_cast<std::uint32_t>(index));
}

const vk::raii::DescriptorSetLayout* CDescriptorPool::GetDescriptorSetLayout(DescriptorSetHandle handle) {
    return m_descriptorSets.at(handle.m_id).m_layout;
}
}

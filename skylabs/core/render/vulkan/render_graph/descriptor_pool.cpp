#include <skylabs/core/render/vulkan/render_graph/descriptor_pool.hpp>

#include <deque>

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
    std::unordered_map<vk::DescriptorType, unsigned int> typeCount;
    unsigned int totalTypes = 0;
    unsigned int totalSets = 0;

    for (const auto& set : m_descriptorSets) {
        totalSets += m_inFlightCount;
        for (const auto& desc : set.meta.m_descriptors) {
            typeCount[desc.m_type] += m_inFlightCount * desc.m_count;
            totalTypes += typeCount[desc.m_type];
        }
    }

    std::vector<vk::DescriptorPoolSize> poolSizes;
    poolSizes.reserve(totalTypes);

    for (auto const& [type, count] : typeCount) {
        poolSizes.emplace_back(type, count);
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
        for (const auto & desc : set.meta.m_descriptors) {
            vk::DescriptorSetLayoutBinding binding {};
            binding.binding = desc.m_binding;
            binding.descriptorType = desc.m_type;
            binding.descriptorCount = desc.m_count;
            binding.stageFlags = desc.m_shaderStages;

            bindings.push_back(binding);
        }

        set.m_layout = &m_layoutCache.GetLayout({ std::move(bindings) });

        std::vector<vk::DescriptorSetLayout> layouts(m_inFlightCount, **set.m_layout);

        vk::DescriptorSetAllocateInfo allocInfo {};
        allocInfo.descriptorPool = *m_pool;
        allocInfo.setSetLayouts(layouts);

        set.m_descriptorSets = (*device).allocateDescriptorSets(allocInfo);
    }
}

void CDescriptorPool::BindTextureToIndex(
    DescriptorSetHandle setHandle,
    std::uint32_t binding,
    std::uint32_t arrayIndex,
    const CImage& image,
    vk::Sampler sampler
) {
    auto& set = m_descriptorSets.at(setHandle.m_id);

    for (std::uint32_t i = 0; i < m_inFlightCount; ++i) {
        vk::DescriptorImageInfo iInfo {};
        iInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        iInfo.imageView = image.View();
        iInfo.sampler = sampler;

        vk::WriteDescriptorSet write {};
        write.dstSet = set.m_descriptorSets[i];
        write.dstBinding = binding;
        write.dstArrayElement = arrayIndex;
        write.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        write.descriptorCount = 1;
        write.pImageInfo = &iInfo;

        m_context->Device()->updateDescriptorSets(write, nullptr);
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
            for (const auto& desc : set.meta.m_descriptors) {
                if (std::holds_alternative<std::monostate>(desc.m_info))
                    continue;

                vk::WriteDescriptorSet write {};
                write.dstSet = set.m_descriptorSets[frameIdx];
                write.dstBinding = desc.m_binding;
                write.dstArrayElement = 0;
                write.descriptorType = desc.m_type;
                write.descriptorCount = desc.m_count;

                if (std::holds_alternative<BufferDescriptorInfo>(desc.m_info)) {
                    auto info = std::get<BufferDescriptorInfo>(desc.m_info);
                    CBuffer& buffer = bufferManager.GetBuffer(info.m_buffer, frameIdx);

                    vk::DescriptorBufferInfo bInfo {};
                    bInfo.buffer = *buffer;
                    bInfo.offset = 0;
                    bInfo.range = vk::WholeSize;

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
                }  else if (std::holds_alternative<StorageBufferDescriptorInfo>(desc.m_info)) {
                    auto info = std::get<StorageBufferDescriptorInfo>(desc.m_info);

                    int targetFrame = (static_cast<int>(frameIdx) + info.m_frameOffset) % static_cast<int>(m_inFlightCount);
                    if (targetFrame < 0) targetFrame += m_inFlightCount;

                    CBuffer& buffer = bufferManager.GetBuffer(info.m_buffer, targetFrame);

                    vk::DescriptorBufferInfo bInfo {};
                    bInfo.buffer = *buffer;
                    bInfo.offset = 0;
                    bInfo.range = vk::WholeSize;

                    bufferInfos.push_back(bInfo);
                    write.pBufferInfo = &bufferInfos.back();
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

#include <skylabs/core/render/vulkan/render_graph/resource_manager.hpp>

#include <frozen/map.h>
#include <deque>

namespace {
constexpr frozen::map<Vulkan::RG::DescriptorType, vk::DescriptorType, 3> g_descriptorTypeMap
{
    { Vulkan::RG::DescriptorType::eUniformBuffer, vk::DescriptorType::eUniformBuffer },
    { Vulkan::RG::DescriptorType::eStorageImage, vk::DescriptorType::eStorageImage },
    { Vulkan::RG::DescriptorType::eCombinedImageSampler, vk::DescriptorType::eCombinedImageSampler },
};

constexpr frozen::map<Vulkan::RG::BufferUsageFlagBits, vk::BufferUsageFlagBits, 1> g_BufferUsageMap
{
    { Vulkan::RG::BufferUsageFlagBits::eUniformBuffer, vk::BufferUsageFlagBits::eUniformBuffer },
};

vk::BufferUsageFlags GetVkBufferUsageFlags(Vulkan::RG::BufferUsageFlags stage) {
    vk::BufferUsageFlags flags;
    for (auto const& [bit, vkBit] : g_BufferUsageMap) {
        if (stage & bit) {
            flags |= vkBit;
        }
    }
    return flags;
}
}

namespace Vulkan::RG {
CTextureManager::CTextureManager(
    const CContext& context,
    Utils::Extent2D viewportExtent,
    std::uint32_t inFlightCount
) : m_context(&context), m_viewportExtent(viewportExtent), m_inFlightCount(inFlightCount)
{ }

TextureHandle CTextureManager::CreateTexture(const char* debugName, const TextureDescirption& description) {
    TextureHandle handle { static_cast<unsigned int>(m_textures.size()) };

    TextureMeta meta {
        .m_debugName = debugName,
        .m_description = description
    };

    m_textures.emplace_back(meta);
    m_dirtyIndices.push_back(handle.m_id);
    return handle;
}

TextureHandle CTextureManager::ImportTexture(const char* debugName, CImage image) {
    TextureHandle handle{ static_cast<unsigned int>(m_textures.size()) };
    Texture entry;
    entry.m_info.m_debugName = debugName;
    entry.m_info.m_description.m_extent = vk::Extent3D { image.Extent().width, image.Extent().height, 1 };
    entry.m_images.push_back(std::move(image));
    m_textures.push_back(std::move(entry));
    return handle;
}

void CTextureManager::GenerateTextures() {
    const bool hasDebug =
        m_context->Instance().IsExtensionEnabled(vk::EXTDebugUtilsExtensionName);

    std::erase_if(m_dirtyIndices, [&](auto index) {
        auto& [meta, images] = m_textures.at(index);

        images.clear();
        images.reserve(m_inFlightCount);
        for (std::size_t i = 0; i < m_inFlightCount; ++i) {
            auto image = CreateImage(meta);
            if (hasDebug)
                m_context->Device()->setDebugUtilsObjectNameEXT(*image, fmt::format("{}-{}", meta.m_debugName, i));
            images.push_back(std::move(image));
        }

        return std::holds_alternative<vk::Extent3D>(meta.m_description.m_extent);
    });
}

CImage& CTextureManager::GetTexture(TextureHandle handle, int index) {
    auto& entry = m_textures.at(handle.m_id);

    if (entry.m_images.size() == 1) {
        return entry.m_images[0];
    }

    return entry.m_images.at((index == -1) ? m_frameIndex : static_cast<uint32_t>(index));
}

void CTextureManager::Resize(const Utils::Extent2D newViewportExtent) {
    m_viewportExtent = newViewportExtent;
    GenerateTextures();
}

CImage CTextureManager::CreateImage(const TextureMeta& meta) {
    TextureDescirption desc = meta.m_description;

    vk::Extent3D extent { 1, 1, 1 };
    if (std::holds_alternative<RelativeTextureSize>(meta.m_description.m_extent)) {
        auto textureSize = std::get<RelativeTextureSize>(meta.m_description.m_extent);
        extent.width = static_cast<std::uint32_t>(m_viewportExtent.m_width * textureSize.m_scaleX);
        extent.height = static_cast<std::uint32_t>(m_viewportExtent.m_height * textureSize.m_scaleY);
    } else {
        extent = std::get<vk::Extent3D>(meta.m_description.m_extent);
    }

    vk::Format format;
    switch (desc.m_format) {
        case TextureFormat::eRGBA8888Srgb: format = vk::Format::eR8G8B8A8Srgb; break;
        case TextureFormat::eRGBA8888Unorm: format = vk::Format::eR8G8B8A8Unorm; break;
        case TextureFormat::eDepthOptimal: format = vk::Format::eD32Sfloat; break; // TODO: Find formats
        default: std::unreachable();
    };

    vk::ImageAspectFlags aspects;
    vk::ImageUsageFlags usage;
    if (desc.m_usage & TextureUsageBits::eAttachment) {
        aspects |= vk::ImageAspectFlagBits::eColor;
        usage |= vk::ImageUsageFlagBits::eColorAttachment;
    }

    if (desc.m_usage & TextureUsageBits::eDepthAttachment) {
        aspects |= vk::ImageAspectFlagBits::eDepth;
        usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
    }

    if (desc.m_usage & TextureUsageBits::eSampled) {
        usage |= vk::ImageUsageFlagBits::eSampled;
    }

    if (desc.m_usage & TextureUsageBits::eStorage) {
        aspects |= vk::ImageAspectFlagBits::eColor;
        usage |= vk::ImageUsageFlagBits::eStorage;
    }

    vk::SampleCountFlagBits sampleCount = desc.m_sampled ? vk::SampleCountFlagBits::e8 : vk::SampleCountFlagBits::e1;
    std::uint32_t mipLevels = desc.m_mipLevels;

    return CImage {
        *m_context,
        extent,
        format,
        usage,
        aspects,
        mipLevels,
        sampleCount
    };
}


CBufferManager::CBufferManager(
    const CContext& context,
    std::uint32_t inFlightCount
) : m_context(&context), m_inFlightCount(inFlightCount)
{ }

BufferHandle CBufferManager::CreateBuffer(const char* debugName, const BufferDescirption& description) {
    BufferHandle handle { static_cast<unsigned int>(m_buffers.size()) };
    m_buffers.emplace_back(BufferMeta { .m_debugName = debugName, .m_description = description });
    return handle;
}

BufferHandle CBufferManager::ImportBuffer(const char* debugName, CBuffer buffer) {
    BufferHandle handle{ static_cast<unsigned int>(m_buffers.size()) };
    Buffer entry;
    entry.m_meta.m_debugName = debugName;
    entry.m_meta.m_description.m_size = buffer.Size();
    entry.m_meta.m_description.m_isInFlight = false;
    entry.m_buffers.push_back(std::move(buffer));
    m_buffers.push_back(std::move(entry));
    return handle;
}

void CBufferManager::GenerateBuffers() {
    const bool hasDebug =
        m_context->Instance().IsExtensionEnabled(vk::EXTDebugUtilsExtensionName);

    auto createBuffer = [this](BufferMeta meta) {
        return CBuffer {
            *m_context,
            meta.m_description.m_size,
            GetVkBufferUsageFlags(meta.m_description.m_usage),
            meta.m_description.m_location
        };
    };

    for (auto& [meta, buffers] : m_buffers) {
        if (!buffers.empty())
            continue;

        if (meta.m_description.m_isInFlight) {
            buffers.clear();
            buffers.reserve(m_inFlightCount);

            for (std::size_t i = 0; i < m_inFlightCount; ++i) {
                CBuffer buffer = createBuffer(meta);

                if (hasDebug)
                    m_context->Device()->setDebugUtilsObjectNameEXT(**buffer, fmt::format("{}-{}", meta.m_debugName, i));

                buffers.push_back(std::move(buffer));
            }
        } else {
            buffers.push_back(createBuffer(meta));
        }
    }
}

CBuffer& CBufferManager::GetBuffer(BufferHandle handle, int index) {
    auto& entry = m_buffers.at(handle.m_id);

    if (entry.m_buffers.size() == 1) {
        return entry.m_buffers.at(0);
    }

    return entry.m_buffers.at((index == -1) ? m_frameIndex : static_cast<std::uint32_t>(index));
}


CDescriptorManager::CDescriptorManager(
    const CContext& context,
    std::uint32_t inFlightCount
) : m_context(&context), m_inFlightCount(inFlightCount), m_layoutCache(context)
{ }

DescriptorSetHandle CDescriptorManager::CreateDescriptorSet(std::span<const DescriptorDescription> descriptors) {
    DescriptorSetHandle handle { static_cast<unsigned int>(m_descriptorSets.size()) };
    m_descriptorSets.emplace_back(DescriptorSetMeta { .m_descriptors = std::vector(descriptors.begin(), descriptors.end()) });
    return handle;
}

void CDescriptorManager::CreateDescriptorPool() {
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
            poolSizes.push_back({ vkType, count });
        }
    }

    if (poolSizes.empty()) return;

    vk::DescriptorPoolCreateInfo poolInfo {};
    poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    poolInfo.maxSets = totalSets;
    poolInfo.poolSizeCount = static_cast<std::uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    m_pool = vk::raii::DescriptorPool { *m_context->Device(), poolInfo };
}

void CDescriptorManager::CreateDescriptorSets() {
    auto& device = *m_context->Device();

for (auto& set : m_descriptorSets) {
        // 1. Собираем биндинги из описаний (метаданных)
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        for (uint32_t i = 0; i < set.meta.m_descriptors.size(); ++i) {
            const auto& desc = set.meta.m_descriptors[i];

            vk::DescriptorSetLayoutBinding binding {};
            binding.binding = i;
            binding.descriptorType = g_descriptorTypeMap.at(desc.m_type);
            binding.descriptorCount = 1;
            binding.stageFlags = desc.m_shaderStages;

            bindings.push_back(binding);
        }

        set.m_layout = &m_layoutCache.GetLayout(std::move(bindings));

        std::vector<vk::DescriptorSetLayout> layouts(m_inFlightCount, **set.m_layout);

        vk::DescriptorSetAllocateInfo allocInfo {};
        allocInfo.descriptorPool = *m_pool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
        allocInfo.pSetLayouts = layouts.data();

        set.m_descriptorSets = (*device).allocateDescriptorSets(allocInfo);
    }
}

void CDescriptorManager::UpdateDescriptorSets(
    CBufferManager& bufferManager,
    CTextureManager& textureManager
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

                std::visit([&](auto&& info) {
                    using T = std::decay_t<decltype(info)>;

                    if constexpr (std::is_same_v<T, BufferDescriptorInfo>) {
                        CBuffer& buffer = bufferManager.GetBuffer(info.m_buffer, frameIdx);

                        vk::DescriptorBufferInfo bInfo {};
                        bInfo.buffer = *buffer;
                        bInfo.offset = 0;
                        bInfo.range = VK_WHOLE_SIZE;

                        bufferInfos.push_back(bInfo);
                        write.pBufferInfo = &bufferInfos.back();
                    }
                    else if constexpr (std::is_same_v<T, SampledImageDescriptorInfo>) {
                        CImage& image = textureManager.GetTexture(info.m_image, frameIdx);

                        vk::DescriptorImageInfo iInfo {};
                        iInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                        iInfo.imageView = image.View();
                        iInfo.sampler = **info.m_sampler;

                        imageInfos.push_back(iInfo);
                        write.pImageInfo = &imageInfos.back();
                    }
                    else if constexpr (std::is_same_v<T, StorageImageDescriptorInfo>) {
                        CImage& image = textureManager.GetTexture(info.m_image, frameIdx);

                        vk::DescriptorImageInfo iInfo {};
                        iInfo.imageLayout = vk::ImageLayout::eGeneral;
                        iInfo.imageView = image.View();

                        imageInfos.push_back(iInfo);
                        write.pImageInfo = &imageInfos.back();
                    }
                }, desc.m_info);

                descriptorWrites.push_back(write);
            }
        }
    }

    if (!descriptorWrites.empty()) {
        device.updateDescriptorSets(descriptorWrites, nullptr);
    }
}

vk::DescriptorSet CDescriptorManager::GetDescriptorSet(DescriptorSetHandle handle, int index) {
    return m_descriptorSets.at(handle.m_id).m_descriptorSets.at((index == -1) ? m_frameIndex : static_cast<std::uint32_t>(index));
}

const vk::raii::DescriptorSetLayout* CDescriptorManager::GetDescriptorSetLayout(DescriptorSetHandle handle) {
    return m_descriptorSets.at(handle.m_id).m_layout;
}
}

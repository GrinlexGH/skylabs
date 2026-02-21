#include <skylabs/core/render/vulkan/render_graph/resource_manager.hpp>

#include <skylabs/public/logging.hpp>

namespace Vulkan::RG {
CResourceManager::CResourceManager(
    const CContext& context,
    Utils::Extent2D viewportExtent,
    std::uint32_t inFlightCount
) : m_context(&context), m_viewportExtent(viewportExtent), m_inFlightCount(inFlightCount)
{ }

TextureHandle CResourceManager::CreateEmptyTexture(const char* debugName, const TextureDescirption& description) {
    TextureHandle handle { static_cast<unsigned int>(m_textures.size()) };

    TextureMeta meta;
    meta.m_debugName = debugName;
    meta.m_format = description.m_format;
    meta.m_usage = description.m_usage;
    meta.m_sampled = description.m_sampled;
    meta.m_source = TextureMeta::EmptySource {
        .m_extent = description.m_extent,
        .m_mipLevels = description.m_mipLevels
    };

    m_textures.emplace_back(meta);
    return handle;
}

void CResourceManager::GenerateTextures() {
    for (auto& [meta, images] : m_textures) {
        if (meta.m_dirty) {
            images.reserve(m_inFlightCount);
            for (std::size_t i = 0; i < m_inFlightCount; i++) {
                if (images.size() < m_inFlightCount) {
                    images.push_back(CreateImage(meta));
                } else {
                    images.at(i) = CreateImage(meta);
                }
            }

            meta.m_dirty = false;
        }
    }
}

CImage& CResourceManager::GetTexture(TextureHandle handle) {
    return m_textures.at(handle.m_id).m_images.at(m_frameIndex);
}


DescriptorHandle CResourceManager::CreateUniformBuffer(const char* debugName, std::size_t size) {
    DescriptorHandle handle { static_cast<unsigned int>(m_creationPendingUniformBuffers.size()) };

    UniformBufferMeta meta;
    meta.m_debugName = debugName;
    meta.m_size = size;

    m_creationPendingUniformBuffers.try_emplace(handle.m_id, meta);

    m_descriptorRequirements.uniformBuffers += m_inFlightCount;

    return handle;
}

void CResourceManager::GenerateDescriptorObjects() {
    for (auto& [id, desc] : m_creationPendingUniformBuffers) {
        std::vector<CHostBuffer> ubos;
        ubos.reserve(m_inFlightCount);
        for (std::size_t _ = 0; _ < m_inFlightCount; _++) {
            ubos.emplace_back(*m_context, static_cast<vk::DeviceSize>(desc.m_size), vk::BufferUsageFlagBits::eUniformBuffer);
        }

        m_uniformBuffers.insert_or_assign(id, std::move(ubos));
    }
}

CHostBuffer& CResourceManager::GetUniformBuffer(DescriptorHandle handle, int index) {
    return m_uniformBuffers.at(handle.m_id).at(index == -1 ? m_frameIndex : index);
}

void CResourceManager::BuildDescriptorPool() {
    std::vector<vk::DescriptorPoolSize> poolSizes;

    if (m_descriptorRequirements.uniformBuffers > 0) {
        poolSizes.emplace_back(vk::DescriptorType::eUniformBuffer, m_descriptorRequirements.uniformBuffers);
    }
    if (m_descriptorRequirements.combinedSamplers > 0) {
        poolSizes.emplace_back(vk::DescriptorType::eCombinedImageSampler, m_descriptorRequirements.combinedSamplers);
    }
    if (m_descriptorRequirements.storageImages > 0) {
        poolSizes.emplace_back(vk::DescriptorType::eStorageImage, m_descriptorRequirements.storageImages);
    }

    vk::DescriptorPoolCreateInfo poolInfo {};
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = m_inFlightCount * 3; //! HARDCODE!!!

    m_descriptorPool = vk::raii::DescriptorPool { *m_context->Device(), poolInfo };
}

void CResourceManager::Resize(const Utils::Extent2D newViewportExtent) {
    m_viewportExtent = newViewportExtent;

    for (auto& [meta, images] : m_textures) {
        std::visit([&](auto&& source) {
            using T = std::decay_t<decltype(source)>;

            if constexpr (std::is_same_v<T, TextureMeta::EmptySource>) {
                std::visit([&](auto&& textureSize) {
                    using T = std::decay_t<decltype(textureSize)>;

                    if constexpr (std::is_same_v<T, RelativeTextureSize>) {
                        meta.m_dirty = true;
                    }
                }, source.m_extent);
            }
        }, meta.m_source);
    }

    GenerateTextures();
}

CImage CResourceManager::CreateImage(const TextureMeta& desc) {
    vk::Extent2D extent;
    std::uint32_t mipLevels;

    std::visit([&](auto&& source) {
        using T = std::decay_t<decltype(source)>;

        if constexpr (std::is_same_v<T, TextureMeta::EmptySource>) {
            mipLevels = source.m_mipLevels;

            std::visit([&](auto&& textureSize) {
                using T = std::decay_t<decltype(textureSize)>;

                if constexpr (std::is_same_v<T, RelativeTextureSize>) {
                    // TODO: scale
                    extent.width = m_viewportExtent.m_width;
                    extent.height = m_viewportExtent.m_height;
                } else if constexpr (std::is_same_v<T, AbsoluteTextureSize>) {
                    extent.width = textureSize.m_width;
                    extent.height = textureSize.m_height;
                }
            }, source.m_extent);
        }
    }, desc.m_source);

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
        aspects |= vk::ImageAspectFlagBits::eColor;
        usage |= vk::ImageUsageFlagBits::eSampled;
    }

    if (desc.m_usage & TextureUsageBits::eStorage) {
        aspects |= vk::ImageAspectFlagBits::eColor;
        usage |= vk::ImageUsageFlagBits::eStorage;
    }

    vk::SampleCountFlagBits sampleCount = desc.m_sampled ? vk::SampleCountFlagBits::e8 : vk::SampleCountFlagBits::e1;

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
}

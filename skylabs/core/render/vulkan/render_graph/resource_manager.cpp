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
    TextureHandle handle { static_cast<unsigned int>(m_creationPendingTextures.size()) };

    TextureMeta meta;
    meta.m_debugName = debugName;
    meta.m_format = description.m_format;
    meta.m_usage = description.m_usage;
    meta.m_sampled = description.m_sampled;
    meta.m_source = TextureMeta::EmptySource { .m_extent = description.m_extent, .m_mipLevels = description.m_mipLevels };

    m_creationPendingTextures.try_emplace(handle.m_id, meta);

    return handle;
}

TextureHandle CResourceManager::CreateAssetTexture(const char* debugName, const AssetTextureDescirption& description) {
    TextureHandle handle { static_cast<unsigned int>(m_creationPendingTextures.size()) };

    TextureMeta meta;
    meta.m_debugName = debugName;
    meta.m_format = description.m_format;
    meta.m_usage = TextureUsageBits::eSampled;
    meta.m_sampled = description.m_sampled;
    meta.m_source = TextureMeta::AssetSource { .m_assetFileName = description.m_assetFileName, .m_mipmapped = description.m_mipmapped };

    m_creationPendingTextures.try_emplace(handle.m_id, meta);

    return handle;
}

void CResourceManager::GenerateTextures() {
    for (auto& [id, desc] : m_creationPendingTextures) {
        if (desc.m_dirty) {
            std::vector<CImage> images;
            images.reserve(m_inFlightCount);
            for (std::size_t _ = 0; _ < m_inFlightCount; _++) {
                images.push_back(CreateImage(desc));
            }

            m_textures.insert_or_assign(id, std::move(images));
            desc.m_dirty = false;
        }
    }
}

CImage& CResourceManager::GetTexture(TextureHandle handle) {
    TextureMeta imgMeta = m_creationPendingTextures.at(handle.m_id);

    bool isAssetSource = false;
    std::visit([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, TextureMeta::AssetSource>) {
            isAssetSource = true;
        }
    }, imgMeta.m_source);

    return m_textures.at(handle.m_id).at(m_frameIndex);
}

void CResourceManager::Resize(const Utils::Extent2D newViewportExtent) {
    m_viewportExtent = newViewportExtent;

    for (auto& [id, desc] : m_creationPendingTextures) {
        std::visit([&](auto&& source) {
            using T = std::decay_t<decltype(source)>;

            if constexpr (std::is_same_v<T, TextureMeta::EmptySource>) {
                std::visit([&](auto&& textureSize) {
                    using T = std::decay_t<decltype(textureSize)>;

                    if constexpr (std::is_same_v<T, RelativeTextureSize>) {
                        desc.m_dirty = true;
                    }
                }, source.m_extent);
            }
        }, desc.m_source);
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
        } else if constexpr (std::is_same_v<T, TextureMeta::AssetSource>) {
            Log::Debug("Asset loading not implemented");
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

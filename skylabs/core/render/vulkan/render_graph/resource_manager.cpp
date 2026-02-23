#include <skylabs/core/render/vulkan/render_graph/resource_manager.hpp>

#include <skylabs/public/logging.hpp>

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

void CTextureManager::GenerateTextures() {
    const bool hasDebug =
        m_context->Instance().IsExtensionEnabled(vk::EXTDebugUtilsExtensionName);

    for (auto index : m_dirtyIndices) {
        auto& [meta, images] = m_textures.at(index);

        images.clear();
        images.reserve(m_inFlightCount);

        for (std::size_t i = 0; i < m_inFlightCount; ++i) {
            auto image = CreateImage(meta);

            if (hasDebug) {
                m_context->Device()->setDebugUtilsObjectNameEXT(
                    *image,
                    fmt::format("{}-{}", meta.m_debugName, i)
                );
            }

            images.push_back(std::move(image));
        }
    }
}

CImage& CTextureManager::GetTexture(TextureHandle handle, int index) {
    return m_textures.at(handle.m_id).m_images.at(index == -1 ? m_frameIndex : index);
}

void CTextureManager::Resize(const Utils::Extent2D newViewportExtent) {
    m_viewportExtent = newViewportExtent;
    GenerateTextures();
}

CImage CTextureManager::CreateImage(const TextureMeta& meta) {
    TextureDescirption desc = meta.m_description;

    vk::Extent2D extent;

    std::visit([&](auto&& textureSize) {
        using T = std::decay_t<decltype(textureSize)>;

        if constexpr (std::is_same_v<T, RelativeTextureSize>) {
            extent.width = m_viewportExtent.m_width * textureSize.m_scaleX;
            extent.height = m_viewportExtent.m_height * textureSize.m_scaleY;
        } else if constexpr (std::is_same_v<T, AbsoluteTextureSize>) {
            extent.width = textureSize.m_width;
            extent.height = textureSize.m_height;
        }
    }, desc.m_extent);

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

CDescriptorManager::CDescriptorManager(
    const CContext& context,
    std::uint32_t inFlightCount
) : m_context(&context), m_inFlightCount(inFlightCount)
{ }

void CDescriptorManager::CreateDescriptorSet(std::span<const DescriptorDescription> descriptors) {

}
}

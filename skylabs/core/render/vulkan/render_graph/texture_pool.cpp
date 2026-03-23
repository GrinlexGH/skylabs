#include <skylabs/core/render/vulkan/render_graph/texture_pool.hpp>

namespace Vulkan {
CTexturePool::CTexturePool(
    const CContext& context,
    vk::Extent2D viewportExtent,
    std::uint32_t inFlightCount
) : m_context(&context), m_viewportExtent(viewportExtent), m_inFlightCount(inFlightCount)
{ }

TextureHandle CTexturePool::CreateTexture(const char* debugName, const TextureDescirption& description) {
    TextureHandle handle { static_cast<unsigned int>(m_textures.size()) };

    TextureMeta meta {
        .m_debugName = debugName,
        .m_description = description
    };

    m_textures.emplace_back(meta);
    m_dirtyIndices.push_back(handle.m_id);
    return handle;
}

TextureHandle CTexturePool::ImportTexture(const char* debugName, CImage image) {
    TextureHandle handle{ static_cast<unsigned int>(m_textures.size()) };
    Texture entry;
    entry.m_info.m_debugName = debugName;
    entry.m_info.m_description.m_extent = vk::Extent3D { image.Extent().width, image.Extent().height, 1 };
    entry.m_images.push_back(std::move(image));
    m_textures.push_back(std::move(entry));
    return handle;
}

void CTexturePool::GenerateTextures() {
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

        return !std::holds_alternative<RelativeTextureSize>(meta.m_description.m_extent);
    });
}

CImage& CTexturePool::GetTexture(TextureHandle handle, int index) {
    auto& entry = m_textures.at(handle.m_id);

    if (entry.m_images.size() == 1) {
        return entry.m_images[0];
    }

    return entry.m_images[(index == -1) ? m_frameIndex : static_cast<uint32_t>(index)];
}

void CTexturePool::Resize(const vk::Extent2D newViewportExtent) {
    m_viewportExtent = newViewportExtent;
    GenerateTextures();
}

CImage CTexturePool::CreateImage(const TextureMeta& meta) {
    TextureDescirption desc = meta.m_description;

    vk::Extent3D extent { 1, 1, 1 };
    if (std::holds_alternative<RelativeTextureSize>(meta.m_description.m_extent)) {
        auto textureSize = std::get<RelativeTextureSize>(meta.m_description.m_extent);
        extent.width = static_cast<std::uint32_t>(m_viewportExtent.width * textureSize.m_scaleX);
        extent.height = static_cast<std::uint32_t>(m_viewportExtent.height * textureSize.m_scaleY);
    } else {
        extent = std::get<vk::Extent3D>(meta.m_description.m_extent);
    }

    return CImage { *m_context, { extent, desc.m_format, desc.m_mipLevels, desc.m_arrayLevels, desc.m_sampleCount, desc.m_usage } };
}
}

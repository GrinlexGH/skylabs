#include <skylabs/core/render/vulkan/render_graph/resource_manager.hpp>

namespace Vulkan::RG {
CResourceManager::CResourceManager(const CContext& context) : m_context(&context) { }

TextureHandle CResourceManager::CreateEmptyTexture(TextureDescirption description) {
    TextureHandle handle { static_cast<unsigned int>(m_creationPendingTextures.size()) };

    TextureMeta meta;
    meta.m_format = description.m_format;
    meta.m_usage = description.m_usage;
    meta.m_sampled = description.m_sampled;

    meta.m_source = TextureMeta::EmptySource { description.m_extent, description.m_mipLevels };

    m_creationPendingTextures.try_emplace(handle.m_id, meta);

    return handle;
}

TextureHandle CResourceManager::CreateAssetTexture(AssetTextureDescirption description) {
    TextureHandle handle { static_cast<unsigned int>(m_creationPendingTextures.size()) };

    TextureMeta meta;
    meta.m_format = description.m_format;
    meta.m_usage = TextureUsageBits::eSampled;
    meta.m_sampled = description.m_sampled;

    meta.m_source = TextureMeta::AssetSource { description.m_assetFileName, description.m_mipmapped };

    m_creationPendingTextures.try_emplace(handle.m_id, meta);

    return handle;
}

void CResourceManager::GenerateTextures() {
    for (const auto& [id, desc] : m_creationPendingTextures) {
        std::vector<CImage> images;
        images.reserve(1);
        images.emplace_back(nullptr);
        m_textures.try_emplace(id, std::move(images));
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

    return m_textures.at(handle.m_id).at(isAssetSource ? 0 : m_frameIndex);
}
}

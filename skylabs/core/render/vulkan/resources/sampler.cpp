#include <skylabs/core/render/vulkan/resources/sampler.hpp>

namespace Vulkan {
CSampler::CSampler(
    const CContext& context,
    SamplerCreateInfo options
) {
    vk::SamplerCreateInfo createInfo {};

    if (std::holds_alternative<vk::Filter>(options.m_filtering)) {
        auto filtering = std::get<vk::Filter>(options.m_filtering);
        createInfo.magFilter = filtering;
        createInfo.minFilter = filtering;
    } else {
        auto filtering = std::get<SamplerCreateInfo::MinMagFilter>(options.m_filtering);
        createInfo.magFilter = filtering.m_mag;
        createInfo.minFilter = filtering.m_min;
    }

    if (options.m_anisotropy.has_value()) {
        assert(context.Device().Caps().m_samplerAnisotropy);
        createInfo.anisotropyEnable = vk::True;
        createInfo.maxAnisotropy = static_cast<float>(*options.m_anisotropy);
    }

    if (std::holds_alternative<vk::SamplerAddressMode>(options.m_addressMode)) {
        auto addressMode = std::get<vk::SamplerAddressMode>(options.m_addressMode);
        createInfo.addressModeU = addressMode;
        createInfo.addressModeV = addressMode;
        createInfo.addressModeW = addressMode;
    } else {
        auto addressMode = std::get<SamplerCreateInfo::AddressMode>(options.m_addressMode);
        createInfo.addressModeU = addressMode.m_u;
        createInfo.addressModeV = addressMode.m_v;
        createInfo.addressModeW = addressMode.m_w;
    }

    createInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    createInfo.unnormalizedCoordinates = vk::False;

    if (options.m_compareOp.has_value()) {
        createInfo.compareEnable = vk::True;
        createInfo.compareOp = *options.m_compareOp;
    }

    createInfo.mipmapMode = options.m_mipmapFiltering;
    createInfo.mipLodBias = options.m_mipMapLevels.m_bias;
    createInfo.minLod = options.m_mipMapLevels.m_min;
    createInfo.maxLod = options.m_mipMapLevels.m_max;

    m_handle = vk::raii::Sampler { *context.Device(), createInfo };
}
}

#include "skylabs/core/render/vulkan/resources/sampler.hpp"

namespace sk::render::vulkan {
Sampler::Sampler(const Device& device, const SamplerCreateInfo& options) {
    vk::SamplerCreateInfo createInfo { };

    if (std::holds_alternative<vk::Filter>(options.filtering)) {
        auto filtering = std::get<vk::Filter>(options.filtering);
        createInfo.magFilter = filtering;
        createInfo.minFilter = filtering;
    } else {
        auto filtering = std::get<SamplerCreateInfo::MinMagFilter>(options.filtering);
        createInfo.magFilter = filtering.mag;
        createInfo.minFilter = filtering.min;
    }

    if (options.anisotropy.has_value()) {
        assert(device.Caps().samplerAnisotropy);
        createInfo.anisotropyEnable = vk::True;
        createInfo.maxAnisotropy = static_cast<float>(*options.anisotropy);
    }

    if (std::holds_alternative<vk::SamplerAddressMode>(options.addressMode)) {
        auto addressMode = std::get<vk::SamplerAddressMode>(options.addressMode);
        createInfo.addressModeU = addressMode;
        createInfo.addressModeV = addressMode;
        createInfo.addressModeW = addressMode;
    } else {
        auto addressMode = std::get<SamplerCreateInfo::AddressMode>(options.addressMode);
        createInfo.addressModeU = addressMode.u;
        createInfo.addressModeV = addressMode.v;
        createInfo.addressModeW = addressMode.w;
    }

    createInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    createInfo.unnormalizedCoordinates = vk::False;

    if (options.compareOp.has_value()) {
        createInfo.compareEnable = vk::True;
        createInfo.compareOp = *options.compareOp;
    }

    createInfo.mipmapMode = options.mipmapFiltering;
    createInfo.mipLodBias = options.mipMapLevels.bias;
    createInfo.minLod = options.mipMapLevels.min;
    createInfo.maxLod = options.mipMapLevels.max;

    m_handle = vk::raii::Sampler { *device, createInfo };
}
}

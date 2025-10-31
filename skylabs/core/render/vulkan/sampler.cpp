#include <skylabs/core/render/vulkan/sampler.hpp>

namespace Vulkan {
CSampler::CSampler(const CContext& context) {
    vk::SamplerCreateInfo createInfo {};
    createInfo.magFilter = vk::Filter::eNearest;
    createInfo.minFilter = vk::Filter::eNearest;
    createInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    createInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    createInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    createInfo.anisotropyEnable = vk::True;
    createInfo.maxAnisotropy = context.GetPhysicalDevice()->GetProperties().limits.maxSamplerAnisotropy;
    createInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    createInfo.unnormalizedCoordinates = vk::False;
    createInfo.compareEnable = vk::False;
    createInfo.compareOp = vk::CompareOp::eAlways;
    createInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    createInfo.mipLodBias = 0.0f;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = 0.0f;
    m_handle = (*context.GetDevice()).createSampler(createInfo);
}
}

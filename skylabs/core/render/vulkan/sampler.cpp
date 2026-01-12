#include <skylabs/core/render/vulkan/sampler.hpp>

namespace Vulkan {
CSampler::CSampler(const CContext& context) {
    vk::SamplerCreateInfo createInfo {};
    createInfo.magFilter = vk::Filter::eNearest;
    createInfo.minFilter = vk::Filter::eNearest;
    createInfo.addressModeU = vk::SamplerAddressMode::eClampToBorder;
    createInfo.addressModeV = vk::SamplerAddressMode::eClampToBorder;
    createInfo.addressModeW = vk::SamplerAddressMode::eClampToBorder;
    createInfo.anisotropyEnable = vk::True;
    createInfo.maxAnisotropy = context.PhysicalDevice().getProperties2KHR().properties.limits.maxSamplerAnisotropy;
    createInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    createInfo.unnormalizedCoordinates = vk::False;
    createInfo.compareEnable = vk::False;
    createInfo.compareOp = vk::CompareOp::eAlways;
    createInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    createInfo.mipLodBias = 0.0f;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = vk::LodClampNone;
    m_handle = vk::raii::Sampler { *context.Device(), createInfo };
}
}

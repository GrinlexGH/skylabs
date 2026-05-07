#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

#include <variant>

namespace Vulkan {
struct SamplerCreateInfo {
    struct MinMagFilter {
        vk::Filter m_min = vk::Filter::eNearest;
        vk::Filter m_mag = vk::Filter::eNearest;
    };

    enum class Anisotropy : std::uint8_t { e2 = 2, e4 = 4, e8 = 8, e16 = 16 };

    struct AddressMode {
        vk::SamplerAddressMode m_u = vk::SamplerAddressMode::eRepeat;
        vk::SamplerAddressMode m_v = vk::SamplerAddressMode::eRepeat;
        vk::SamplerAddressMode m_w = vk::SamplerAddressMode::eRepeat;
    };

    struct MipMapLevels {
        float m_bias = 0.0f;
        float m_min = 0.0f;
        float m_max = vk::LodClampNone;
    };

    std::variant<vk::Filter, MinMagFilter> m_filtering = vk::Filter::eNearest;
    std::optional<Anisotropy> m_anisotropy = std::nullopt;
    std::variant<vk::SamplerAddressMode, AddressMode> m_addressMode = vk::SamplerAddressMode::eRepeat;
    std::optional<vk::CompareOp> m_compareOp = std::nullopt;
    vk::SamplerMipmapMode m_mipmapFiltering = vk::SamplerMipmapMode::eNearest;
    MipMapLevels m_mipMapLevels = {};
};

class CSampler
{
public:
    explicit CSampler(std::nullptr_t) {}
    explicit CSampler(const CDeviceContext& context, SamplerCreateInfo options = {});
    CSampler(const CSampler&) = delete;
    CSampler(CSampler&&) noexcept = default;
    CSampler& operator=(const CSampler&) = delete;
    CSampler& operator=(CSampler&&) noexcept = default;
    ~CSampler() = default;

    [[nodiscard]] const vk::raii::Sampler& operator*() const noexcept { return m_handle; }

private:
    vk::raii::Sampler m_handle = nullptr;
};
}

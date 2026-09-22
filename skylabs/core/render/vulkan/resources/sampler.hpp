#pragma once
#include <variant>

#include "skylabs/core/render/vulkan/context/device.hpp"

namespace sk::render::vulkan {
struct SamplerCreateInfo {
    struct MinMagFilter {
        vk::Filter min = vk::Filter::eNearest;
        vk::Filter mag = vk::Filter::eNearest;
    };

    enum class Anisotropy : std::uint8_t { e2 = 2, e4 = 4, e8 = 8, e16 = 16 };

    struct AddressMode {
        vk::SamplerAddressMode u = vk::SamplerAddressMode::eRepeat;
        vk::SamplerAddressMode v = vk::SamplerAddressMode::eRepeat;
        vk::SamplerAddressMode w = vk::SamplerAddressMode::eRepeat;
    };

    struct MipMapLevels {
        float bias = 0.0f;
        float min = 0.0f;
        float max = vk::LodClampNone;
    };

    std::variant<vk::Filter, MinMagFilter> filtering = vk::Filter::eNearest;
    std::optional<Anisotropy> anisotropy = std::nullopt;
    std::variant<vk::SamplerAddressMode, AddressMode> addressMode = vk::SamplerAddressMode::eRepeat;
    std::optional<vk::CompareOp> compareOp = std::nullopt;
    vk::SamplerMipmapMode mipmapFiltering = vk::SamplerMipmapMode::eNearest;
    MipMapLevels mipMapLevels = { };
};

class Sampler {
public:
    explicit Sampler(std::nullptr_t) { }
    explicit Sampler(const Device& device, const SamplerCreateInfo& options = { });
    Sampler(const Sampler&) = delete;
    Sampler(Sampler&&) noexcept = default;
    Sampler& operator=(const Sampler&) = delete;
    Sampler& operator=(Sampler&&) noexcept = default;
    ~Sampler() = default;

    [[nodiscard]] const vk::raii::Sampler& operator*() const noexcept { return m_handle; }

private:
    vk::raii::Sampler m_handle = nullptr;
};
}

#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
class CSampler
{
public:
    explicit CSampler(std::nullptr_t) {}
    explicit CSampler(const CContext& context);
    CSampler(const CSampler&) = delete;
    CSampler(CSampler&&) noexcept = default;
    CSampler& operator=(const CSampler&) = delete;
    CSampler& operator=(CSampler&&) noexcept = default;
    ~CSampler() = default;

    [[nodiscard]] auto operator*() const noexcept -> const vk::raii::Sampler& { return m_handle; }
    [[nodiscard]] auto GetHandle() const noexcept -> const vk::raii::Sampler& { return m_handle; }

private:
    vk::raii::Sampler m_handle = nullptr;
};
}

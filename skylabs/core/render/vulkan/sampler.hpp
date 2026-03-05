#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

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

    [[nodiscard]] const vk::raii::Sampler& operator*() const noexcept { return m_handle; }

private:
    vk::raii::Sampler m_handle = nullptr;
};
}

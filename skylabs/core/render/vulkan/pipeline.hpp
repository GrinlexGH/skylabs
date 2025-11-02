#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
class CPipeline
{
public:
    explicit CPipeline(const CContext& context);
    CPipeline(const CPipeline&) = delete;
    CPipeline(CPipeline&&) noexcept = default;
    CPipeline& operator=(const CPipeline&) = delete;
    CPipeline& operator=(CPipeline&&) noexcept = default;
    ~CPipeline() = default;

    auto operator*() const noexcept -> const vk::raii::Pipeline& { return m_handle; }
    [[nodiscard]] auto GetHandle() -> const vk::raii::Pipeline& { return m_handle; }

private:
    vk::raii::Pipeline m_handle { nullptr };
};
}

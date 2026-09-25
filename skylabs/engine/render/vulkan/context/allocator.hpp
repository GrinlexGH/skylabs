#pragma once
#include <vk_mem_alloc_raii.hpp>

#include "skylabs/engine/render/vulkan/context/device.hpp"

namespace sk::render::vulkan {
class Allocator {
public:
    explicit Allocator(std::nullptr_t) { }
    explicit Allocator(const vk::raii::Instance& instance, const Device& device);
    Allocator(const Allocator&) = delete;
    Allocator(Allocator&& other) noexcept = default;
    Allocator& operator=(const Allocator&) = delete;
    Allocator& operator=(Allocator&& rhs) noexcept = default;
    ~Allocator() = default;

    [[nodiscard]] const vma::raii::Allocator& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vma::raii::Allocator* operator->() const noexcept { return &m_handle; }

private:
    vma::raii::Allocator m_handle { nullptr };
};
}

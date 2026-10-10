#pragma once
#include <span>

#include <vulkan/vulkan.hpp>

namespace sk::render::vulkan {
class IOSAdapter {
public:
    virtual ~IOSAdapter() = default;

    [[nodiscard]] virtual PFN_vkGetInstanceProcAddr GetVkGetInstanceProcAddr() const = 0;

    /**
     * @remark Returned extension names should be available since this function first call until the end
     *         of the program.
     *
     * @return Span of required extension names.
     */
    [[nodiscard]] virtual std::span<const char* const> RequiredInstanceExtensions() const = 0;
    [[nodiscard]] virtual vk::SurfaceKHR CreateSurface(const vk::Instance& instance) const = 0;
};
}

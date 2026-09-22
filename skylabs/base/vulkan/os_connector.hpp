#pragma once
#include <span>

#include <vulkan/vulkan.hpp>

#include "sk_base_export.h"

namespace sk::vulkan {
class SK_BASE_PUBLIC_CLASS IOSConnector {
public:
    virtual ~IOSConnector() = default;

    [[nodiscard]] virtual PFN_vkGetInstanceProcAddr GetVkGetInstanceProcAddr() const = 0;
    [[nodiscard]] virtual std::span<const char* const> RequiredInstanceExtensions() const = 0;
    [[nodiscard]] virtual vk::SurfaceKHR CreateSurface(const vk::Instance& instance) const = 0;
};
}

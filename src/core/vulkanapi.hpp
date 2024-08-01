#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

class CVulkanAPI {
public:


private:
    vk::Instance instance_;
    vk::DebugUtilsMessengerEXT debugMessenger_;
    vk::SurfaceKHR surface_;
    vk::PhysicalDevice physicalDevice_;
    vk::Device device_;
    vk::Queue graphicsQueue_;
    vk::Queue presentQueue_;
};

#pragma once

#include "window.hpp"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

// Stores instance-specific objects like vk::Instance, vk::PhysicalDevice, etc
class CVulkanRendererContext {
public:
    CVulkanRendererContext() = default;

    void Initialize(IWindow* window);
    vk::Instance GetInstance() const;
    vk::SurfaceKHR GetSurface() const;
    IWindow* GetWindow() const;
    vk::DebugUtilsMessengerEXT GetDebugMessenger() const;

private:
    void InitializeInstanceExtensions();
    void InitializeInstance();
    std::vector<const char*> FindValidationLayers();
    vk::SurfaceKHR CreateSurface();

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagBitsEXT messageType,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    );

    vk::Instance m_instance {};
    std::unordered_map<std::string, bool> m_requestedInstanceExtensions {};
    std::unordered_set<std::string> m_enabledInstanceExtensions {};
    vk::DebugUtilsMessengerEXT m_debugMessenger {};
    vk::SurfaceKHR m_surface {};

    IWindow* m_window = nullptr;
};

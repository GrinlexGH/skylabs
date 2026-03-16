# core
set(CURRENT_TARGET_NAME core)

add_library(${CURRENT_TARGET_NAME} MODULE)
set_target_properties(${CURRENT_TARGET_NAME} PROPERTIES PREFIX "" IMPORT_PREFIX "")

target_sources(
    ${CURRENT_TARGET_NAME}
    PRIVATE
        main.cpp
        launcher.hpp
        launcher.cpp
        header_factory.cpp
        window.hpp
        camera.hpp
        render/renderer.hpp
        render/vulkan/platform/window.hpp
        render/vulkan/renderer.hpp
        render/vulkan/renderer.cpp
        render/vulkan/context/profile.hpp
        render/vulkan/context/profile.cpp
        render/vulkan/context/allocator.hpp
        render/vulkan/context/allocator.cpp
        render/vulkan/context/instance.hpp
        render/vulkan/context/instance.cpp
        render/vulkan/context/extensions.hpp
        render/vulkan/context/physical_device.hpp
        render/vulkan/context/physical_device.cpp
        render/vulkan/context/queue.hpp
        render/vulkan/context/device.hpp
        render/vulkan/context/device.cpp
        render/vulkan/context/context.hpp
        render/vulkan/context/context.cpp
        render/vulkan/platform/surface.hpp
        render/vulkan/platform/surface.cpp
        render/vulkan/platform/swapchain.hpp
        render/vulkan/platform/swapchain.cpp
        render/vulkan/frame_data.hpp
        render/vulkan/frame_data.cpp
        render/vulkan/pipeline/shader.hpp
        render/vulkan/pipeline/shader.cpp
        render/vulkan/resources/image.hpp
        render/vulkan/resources/image.cpp
        render/vulkan/resources/buffer.hpp
        render/vulkan/resources/buffer.cpp
        render/vulkan/resources/sampler.hpp
        render/vulkan/resources/sampler.cpp
        render/vulkan/pipeline/graphics_pipeline.hpp
        render/vulkan/pipeline/graphics_pipeline.cpp
        render/vulkan/pipeline/pipeline_layout_cache.hpp
        render/vulkan/pipeline/pipeline_layout_cache.cpp
        render/vulkan/pipeline/descriptor_layout_cache.hpp
        render/vulkan/pipeline/descriptor_layout_cache.cpp
        render/vulkan/render_graph/resource_manager.hpp
        render/vulkan/render_graph/resource_manager.cpp
        render/vertex.hpp
        SDL/context.cpp
        SDL/context.hpp
        SDL/vulkan/vulkan.hpp
        SDL/vulkan/window.cpp
        SDL/vulkan/window.hpp
        SDL/video.hpp
        SDL/keyboard.hpp
)

target_precompile_headers(${CURRENT_TARGET_NAME} PRIVATE pch.hpp)

target_link_libraries(${CURRENT_TARGET_NAME} PRIVATE
    public
    SDL3::SDL3
    SDL3_image::SDL3_image
    Vulkan::Headers
    glm::glm
    VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
    tinyobjloader::tinyobjloader
    Catch2::Catch2
    frozen::frozen
    SteamworksSDK::SteamworksSDK
    spirv-reflect::spirv-reflect
)

skylabs_configure_target(${CURRENT_TARGET_NAME})

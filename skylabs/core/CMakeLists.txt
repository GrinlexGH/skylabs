# core
set(CURRENT_TARGET_NAME core)

add_library(${CURRENT_TARGET_NAME} MODULE)
set_target_properties(${CURRENT_TARGET_NAME} PROPERTIES PREFIX "" IMPORT_PREFIX "")

target_sources(
    ${CURRENT_TARGET_NAME}
    PRIVATE
    FILE_SET CXX_MODULES FILES
        render/vulkan/context/instance.cppm
        render/vulkan/context/surface.cppm
        render/vulkan/context/physical_device.cppm
        render/vulkan/context/device.cppm
        render/vulkan/context/allocator.cppm
        render/vulkan/context/context.cppm
        render/vulkan/resources/resources.cppm
        render/vulkan/resources/image.cppm
        render/vulkan/resources/buffer.cppm
        render/vulkan/resources/sampler.cppm
    FILE_SET SOURCES FILES
        camera.hpp
        launcher.cpp
        launcher.hpp
        main.cpp
        render/renderer.hpp
        render/vertex.hpp
        render/vulkan/command_recording/command_buffer.cpp
        render/vulkan/command_recording/command_buffer.hpp
        render/vulkan/command_recording/command_buffer_allocator.cpp
        render/vulkan/command_recording/command_buffer_allocator.hpp
        render/vulkan/command_recording/sync_state.cpp
        render/vulkan/command_recording/sync_state.hpp
        render/vulkan/context/instance.cpp
        render/vulkan/context/physical_device.cpp
        render/vulkan/context/device.cpp
        render/vulkan/context/allocator.cpp
        render/vulkan/context/context.cpp
        render/vulkan/pipeline/compute_pipeline.cpp
        render/vulkan/pipeline/compute_pipeline.hpp
        render/vulkan/pipeline/descriptor_layout_cache.cpp
        render/vulkan/pipeline/descriptor_layout_cache.hpp
        render/vulkan/pipeline/graphics_pipeline.cpp
        render/vulkan/pipeline/graphics_pipeline.hpp
        render/vulkan/pipeline/pipeline_layout_cache.cpp
        render/vulkan/pipeline/pipeline_layout_cache.hpp
        render/vulkan/pipeline/shader.cpp
        render/vulkan/pipeline/shader.hpp
        render/vulkan/platform/swapchain.cpp
        render/vulkan/platform/swapchain.hpp
        render/vulkan/pipeline/descriptor_allocator.cpp
        render/vulkan/pipeline/descriptor_allocator.hpp
        render/vulkan/pipeline/descriptor_writer.cpp
        render/vulkan/pipeline/descriptor_writer.hpp
        render/vulkan/renderer.cpp
        render/vulkan/renderer.hpp
        render/vulkan/resources/buffer.cpp
        render/vulkan/resources/image.cpp
        render/vulkan/resources/sampler.cpp
        render/vulkan/submesh.hpp
        render/vulkan/render_pass.hpp
        render/vulkan/main_pass.cpp
        render/vulkan/main_pass.hpp
        render/vulkan/post_process_pass.cpp
        render/vulkan/post_process_pass.hpp
        render/render_object.hpp
        render/vulkan/render_object.hpp
        render/vulkan/render_object.cpp
)

target_precompile_headers(${CURRENT_TARGET_NAME} PRIVATE pch.hpp)

add_dependencies(${CURRENT_TARGET_NAME} shaders)

target_link_libraries(${CURRENT_TARGET_NAME} PRIVATE
    public
    Catch2::Catch2
    EnTT::EnTT
    SDL3_image::SDL3_image
    stb::stb
    SteamworksSDK::SteamworksSDK
    tinyobjloader::tinyobjloader
    vk-bootstrap::vk-bootstrap
    glm-HppModule
    Vulkan-HppModule
    VulkanMemoryAllocator-HppModule
)

skylabs_configure_target(${CURRENT_TARGET_NAME})

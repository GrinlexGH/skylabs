# core
set(CURRENT_TARGET_NAME core)

add_library(${CURRENT_TARGET_NAME} MODULE)
set_target_properties(${CURRENT_TARGET_NAME} PROPERTIES PREFIX "" IMPORT_PREFIX "")

target_sources(
    ${CURRENT_TARGET_NAME}
    PRIVATE
    FILE_SET HEADERS FILES
        pch.hpp
        camera.hpp
        render/vulkan/command_recording/command_buffer_allocator.hpp
        render/vulkan/command_recording/command_buffer.hpp
        render/vulkan/command_recording/sync_state.hpp
        render/vulkan/context/allocator.hpp
        render/vulkan/context/context.hpp
        render/vulkan/context/device.hpp
        render/vulkan/context/instance.hpp
        render/vulkan/context/physical_device.hpp
        render/vulkan/context/surface.hpp
        render/vulkan/pipeline/compute_pipeline.hpp
        render/vulkan/pipeline/descriptor_allocator.hpp
        render/vulkan/pipeline/descriptor_layout_cache.hpp
        render/vulkan/pipeline/descriptor_writer.hpp
        render/vulkan/pipeline/graphics_pipeline.hpp
        render/vulkan/pipeline/pipeline_layout_cache.hpp
        render/vulkan/pipeline/shader.hpp
        render/vulkan/swapchain.hpp
        render/vulkan/resources/buffer.hpp
        render/vulkan/resources/image.hpp
        render/vulkan/resources/sampler.hpp
        render/vulkan/in_flight.hpp
        render/vulkan/main_pass.hpp
        render/vulkan/post_process_pass.hpp
        render/vulkan/render_object.hpp
        render/vulkan/renderer.hpp
        render/vulkan/submesh.hpp
        render/render_object.hpp
        render/renderer.hpp
        render/vertex.hpp
        camera.hpp
        launcher.hpp
    FILE_SET SOURCES FILES
        render/vulkan/command_recording/command_buffer_allocator.cpp
        render/vulkan/command_recording/command_buffer.cpp
        render/vulkan/command_recording/sync_state.cpp
        render/vulkan/context/allocator.cpp
        render/vulkan/context/context.cpp
        render/vulkan/context/device.cpp
        render/vulkan/context/instance.cpp
        render/vulkan/context/physical_device.cpp
        render/vulkan/pipeline/compute_pipeline.cpp
        render/vulkan/pipeline/descriptor_allocator.cpp
        render/vulkan/pipeline/descriptor_layout_cache.cpp
        render/vulkan/pipeline/descriptor_writer.cpp
        render/vulkan/pipeline/graphics_pipeline.cpp
        render/vulkan/pipeline/pipeline_layout_cache.cpp
        render/vulkan/pipeline/shader.cpp
        render/vulkan/swapchain.cpp
        render/vulkan/resources/buffer.cpp
        render/vulkan/resources/image.cpp
        render/vulkan/resources/sampler.cpp
        render/vulkan/main_pass.cpp
        render/vulkan/post_process_pass.cpp
        render/vulkan/render_object.cpp
        render/vulkan/renderer.cpp
        header_factory.cpp
        launcher.cpp
        main.cpp
)

target_precompile_headers(${CURRENT_TARGET_NAME} PRIVATE pch.hpp)

add_dependencies(${CURRENT_TARGET_NAME} shaders)

target_link_libraries(${CURRENT_TARGET_NAME} PRIVATE
    public
    EnTT::EnTT
    glm::glm
    SDL3_image::SDL3_image
    tinyobjloader::tinyobjloader
    vk-bootstrap::vk-bootstrap
    VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
)

skylabs_configure_target(${CURRENT_TARGET_NAME} LIBRARY_DESTINATION $<IF:$<PLATFORM_ID:Windows>,bin,lib>)

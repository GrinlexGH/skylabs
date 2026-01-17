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
        render/vulkan/window.hpp
        render/vulkan/profile.hpp
        render/vulkan/profile.cpp
        render/vulkan/renderer.hpp
        render/vulkan/renderer.cpp
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
        render/vulkan/surface.hpp
        render/vulkan/surface.cpp
        render/vulkan/swapchain.hpp
        render/vulkan/swapchain.cpp
        render/vulkan/frame_data.hpp
        render/vulkan/frame_data.cpp
        render/vulkan/shader.hpp
        render/vulkan/shader.cpp
        render/vulkan/memory/image.hpp
        render/vulkan/memory/image.cpp
        render/vulkan/memory/device_buffer.hpp
        render/vulkan/memory/device_buffer.cpp
        render/vulkan/memory/host_buffer.hpp
        render/vulkan/memory/host_buffer.cpp
        render/vulkan/sampler.hpp
        render/vulkan/sampler.cpp
        render/vulkan/pipeline.hpp
        render/vulkan/pipeline.cpp
        render/vulkan/render_target.hpp
        render/vulkan/vertex_format.hpp
        render/vulkan/vertex_format.cpp
        render/vertex.hpp
        SDL/context.cpp
        SDL/context.hpp
        SDL/vulkan/vulkan.hpp
        SDL/vulkan/window.cpp
        SDL/vulkan/window.hpp
        SDL/video.hpp
        SDL/keyboard.hpp
)

# No warnings in header factory file because of LTO in msvc
if(IS_MSVC_LIKE)
    set_source_files_properties(header_factory.cpp PROPERTIES COMPILE_OPTIONS "/W1")
endif()

add_dependencies(${CURRENT_TARGET_NAME} public shaders)

target_link_libraries(${CURRENT_TARGET_NAME} PRIVATE
    public
    SDL3::SDL3
    SDL3_image::SDL3_image
    Vulkan::Headers
    glm::glm
    VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
    tinyobjloader::tinyobjloader
    benchmark::benchmark
    frozen::frozen
)

if(SKYLABS_ENABLE_STEAM)
    target_link_libraries(${CURRENT_TARGET_NAME} PRIVATE SteamworksSDK::SteamworksSDK)
endif()

skylabs_configure_target(${CURRENT_TARGET_NAME})

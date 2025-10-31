# core
set(CURRENT_TARGET_NAME core)

set(SOURCES
    main.cpp
    launcher.hpp
    launcher.cpp
    header_factory.cpp
    window.hpp
    camera.hpp
    render/renderer.hpp
    render/vulkan/window.hpp
    render/vulkan/renderer.hpp
    render/vulkan/renderer.cpp
    render/vulkan/context/allocator.hpp
    render/vulkan/context/allocator.cpp
    render/vulkan/context/instance.hpp
    render/vulkan/context/instance.cpp
    render/vulkan/context/extensions.hpp
    render/vulkan/context/physical_device.hpp
    render/vulkan/context/physical_device.cpp
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
    render/vulkan/memory/memory_mapping.hpp
    render/vulkan/memory/memory_mapping.cpp
    render/vulkan/sampler.hpp
    render/vulkan/sampler.cpp
    SDL/context.cpp
    SDL/context.hpp
    SDL/vulkan/vulkan.hpp
    SDL/vulkan/window.cpp
    SDL/vulkan/window.hpp
    SDL/video.hpp
    SDL/keyboard.hpp
)

add_library(${CURRENT_TARGET_NAME} MODULE ${SOURCES})

# No warnings in header factory file because of LTO in msvc
if(IS_MSVC_LIKE)
    set_source_files_properties(
        header_factory.cpp
        PROPERTIES COMPILE_OPTIONS
        "/W1"
    )
endif()

foreach(source IN LISTS SOURCES)
    cmake_path(GET source PARENT_PATH source_directory)
    source_group("Source Files/${source_directory}" FILES "${source}")
endforeach()

add_dependencies(${CURRENT_TARGET_NAME} public shaders)

deps_target_link_and_copy_runtime(${CURRENT_TARGET_NAME}
    PRIVATE
    public
    SDL3::SDL3
    SDL3_image::SDL3_image
    Vulkan::Headers
    glm::glm
    GPUOpen::VulkanMemoryAllocator
    VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
)

if(SKYLABS_ENABLE_STEAM)
    deps_target_link_and_copy_runtime(${CURRENT_TARGET_NAME}
        PRIVATE
        SteamworksSDK::SteamAPI
        SteamworksSDK::AppTicket
    )
endif()

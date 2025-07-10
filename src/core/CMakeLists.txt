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
    render/vulkan/vulkan_window.hpp
    render/vulkan/vulkan_renderer.hpp
    render/vulkan/vulkan_renderer.cpp
    render/vulkan/instance.hpp
    render/vulkan/instance.cpp
    render/vulkan/extensions.hpp
    render/vulkan/physical_device.hpp
    render/vulkan/physical_device.cpp
    render/vulkan/device.hpp
    render/vulkan/device.cpp
    render/vulkan/render_context.hpp
    render/vulkan/render_context.cpp
    render/vulkan/surface.hpp
    render/vulkan/surface.cpp
    render/vulkan/swapchain.hpp
    render/vulkan/swapchain.cpp
    render/vulkan/allocator.hpp
    render/vulkan/allocator.cpp
    render/vulkan/frame_data.hpp
    render/vulkan/frame_data.cpp
    SDL/context.cpp
    SDL/context.hpp
    SDL/vulkan.hpp
    SDL/window.cpp
    SDL/window.hpp
    SDL/video.hpp
)

add_library(${CURRENT_TARGET_NAME} MODULE ${SOURCES})

foreach(FILE IN LISTS SOURCES)
    get_filename_component(SOURCE_PATH "${FILE}" PATH)
    string(REPLACE "/" "\\" SOURCE_PATH_GROUP "${SOURCE_PATH}")
    source_group("Source Files\\${SOURCE_PATH_GROUP}" FILES "${FILE}")
endforeach()

set_target_properties(${CURRENT_TARGET_NAME}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBS_OUTPUT_DIRECTORY}/
    RUNTIME_OUTPUT_DIRECTORY ${DLLS_OUTPUT_DIRECTORY}/
    LIBRARY_OUTPUT_DIRECTORY ${DLLS_OUTPUT_DIRECTORY}/
)

add_dependencies(${CURRENT_TARGET_NAME} public shaders)
target_link_libraries(${CURRENT_TARGET_NAME}
    PRIVATE
    public
    SDL3::SDL3
    SDL3_image::SDL3_image
    Vulkan::Vulkan
    glm::glm
    GPUOpen::VulkanMemoryAllocator
    VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
    SteamworksSDK::SteamworksSDK
    SteamworksSDK::AppTicket
)

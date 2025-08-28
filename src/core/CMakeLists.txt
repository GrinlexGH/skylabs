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
    render/vulkan/allocator.hpp
    render/vulkan/allocator.cpp
    render/vulkan/frame_data.hpp
    render/vulkan/frame_data.cpp
    render/vulkan/render_pass/attachment.cpp
    render/vulkan/render_pass/attachment.hpp
    render/vulkan/render_pass/render_pass.cpp
    render/vulkan/render_pass/render_pass.hpp
    SDL/context.cpp
    SDL/context.hpp
    SDL/vulkan/vulkan.hpp
    SDL/vulkan/window.cpp
    SDL/vulkan/window.hpp
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
    Vulkan::Headers
    glm::glm
    GPUOpen::VulkanMemoryAllocator
    VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
)

if(ENABLE_STEAM)
    target_link_libraries(${CURRENT_TARGET_NAME}
        PRIVATE
        SteamworksSDK::SteamAPI
        SteamworksSDK::AppTicket
    )
endif()

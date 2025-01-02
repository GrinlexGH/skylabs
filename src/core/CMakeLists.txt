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
    render/vulkan/vulkan.hpp
    render/vulkan/vulkan_window.hpp
    render/vulkan/vulkan_renderer.hpp
    render/vulkan/vulkan_renderer.cpp
    render/vulkan/instance.hpp
    render/vulkan/instance.cpp
    render/vulkan/extensions/extension_manager.hpp
    render/vulkan/extensions/debug_messenger.hpp
    render/vulkan/extensions/debug_messenger.cpp
    SDL/SDL_context.cpp
    SDL/SDL_context.hpp
    SDL/SDL_vulkan.hpp
    SDL/SDL_window.cpp
    SDL/SDL_window.hpp
    SDL/SDL_video.hpp
    SDL/SDL.hpp
)

add_library(${CURRENT_TARGET_NAME} SHARED ${SOURCES})

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
    Vulkan::Vulkan
    glm::glm
    GPUOpen::VulkanMemoryAllocator
    VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
    tinyobjloader::tinyobjloader
    #GLFW3::GLFW3
)

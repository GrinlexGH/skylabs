# core
set(CURRENT_TARGET_NAME core)

set(SOURCES
    coremain.cpp
    launcher.hpp
    launcher.cpp
    render/renderer.hpp
    render/vulkan_renderer.hpp
    render/vulkan_renderer.cpp
    header_factory.cpp
    stb_image.h
    vulkan.hpp
    vulkan_window.hpp
    SDL/SDL_context.cpp
    SDL/SDL_context.hpp
    SDL/SDL_vulkan.hpp
    SDL/SDL_window.cpp
    SDL/SDL_window.hpp
    SDL/SDL_video.hpp
    SDL/SDL.hpp
    camera.hpp
)

if(WIN32)
    set(SOURCES
        ${SOURCES}
        winapp.cpp
    )
elseif(UNIX)
    set(SOURCES
        ${SOURCES}
        posixapp.cpp
    )
endif()

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

# core
set(CURRENT_TARGET_NAME core)

set(SOURCES
    coremain.cpp
    launcher.hpp
    launcher.cpp
    renderer.hpp
    vulkan_renderer.hpp
    vulkan_renderer.cpp
    vulkan_renderer_context.hpp
    vulkan_renderer_context.cpp
    window.hpp
    SDL.hpp
    SDL_Vulkan.hpp
    SDL.cpp
    headerfactory.cpp
    stb_image.h
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

foreach(FILE ${SOURCES})
    get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")
    string(REPLACE "/" "\\" GROUP "${GROUP}")
    set(GROUP "Source Files${GROUP}")
    source_group("${GROUP}" FILES "${FILE}")
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

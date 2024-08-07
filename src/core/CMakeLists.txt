# core
set(CURRENT_TARGET_NAME core)

set(SOURCES
    coremain.cpp
    launcher.hpp
    launcher.cpp
    renderapi.hpp
    vulkanapi.hpp
    vulkanapi.cpp
    window.hpp
    SDL.hpp
    SDL_Vulkan.hpp
    SDL.cpp
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

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(${CURRENT_TARGET_NAME} SHARED ${SOURCES})

foreach(source IN LISTS SOURCES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()

set_target_properties(${CURRENT_TARGET_NAME}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBS_OUTPUT_DIRECTORY}/
    RUNTIME_OUTPUT_DIRECTORY ${DLLS_OUTPUT_DIRECTORY}/
    LIBRARY_OUTPUT_DIRECTORY ${DLLS_OUTPUT_DIRECTORY}/
)

add_dependencies(${CURRENT_TARGET_NAME} public)
target_link_libraries(${CURRENT_TARGET_NAME}
    PRIVATE
    public
    SDL2::SDL2
    Vulkan::Vulkan

    GLFW3::GLFW3
)

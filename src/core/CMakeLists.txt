# core
set(CURRENT_TARGET_NAME core)

set(SOURCES
    coremain.cpp
    launcher.hpp
    launcher.cpp
    renderapi.hpp
    vulkanapi.hpp
    vulkanapi.cpp
    vulkan_initializer.hpp
    vulkan_initializer.cpp
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

add_library(${CURRENT_TARGET_NAME} SHARED ${SOURCES})

foreach(FILE ${SOURCES})
    get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")
    string(REPLACE "/" "\\" GROUP "${GROUP}")
    if ("${FILE}" MATCHES ".*\\.cpp")
        set(GROUP "Source Files${GROUP}")
    elseif("${FILE}" MATCHES ".*\\.h")
        set(GROUP "Header Files${GROUP}")
    endif()
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
    SDL2::SDL2
    Vulkan::Vulkan
    #GLFW3::GLFW3
)

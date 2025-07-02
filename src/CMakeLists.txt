# src
set(PROJECTS
    launcher
    core
    public
)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(CMAKE_SKIP_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")

set(ROOT_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>/)
set(DLLS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>/bin)
set(LIBS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>/libs)
set(SHADER_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>/shaders)

# For project info header generation
set(APP_NAME ${PROJECT_NAME})
set(APP_GAME_NAME ${PROJECT_NAME})
set(APP_VERSION ${PROJECT_VERSION})
set(APP_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(APP_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(APP_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(APP_VERSION_TWEAK ${PROJECT_VERSION_TWEAK})
set(APP_COMPANY "Grinlex Stydios")
set(APP_COPYRIGHT "MIT License. Copyright (c) 2025-present ${APP_COMPANY}")
set(APP_HOMEPAGE ${CMAKE_PROJECT_HOMEPAGE_URL})

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(
        "-Wall" "-Wextra" "-Wpedantic"
        #"$<$<CONFIG:Release>:-Werror>"
        "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-O3>"
        "-march=native" "-mtune=native"
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang" AND NOT CMAKE_CXX_SIMULATE_ID MATCHES "MSVC")
        set(CMAKE_LINKER_TYPE LLD)
        add_compile_options(
            "-stdlib=libc++"
        )
        add_link_options(
            "-stdlib=libc++"
        )
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(
        "/W4" "/utf-8" "/Zc:__cplusplus" "/MP"
        #"$<$<CONFIG:Release>:/WX>"
        "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:/Ox>"
    )
endif()

# All macros in one place (except targetname_EXPORT macros)
add_compile_definitions(
    NOMINMAX
    $<$<CONFIG:Debug>:DEBUG> $<$<CONFIG:RelWithDebInfo>:DEBUG>
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)

if(WIN32)
    add_compile_definitions(PLATFORM_WINDOWS UNICODE _UNICODE VK_USE_PLATFORM_WIN32_KHR)
endif()

if(UNIX)
    add_compile_definitions(PLATFORM_UNIX)
endif()

if(LINUX)
    add_compile_definitions(PLATFORM_LINUX)
endif()

if(APPLE)
    add_compile_definitions(PLATFORM_APPLE)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_compile_definitions(ARCH_64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_compile_definitions(ARCH_32)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" OR CMAKE_CXX_SIMULATE_ID MATCHES "MSVC")
    add_compile_definitions(COMPILER_MSVC)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_definitions(COMPILER_GCC)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    add_compile_definitions(COMPILER_CLANG)
endif()

add_subdirectory("shaders")
foreach(PROJECT ${PROJECTS})
    add_subdirectory("${PROJECT}")
endforeach()

foreach(PROJECT ${PROJECTS})
    if(WIN32)
        set_target_properties(${PROJECT} PROPERTIES PREFIX "")
    endif()
endforeach()

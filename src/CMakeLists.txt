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

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(
        "-Wall" "-Wextra" "-Wpedantic"
        "$<$<CONFIG:Release>:-Werror>"
        "-march=native" "-mtune=native"
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(
        "/W4" "/utf-8" "/Zc:__cplusplus" "/MP"
        "$<$<CONFIG:Release>:/WX>"
    )
endif()

add_compile_definitions(NOMINMAX $<$<CONFIG:Debug>:DEBUG> $<$<CONFIG:RelWithDebInfo>:DEBUG>)

if(WIN32)
    add_compile_definitions(PLATFORM_WINDOWS UNICODE _UNICODE)
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
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_definitions(COMPILER_GCC)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
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

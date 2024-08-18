# src
set(projects
    launcher
    core
    public
)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(LIBS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>/libs)
set(DLLS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>/bin)
set(ROOT_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>)

foreach(SUBDIR ${projects})
    add_subdirectory("${SUBDIR}")
endforeach()

set(CMAKE_COLOR_DIAGNOSTICS ON)

foreach(project ${projects})
    target_compile_definitions(${project} PRIVATE NOMINMAX)

    if(CYGWIN)
        target_compile_definitions(${project} PRIVATE PLATFORM_CYGWIN)
    endif()

    if(UNIX)
        target_compile_definitions(${project} PRIVATE PLATFORM_POSIX)
    endif()

    if(WIN32)
        target_compile_definitions(${project} PRIVATE PLATFORM_WINDOWS UNICODE _UNICODE)
    endif()

    if(MSVC)
        target_compile_options(${project} PRIVATE "/WX" "/W4" "/utf-8" "/Zc:__cplusplus")

        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            target_compile_definitions(${project} PRIVATE COMPILER_MSVC64)
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
            target_compile_definitions(${project} PRIVATE COMPILER_MSVC32)
        endif()
    else()
        target_compile_definitions(${project} PRIVATE COMPILER_GCC)
        target_compile_options(${project} PRIVATE "-Wall" "-Wextra" "-Wpedantic" "-Werror")

        if(WIN32)
            target_link_options(${project} PRIVATE "-municode")
            set_target_properties(${project} PROPERTIES PREFIX "")
        endif()
    endif()
endforeach()

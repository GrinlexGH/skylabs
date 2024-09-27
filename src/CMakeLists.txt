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

add_subdirectory("shaders")

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

    if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            target_compile_definitions(${project} PRIVATE COMPILER_MSVC64)
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
            target_compile_definitions(${project} PRIVATE COMPILER_MSVC32)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_definitions(${project} PRIVATE COMPILER_GCC)
        if(WIN32)
            set_target_properties(${project} PROPERTIES PREFIX "")
        endif()
    endif()
endforeach()

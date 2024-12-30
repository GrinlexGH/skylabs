# src
set(PROJECTS
    launcher
    core
    public
)

add_compile_definitions(NOMINMAX)

if(WIN32)
    add_compile_definitions(PLATFORM_WINDOWS UNICODE _UNICODE)
endif()

if(LINUX)
    add_compile_definitions(PLATFORM_LINUX)
endif()

if(CYGWIN)
    add_compile_definitions(PLATFORM_CYGWIN)
endif()

if(ANDROID)
    add_compile_definitions(PLATFORM_ANDROID)
endif()

if(IOS)
    add_compile_definitions(PLATFORM_IOS)
endif()

if(APPLE)
    add_compile_definitions(PLATFORM_APPLE)
endif()


# For the future: there is a CMAKE_SYSTEM_NAME if
# there is no special definition for the platform

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_compile_definitions(ARCH_64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_compile_definitions(ARCH_32)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_definitions(COMPILER_MSVC)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_definitions(COMPILER_GCC)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
        add_compile_definitions(COMPILER_CLANG)
    endif()
endif()

add_subdirectory("shaders")
foreach(PROJECT ${PROJECTS})
    add_subdirectory("${PROJECT}")
endforeach()

if(WIN32)
    foreach(PROJECT ${PROJECTS})
        set_target_properties(${PROJECT} PROPERTIES PREFIX "")
    endforeach()
endif()

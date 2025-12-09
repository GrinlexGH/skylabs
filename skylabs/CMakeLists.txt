# skylabs
set(PROJECTS
    shaders
    launcher
    core
    public
)

# Output directories
set(CMAKE_PDB_OUTPUT_DIRECTORY ${SKYLABS_BIN_OUTPUT_DIRECTORY})
set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY ${SKYLABS_BIN_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${SKYLABS_BIN_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${SKYLABS_BIN_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${SKYLABS_LIB_OUTPUT_DIRECTORY})

# Portability parameters
set(CMAKE_SKIP_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warnings
if(IS_GNU_LIKE)
    add_compile_options(
        "-Wall" "-Wextra" "-Wpedantic"
        #"$<$<CONFIG:Release>:-Werror>"
    )
elseif(IS_MSVC_LIKE)
    add_compile_options(
        "/W4"
        #"$<$<CONFIG:Release>:/WX>"
    )
endif()

# All global macros
add_compile_definitions(
    NOMINMAX
    $<$<CONFIG:Debug>:DEBUG> $<$<CONFIG:RelWithDebInfo>:DEBUG>
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_TYPESAFE_CONVERSION=1
    VMA_STATIC_VULKAN_FUNCTIONS=0
    VMA_DYNAMIC_VULKAN_FUNCTIONS=0
)

if(WIN32)
    add_compile_definitions(PLATFORM_WINDOWS VK_USE_PLATFORM_WIN32_KHR)
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

if(ANDROID)
    add_compile_definitions(PLATFORM_ANDROID)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_compile_definitions(ARCH_64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_compile_definitions(ARCH_32)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_definitions(COMPILER_MSVC)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_definitions(COMPILER_GCC)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    add_compile_definitions(COMPILER_CLANG)
endif()

foreach(PROJECT ${PROJECTS})
    add_subdirectory("${PROJECT}")
endforeach()

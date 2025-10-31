# Determine a compiler type
set(IS_GNU_LIKE FALSE)
set(IS_MSVC_LIKE FALSE)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        set(IS_MSVC_LIKE TRUE)
    else()
        set(IS_GNU_LIKE TRUE)
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(IS_MSVC_LIKE TRUE)
endif()

# Choosing a runtime
if(IS_MSVC_LIKE)
    set(SKYLABS_STD_RUNTIME "vcruntime")
elseif(IS_GNU_LIKE)
    set(SKYLABS_STD_RUNTIME "libstdcxx")

    # Use libc++ on linux or msys2 just because I can
    if(SKYLABS_TRY_TO_USE_LIBCXX AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND (UNIX OR (WIN32 AND MINGW)))
        set(SKYLABS_STD_RUNTIME "libcxx")
    endif()

    # Use vcruntime for non mingw clang on windows
    if((WIN32 AND NOT MINGW) AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
        set(SKYLABS_STD_RUNTIME "vcruntime")
    endif()
endif()

# Subdir to separate runtimes
set(DEPS_OUT_SUBDIR "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}/${SKYLABS_STD_RUNTIME}/$ENV{DEPS_OUT_SUBDIR}")

include(Deps)

# Build with libcxx
if(SKYLABS_STD_RUNTIME STREQUAL "libcxx")
    if(NOT ANDROID) # Android SDK doesn't support explicitly setting lld
        set(CMAKE_LINKER_TYPE LLD)
    endif()
    add_compile_options("-stdlib=libc++")
    add_link_options("-stdlib=libc++")

    # Compile dependencies with libc++
    deps_append_cmake_define(CMAKE_CXX_FLAGS "-stdlib=libc++")
    deps_append_cmake_define(CMAKE_SHARED_LINKER_FLAGS "-stdlib=libc++")
    deps_append_cmake_define(CMAKE_MODULE_LINKER_FLAGS "-stdlib=libc++")
    deps_append_cmake_define(CMAKE_EXE_LINKER_FLAGS "-stdlib=libc++")
endif()

# Set the same compiler and generator for dependencies
deps_append_cmake_define(CMAKE_GENERATOR)
deps_append_cmake_define(CMAKE_GENERATOR_PLATFORM)
deps_append_cmake_define(CMAKE_C_COMPILER)
deps_append_cmake_define(CMAKE_CXX_COMPILER)

deps_append_cmake_define(CMAKE_TOOLCHAIN_FILE)

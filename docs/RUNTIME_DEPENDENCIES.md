# Debugging And Runtime Dependencies

Once the project compiles successfully, you need to run and debug it. The operating system needs to locate all shared libraries, and the executable needs to find its assets.

## Table of Contents
1. [The Local Debugging Solution](#the-local-debugging-solution)
2. [Configuring CMake Install](#configuring-cmake-install)

## The Local Debugging Solution

Initially, I tried various hacky workarounds to handle DLLs: manually copying them via `add_custom_command`, writing custom scripts with `file(GET_RUNTIME_DEPENDENCIES)`, using [`cmake --install`](https://stackoverflow.com/a/75065206/16793487), and so on.

Eventually, I realized that the best approach was to configure the build directory properly.

I wrapped target configuration in a separate convenient function `skylabs_configure_target` to minimize the boilerplate.

First, I forced CMake to output all binaries to a dedicated folder (e.g., `CMAKE_BINARY_DIR/built`):

```cmake
set(SKYLABS_BUILD_DIR ${CMAKE_BINARY_DIR}/built/$<$<BOOL:${IS_MULTI_CONFIG}>:$<CONFIG>/>)

# ...

function(skylabs_configure_target ...)
    # ...

    set(runtime_dest "bin")
    set(archive_dest "lib")
    set(library_dest "lib")
    
    if(ARG_RUNTIME_DESTINATION)
       set(runtime_dest ${ARG_RUNTIME_DESTINATION})
    endif()
    
    if(ARG_ARCHIVE_DESTINATION)
       set(archive_dest ${ARG_ARCHIVE_DESTINATION})
    endif()
    
    if(ARG_LIBRARY_DESTINATION)
       set(library_dest ${ARG_LIBRARY_DESTINATION})
    endif()
    
    set_target_properties(${target_name} PROPERTIES
       RUNTIME_OUTPUT_DIRECTORY "${SKYLABS_BUILD_DIR}${runtime_dest}"
       PDB_OUTPUT_DIRECTORY "${SKYLABS_BUILD_DIR}${runtime_dest}"
       ARCHIVE_OUTPUT_DIRECTORY "${SKYLABS_BUILD_DIR}${archive_dest}"
       COMPILE_PDB_OUTPUT_DIRECTORY "${SKYLABS_BUILD_DIR}${archive_dest}"
       LIBRARY_OUTPUT_DIRECTORY "${SKYLABS_BUILD_DIR}${library_dest}"
    )

    # ...
endfunction()
```

On Windows, shared libraries must sit next to the executable. CMake 3.21 introduced the `$<TARGET_RUNTIME_DLLS>` generator expression, so I can copy DLLs after build:

```cmake
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            -t $<TARGET_FILE_DIR:${target_name}> $<TARGET_RUNTIME_DLLS:${target_name}>
        COMMAND_EXPAND_LISTS
    )
```

On Linux, copying is unnecessary, because CMake can specify paths to `.so`'s via [`RPATH`](https://cmake.org/cmake/help/latest/prop_tgt/BUILD_RPATH.html#prop_tgt:BUILD_RPATH).

To ensure the debugger finds game assets without copying gigabytes of data, I simply hardcoded the IDE's working directory to the repository root:

```cmake
set(CMAKE_DEBUGGER_WORKING_DIRECTORY ${SKYLABS_ROOT_DIR})
```

There was one remaining issue: dynamically loaded DLL plugins (like with `dlopen` or `LoadLibrary`).
I manually copy them via `add_custom_command`:

```cmake
    if(ARG_RUNTIME_PLUGINS)
       set(runtime_artifacts "")
       foreach(plugin_target IN LISTS ARG_RUNTIME_PLUGINS)
          list(APPEND runtime_artifacts "$<TARGET_FILE:${plugin_target}>")
       endforeach()
    
       add_custom_command(TARGET ${target_name} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
                -t $<TARGET_FILE_DIR:${target_name}> ${runtime_artifacts}
          COMMAND_EXPAND_LISTS
       )
    endif()
```

## Configuring CMake Install

You can configure a complete installation of the entire project into a single folder, because why not? It's convenient to have a portable project folder for quickly sharing with a friend or a VM.

CMake 3.21 provides a very convenient feature that allows you to copy all dependencies to the output folder, both on Windows and Linux:

```cmake
    set(runtime_dependencies_args "")
    if(NOT CMAKE_CROSSCOMPILING)
       set(runtime_lookup_directories "")
       if(WIN32)
          list(APPEND runtime_lookup_directories "${CONAN_RUNTIME_LIB_DIRS}")
          cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH CXX_COMPILER_BIN_DIR)
          list(APPEND runtime_lookup_directories "${CXX_COMPILER_BIN_DIR}")
          cmake_path(GET CMAKE_C_COMPILER PARENT_PATH C_COMPILER_BIN_DIR)
          list(APPEND runtime_lookup_directories "${C_COMPILER_BIN_DIR}")
          list(APPEND runtime_lookup_directories "${SKYLABS_BUILD_DIR}")
          list(APPEND runtime_lookup_directories "${SKYLABS_BUILD_DIR}bin")
          list(APPEND runtime_lookup_directories "${SKYLABS_BUILD_DIR}lib")
       endif()
    
       set(runtime_dependencies_args
          RUNTIME_DEPENDENCIES
          DIRECTORIES ${runtime_lookup_directories}
          PRE_EXCLUDE_REGEXES
                "api-ms-win-.*" "ext-ms-.*"
                "libc\.so\..*" "libgcc_s\.so\..*" "libm\.so\..*" "libstdc\\+\\+\.so\..*"
          POST_EXCLUDE_REGEXES
                "^\/lib.*" "^\/usr\/lib.*"
                "C:[\\\/][Ww][Ii][Nn][Dd][Oo][Ww][Ss][\\\/].*"
          POST_INCLUDE_REGEXES
                "[Vv][Cc][Rr][Uu][Nn][Tt][Ii][Mm][Ee].*" "[Mm][Ss][Vv][Cc][Pp].*"
       )
    endif()
    
    install(TARGETS ${target_name}
       ${runtime_dependencies_args}
       ARCHIVE DESTINATION ${SKYLABS_INSTALL_SUBDIR}${archive_dest}
       LIBRARY DESTINATION ${SKYLABS_INSTALL_SUBDIR}${library_dest}
       RUNTIME DESTINATION ${SKYLABS_INSTALL_SUBDIR}${runtime_dest}
    )
```

To find necessary DLLs, CMake uses `DIRECTORY` folders on Windows and `RPATH` on Linux.

On Linux you also need to rewrite `RPATH` to use relative search paths in installing `elf`s:

```cmake
set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib")
```

For DLL plugins, I manually do `install(IMPORTED_RUNTIME_ARTIFACTS)`:

```cmake
    if(ARG_RUNTIME_PLUGINS)
       set(runtime_artifacts "")
       foreach(plugin_target IN LISTS ARG_RUNTIME_PLUGINS)
          list(APPEND runtime_artifacts "$<TARGET_FILE:${plugin_target}>")
       endforeach()
    
       # ...
    
       install(IMPORTED_RUNTIME_ARTIFACTS ${ARG_RUNTIME_PLUGINS}
          RUNTIME_DEPENDENCY_SET
          LIBRARY DESTINATION ${SKYLABS_INSTALL_SUBDIR}${library_dest}
          RUNTIME DESTINATION ${SKYLABS_INSTALL_SUBDIR}${runtime_dest}
       )
    endif()
```

And don't forget about `.pdb`'s:
```cmake
    install(
       FILES $<$<BOOL:${MSVC}>:$<TARGET_PDB_FILE:${target_name}>>
       DESTINATION ${SKYLABS_INSTALL_SUBDIR}${runtime_dest}
       OPTIONAL
    )
```

And don't forget about the assets:

```cmake
install(DIRECTORY ${SKYLABS_ROOT_DIR}/assets DESTINATION ${SKYLABS_INSTALL_SUBDIR}.)
```

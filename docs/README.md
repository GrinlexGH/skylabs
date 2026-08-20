## Table of Contents
1. [Introduction](#introduction)
2. [Build Configurations](#build-configurations)
3. [Third-Party Libraries](#third-party-libraries)
   - [Git Submodules](#git-submodules)
   - [Pre-Building](#pre-building)
4. [Conan](#conan)
   - [Building From Scratch](#building-from-scratch)
   - [Missing `.pdb` Files](#missing-pdb-files)
   - [ConanCenter](#conancenter)
   - [Local Recipe Index](#local-recipe-index)
   - [Artifactory Server](#artifactory-server)
5. [Debugging And Runtime Dependencies](#debugging-and-runtime-dependencies)
   - [The Local Debugging Solution](#the-local-debugging-solution)
   - [Configuring CMake Install](#configuring-cmake-install)
6. [Android](#android)
   - [SDL Android Project](#sdl-android-project)
   - [Android Studio Project Detection](#android-studio-project-detection)
   - [Custom Conan Gradle Task](#custom-conan-gradle-task)
   - [Copying Runtime Plugins And Custom Files](#copying-runtime-plugins-and-custom-files)
   - [CMAKE_FIND_ROOT_PATH_MODE_XXXXXXX](#cmake_find_root_path_mode_xxxxxxx)
7. [Skylabs](#skylabs)
   - [Clang-Format](#clang-format)
   - [Pre-Commit hooks](#pre-commit-hooks)
   - [Project Structure](#project-structure)

## Introduction

In this file, I'll be describing the issues and topics I encountered while writing this project, as well as the architecture and current state of Skylabs.

## Build Configurations

To ensure the codebase stays portable, I needed a reliable way to compile Skylabs with multiple compilers and configurations.

Initially, I was using [VS Code CMake kits](https://gist.github.com/GrinlexGH/cffbe9727b7183d7044e2c4af378ffd2). CMake kits are great for quick prototyping: they automatically detect local compilers, map executable paths, and seamlessly integrate into VS Code. But they only work in VS Code, so they cannot be easily shared across different IDEs or CI/CD pipelines.

CMake 3.19 added **CMake Presets**:
* `CMakePresets.json` defines generic project presets (generators, build directories, flags).
* `CMakeUserPresets.json` (gitignored) allows overriding local toolchain and compiler paths.

But unfortunately JSON configuration files quickly become bloated, although I personally am not picky about syntax. This eventually led me to look for a cleaner solution, which I found in **Conan profiles**.

## Third-Party Libraries

This is perhaps one of the most painful topics for C/C++. I used several methods in developing Skylabs.

### Git Submodules

The simplest way to include dependencies is adding them as Git submodules and bringing them into the build system using `add_subdirectory()`.

But this approach has several significant drawbacks:
* **IDE Clutter:** Every third-party library injects its own targets and source trees into the Visual Studio solution.
* **Increased Build Time:** Dependencies are compiled alongside your main project every time.
* **Cache Invalidation:** Whenever the CMake cache is wiped or regenerated, all external dependencies have to be rebuilt from scratch.

### Pre-Building

To avoid constant recompilation, a cleaner strategy is compiling dependencies separately, installing them into system via `cmake --install`, and referencing them in the main project using `find_package()`.

To automate this pipeline, I built a custom tool called [Deps](https://github.com/GrinlexGH/deps). But soon I realized that I was reinventing **Conan**.

## Conan

Conan neatly solves both problems discussed in this article:
1. **Dependency Management:** It fetches or builds binaries once, caches them globally, and generates CMake target integration automatically.
2. **Clean Build Profiles:** Conan profiles replace bloated `CMakePresets.json` setup with clean, declarative TOML configuration files specifying compiler versions, standard libraries, architectures, and build types.

It turned out to be the perfect solution.

### Building From Scratch

As I studied Conan, I realized that **it's best to compile libraries from scratch**. The first four paragraphs of [this](https://docs.conan.io/2/examples/dev_flow/debug/step_into_dependencies.html) article explain why. These include an `_ITERATOR_DEBUG_LEVEL` mismatch error and ABI incompatibility between `MinGW` and `MSVC`.

### Missing `.pdb` Files

Another important debugging detail is handling MSVC `.pdb` files. By default, some libraries do not install their `.pdb` files into the package, causing MSVC to generate annoying warnings. To fix this, I include a built-in hook in my Conan config: [`hook_copy_pdbs_to_package.py`](https://docs.conan.io/2/examples/dev_flow/debug/debugging_visual.html#installing-a-hook-to-copy-the-pdbs-to-the-package-folder). It automatically finds and copies the missing `.pdb` files into the final package folder.

### ConanCenter

I encountered several issues with the default **ConanCenter** recipes:
* **Slow updates:** New library versions often take a while to be merged.
* **Build system bugs:** For example, Boost's `b2` build system mistakenly recognizes MinGW's Clang as `clang-cl`, setting the wrong compilation options.
* **Outdated dependencies:** The SDL recipe required an old version of CMake that lacked a necessary generator for my setup.
* **Missing recipes:** Critical libraries for my stack, such as the Slang shader compiler, Steamworks SDK, and Vulkan validation layers for Android, were completely missing.

[Turned out](https://github.com/conan-io/conan/issues/20155#issuecomment-4913977776) that recipes in ConanCenter are contributed with common defaults and can't cover all possible variabilities.

So I found the solution in a **local recipe index**.

### Local Recipe Index

I decided to host my own [local recipe index](https://docs.conan.io/2/devops/devops_local_recipes_index.html). It contains [recipes](../conan/conan-recipes/recipes/) that have been rewritten and updated immediately when a new version of the library is released.

I even wrote a custom Telegram bot which automatically monitors releases for these libraries. (The bot is currently sitting in a private repository as the code is still a bit too raw).

### Artifactory Server

To avoid recompiling on my laptop and PC every time, I set up my own Artifactory server. It's configured like a regular Docker server.

It works very poorly on NTFS, and the built-in Derby database sometimes prevents it from starting correctly when the server is suddenly shut down.

To fix all this, I use PostgreSQL as a separate Docker service, install the server on a BTRFS partition, and use [`winbtrfs`](https://github.com/maharmstone/btrfs).

## Debugging And Runtime Dependencies

Once the project compiles successfully, you need to run and debug it. The operating system needs to locate all shared libraries, and the executable needs to find its assets.

### The Local Debugging Solution

Initially, I tried various hacky workarounds to handle DLLs: manually copying them via `add_custom_command`, writing custom scripts with `file(GET_RUNTIME_DEPENDENCIES)`, using [`cmake --install`](https://stackoverflow.com/a/75065206/16793487), and so on.

Eventually, I realized that the best approach was to configure the build directory properly.

I wrapped target configuration in a separate convenient function `skylabs_configure_target` to minimize the boilerplate.

First, I forced CMake to output all binaries to a dedicated folder (e.g., `CMAKE_BINARY_DIR/built`):

```cmake
set(SKYLABS_BUILD_DIR ${CMAKE_BINARY_DIR}/built/$<$<BOOL:${IS_MULTI_CONFIG}>:$<CONFIG>/>)

...

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
```

On Windows, shared libraries must sit next to the executable. CMake 3.21 introduced the `$<TARGET_RUNTIME_DLLS>` generator expression, so I can copy DLLs after build:

```cmake
add_custom_command(TARGET ${target_name} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        -t $<TARGET_FILE_DIR:${target_name}> $<TARGET_RUNTIME_DLLS:${target_name}>
    COMMAND_EXPAND_LISTS
)
```

On Linux, copying is unnecessary, because CMake can specify paths to `.so`'s via [RPATH](https://cmake.org/cmake/help/latest/prop_tgt/BUILD_RPATH.html#prop_tgt:BUILD_RPATH).

To ensure the debugger finds game assets without copying gigabytes of data, I simply hardcoded the IDE's working directory to the repository root:

```cmake
set(CMAKE_DEBUGGER_WORKING_DIRECTORY ${SKYLABS_ROOT_DIR})
```

There was one remaining issue: dynamically loaded plugins. I manually copy them via `add_custom_command`:

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

### Configuring CMake Install

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

On Linux you also need to rewrite `RPATH` to use relative search paths:

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

   ...

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

And don't forget about assets:

```cmake
install(DIRECTORY ${SKYLABS_ROOT_DIR}/assets DESTINATION ${SKYLABS_INSTALL_SUBDIR}.)
```

## Android

I wasn't originally planning on supporting Android, but I had nothing better to do at my grandma's, so I decided to do it.

I haven't encountered any significant issues with the Android build, but there are a few things worth noting.

### SDL Android Project

Since I use Conan with my custom recipes, I initially created a symlink to the Java source code in my Android project. This turned out to be ineffective, as there are files like `AndroidManifest.xml` that the user must override, but they are also updated by the library.

So I decided to simply move the Android project to a submodule and manually update SDL.

### Android Studio Project Detection

There's a bug in Android Studio that prevents a project from loading correctly. You just need to make sure the **package** field is present in `AndroidManifest.xml`:

```xml
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="org.grinlexstudios.skylabs"
    android:versionCode="1"
    android:versionName="1.0"
    android:installLocation="auto">
```

I spent several days fixing this...

### Custom Conan Gradle Task

The gradle task written [here](https://docs.conan.io/2/examples/cross_build/android/android_studio.html#build-gradle) is complete crap. I wrote my own and slightly tweaked the build dependencies so that Conan would build the libraries before syncing with Android Studio.

### Copying Runtime Plugins And Custom Files

I just copy plugins to `jniLib` directory:

```cmake
if(ARG_RUNTIME_PLUGINS)
   set(runtime_artifacts "")
   foreach(plugin_target IN LISTS ARG_RUNTIME_PLUGINS)
      list(APPEND runtime_artifacts "$<TARGET_FILE:${plugin_target}>")
   endforeach()

   ...

   if(ANDROID)
      add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
               "${SKYLABS_ANDROID_JNILIBS_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
               -t "${SKYLABS_ANDROID_JNILIBS_DIR}" ${runtime_artifacts}
      )
   endif()
endif()
```

For custom files like shader outputs I use these functions:

```cmake
# skylabs_install_directory(<target>
#     DIRECTORIES <directories>... DESTINATION <path>
# )
# Parameters:
#   DIRECTORIES                 List of directories to install with target
#   DESTINATION                 Subdirectory of install destination
function(skylabs_install_directories target_name)
    set(oneValueArgs DESTINATION)
    set(multiValueArgs DIRECTORIES)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    install(DIRECTORY ${ARG_DIRECTORIES}
        DESTINATION ${SKYLABS_INSTALL_SUBDIR}${ARG_DESTINATION}
    )

    if(ANDROID)
        foreach(dir ${ARG_DIRECTORIES})
            string(REGEX MATCH "/$" has_trailing_slash "${dir}")
            if(has_trailing_slash)
                set(dst "${SKYLABS_ANDROID_ASSETS_DIR}/${ARG_DESTINATION}")
            else()
                cmake_path(GET dir FILENAME dir_name)
                set(dst "${SKYLABS_ANDROID_ASSETS_DIR}/${ARG_DESTINATION}/${dir_name}")
            endif()
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${dst}"
                COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
                -t "${dst}" "${dir}"
            )
        endforeach()
    endif()
endfunction()

# skylabs_install_files(<target>
#     FILES <files>... DESTINATION <path>
# )
# Parameters:
#   FILES                       List of files to install with target
#   DESTINATION                 Subdirectory of install destination
function(skylabs_install_files target_name)
    set(oneValueArgs DESTINATION)
    set(multiValueArgs FILES)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    install(FILES ${ARG_FILES}
        DESTINATION ${SKYLABS_INSTALL_SUBDIR}${ARG_DESTINATION}
    )

    if(ANDROID)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "${SKYLABS_ANDROID_ASSETS_DIR}/${ARG_DESTINATION}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${ARG_FILES} "${SKYLABS_ANDROID_ASSETS_DIR}/${ARG_DESTINATION}"
        )
    endif()
endfunction()

...

# Shader target
skylabs_install_directories(${CURRENT_TARGET_NAME}
    DIRECTORIES ${SKYLABS_BUILD_DIR}/shaders
    DESTINATION .
)
```

### CMAKE_FIND_ROOT_PATH_MODE_XXXXXXX

By default, the Android toolchain searches for all libraries only in the NDK. To override this, you need to do something like this:

```cmake
if(ANDROID)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
endif()
```

## Skylabs

Finally, we can move on to describing the project's content.

### Clang-Format

There's no need to use `clang-format` in Skylabs. The configuration file is only needed for default settings for the IDE, so that it doesn't put an indent after the namespace block by default, for example.

### Pre-Commit hooks

Pre-commit hooks are very useful for removing unnecessary ugly whitespaces at the end of lines and other minor problems.

To install them use this command:

```bash
pre-commit install
```

First, of course, you need to set up the Python environment:

```bash
uv venv
source .venv/bin/activate
uv pip install -U -r ./requirements.txt
```

### Project Structure

The `launcher` simply loads `core.dll` plugin. It's only needed to keep the root directory clean, so it's written in C to avoid pulling in unnecessary DLLs.

The `core` contains the main logic of the program, and `public` is an auxiliary library with common things.

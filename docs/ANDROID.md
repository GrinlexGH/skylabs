# Android

I wasn't originally planning on supporting Android, but I had nothing better to do at my grandma's, so I decided to do it.

## Table of Contents
1. [SDL Android Project](#sdl-android-project)
2. [Android Studio Project Detection](#android-studio-project-detection)
3. [Custom Conan Gradle Task](#custom-conan-gradle-task)
4. [Copying Runtime Plugins And Custom Files](#copying-runtime-plugins-and-custom-files)
5. [CMAKE_FIND_ROOT_PATH_MODE_XXXXXXX](#cmake_find_root_path_mode_xxxxxxx)

## SDL Android Project

Since I use Conan with my custom recipes, I initially created a symlink to the Java source code in my Android project. This turned out to be ineffective, as there are files like `AndroidManifest.xml` that the user must override, but they are also updated by the library.

So I decided to simply move the Android project to a submodule and manually update SDL.

## Android Studio Project Detection

There's a bug in Android Studio that prevents a project from loading correctly. You just need to make sure the **package** field is present in `AndroidManifest.xml`:

```xml
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="org.grinlexstudios.skylabs"
    android:versionCode="1"
    android:versionName="1.0"
    android:installLocation="auto">
```

I spent several days fixing this...

## Custom Conan Gradle Task

The gradle task written [here](https://docs.conan.io/2/examples/cross_build/android/android_studio.html#build-gradle) is complete crap.
I [wrote](../android/app/build.gradle.kts) my own and slightly tweaked the build dependencies so that Conan would build the libraries before syncing with Android Studio.

## Copying Runtime Plugins And Custom Files

I just copy plugins to `jniLib` directory:

```cmake
    if(ARG_RUNTIME_PLUGINS)
       set(runtime_artifacts "")
       foreach(plugin_target IN LISTS ARG_RUNTIME_PLUGINS)
          list(APPEND runtime_artifacts "$<TARGET_FILE:${plugin_target}>")
       endforeach()

       # ...

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

# ...

# Shader target
skylabs_install_directories(${CURRENT_TARGET_NAME}
    DIRECTORIES ${SKYLABS_BUILD_DIR}/shaders
    DESTINATION .
)
```

## CMAKE_FIND_ROOT_PATH_MODE_XXXXXXX

By default, the Android toolchain searches for all libraries only in the NDK.
To override this, you need to do something like this:

```cmake
if(ANDROID)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
endif()
```

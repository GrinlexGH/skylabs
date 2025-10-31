# Enabling LTO
include(CheckIPOSupported)
check_ipo_supported(RESULT result)
if(result AND NOT(WIN32 AND MINGW))
    message(STATUS "LTO has been enabled")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# Global compilation options
if(IS_GNU_LIKE)
    add_compile_options(
        "-fdiagnostics-color=always" # I don't wanna read wall of white text on gray background
        "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-O3>"
    )

    if(NOT ANDROID)
        add_compile_options("-march=native" "-mtune=native")
    endif()

    if(WIN32 AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
        add_compile_options("-fansi-escape-codes")
    endif()
elseif(IS_MSVC_LIKE)
    add_compile_options(
        "/utf-8" # No, I don't want to use one byte encoding in source files
        "/Zc:__cplusplus"
        "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:/Ox>"
    )

    if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
        add_compile_options("/Gy" "/Gw")
    endif()

    # Clang-cl doesn't support much options
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(
            "/MP"
            "/analyze:external-"
        )

        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.35)
            add_compile_options("/Zc:checkGwOdr")
        endif()
    endif()
endif()

# Global compile definitions
if(WIN32)
    add_compile_definitions(UNICODE _UNICODE)
endif()

cmake_minimum_required(VERSION 3.19)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.3") 
    cmake_policy(SET CMP0207 NEW)
endif()

if(NOT OUTPUT_DIR)
    message(FATAL_ERROR "OUTPUT_DIR is not defined")
endif()

if(DEFINED COMPILER)
    cmake_path(GET COMPILER PARENT_PATH COMPILER_DIR)
endif()

file(GET_RUNTIME_DEPENDENCIES
    RESOLVED_DEPENDENCIES_VAR _r_deps
    UNRESOLVED_DEPENDENCIES_VAR _u_deps
    EXECUTABLES ${EXECUTABLE}
    LIBRARIES ${SHARED_LIBRARY}
    MODULES ${MODULE_LIBRARY}
    DIRECTORIES ${COMPILER_DIR}
    PRE_EXCLUDE_REGEXES
        "api-ms-win-.*" "ext-ms-.*"
        "libc\.so\..*" "libgcc_s\.so\..*" "libm\.so\..*" "libstdc\\+\\+\.so\..*"
    POST_EXCLUDE_REGEXES
        "^\/lib.*" "^\/usr\/lib.*"
        "C:[\\\/]Windows[\\\/].*"
    POST_INCLUDE_REGEXES
        "vcruntime.*" "msvcp.*"
)

if(_u_deps)
    message(WARNING "Unresolved dependencies: ${_u_deps}")
endif()

file(COPY ${_r_deps}
    DESTINATION ${OUTPUT_DIR}
    FOLLOW_SYMLINK_CHAIN
)

# Restore RUNPATH
# Sorry for using undocumented api
if(NOT UNIX)
    return()
endif()

set(_all_files ${EXECUTABLE} ${SHARED_LIBRARY} ${MODULE_LIBRARY})

foreach(_file IN LISTS _all_files)
    if(NOT EXISTS "${_file}")
        continue()
    endif()

    file(READ_ELF "${_file}" RUNPATH _old_rpath)
    if(NOT _old_rpath)
        file(READ_ELF "${_file}" RPATH _old_rpath)
    endif()

    string(REPLACE ";" ":" _old_rpath "${_old_rpath}")

    file(RPATH_CHANGE
        FILE "${_file}"
        OLD_RPATH "${_old_rpath}"
        NEW_RPATH "\$ORIGIN"
    )
endforeach()

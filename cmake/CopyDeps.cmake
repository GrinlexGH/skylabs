# OUTPUT_DIR - Directory where dependencies will be placed
# EXECUTABLE - `EXECUTABLES` file(GET_RUNTIME_DEPENDENCIES) parameter
# SHARED_LIBRARY - `LIBRARIES` file(GET_RUNTIME_DEPENDENCIES) parameter
# MODULE_LIBRARY - `MODULES` file(GET_RUNTIME_DEPENDENCIES) parameter
# DIRECTORIES - `DIRECTORIES` file(GET_RUNTIME_DEPENDENCIES) parameter
cmake_minimum_required(VERSION 3.20)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.3")
    cmake_policy(SET CMP0207 NEW)
endif()

message(STATUS "Resolving dependencies")

file(GET_RUNTIME_DEPENDENCIES
    RESOLVED_DEPENDENCIES_VAR _r_deps
    UNRESOLVED_DEPENDENCIES_VAR _u_deps
    EXECUTABLES ${EXECUTABLE}
    LIBRARIES ${SHARED_LIBRARY}
    MODULES ${MODULE_LIBRARY}
    DIRECTORIES ${DIRECTORIES}
    PRE_EXCLUDE_REGEXES
        "api-ms-win-.*" "ext-ms-.*"
        "libc\.so\..*" "libgcc_s\.so\..*" "libm\.so\..*" "libstdc\\+\\+\.so\..*"
    POST_EXCLUDE_REGEXES
        "^\/lib.*" "^\/usr\/lib.*"
        "C:[\\\/][Ww][Ii][Nn][Dd][Oo][Ww][Ss][\\\/].*"
    POST_INCLUDE_REGEXES
        "[Vv][Cc][Rr][Uu][Nn][Tt][Ii][Mm][Ee].*" "[Mm][Ss][Vv][Cc][Pp].*"
)

if(_u_deps)
    message(STATUS "Unresolved dependencies: ${_u_deps}")
endif()

file(COPY ${_r_deps}
    DESTINATION ${OUTPUT_DIR}
    FOLLOW_SYMLINK_CHAIN
)

message(STATUS "All dependencies successfully copied")

set(COPIED_DEPS "")

foreach(_dep ${_r_deps})
    cmake_path(GET _dep FILENAME _name)
    list(APPEND COPIED_DEPS "${OUTPUT_DIR}/${_name}")
endforeach()

set(FILES_TO_PATCH ${EXECUTABLE} ${SHARED_LIBRARY} ${MODULE_LIBRARY} ${COPIED_DEPS})

# Restore RUNPATH
include(${CMAKE_CURRENT_LIST_DIR}/PatchRunpath.cmake)

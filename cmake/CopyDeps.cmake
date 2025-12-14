cmake_minimum_required(VERSION 3.19)

if(NOT OUTPUT_DIR)
    message(FATAL_ERROR "OUTPUT_DIR is not defined")
endif()

if(NOT DEFINED APPS)
    set(APPS "")
endif()
if(NOT DEFINED LIBS)
    set(LIBS "")
endif()
if(NOT DEFINED MODS)
    set(MODS "")
endif()

file(GET_RUNTIME_DEPENDENCIES
    RESOLVED_DEPENDENCIES_VAR _r_deps
    UNRESOLVED_DEPENDENCIES_VAR _u_deps
    EXECUTABLES ${APPS}
    LIBRARIES   ${LIBS}
    MODULES     ${MODS}
    PRE_EXCLUDE_REGEXES
        [[api-ms-win-.*]]
        [[ext-ms-.*]]
        [[kernel32\.dll]]
        [[libc\.so\..*]] [[libgcc_s\.so\..*]] [[libm\.so\..*]] [[libstdc\+\+\.so\..*]]
    POST_EXCLUDE_REGEXES
        [[.*/system32/.*\.dll]]
        [[^/lib.*]]
        [[^/usr/lib.*]]
)

file(COPY ${_r_deps}
    DESTINATION ${OUTPUT_DIR}
    FOLLOW_SYMLINK_CHAIN
)

# Sorry for using undocumented api
# Restoring rpath
if(NOT UNIX)
    return()
endif()

foreach(MOD ${MODS})
    file(READ_ELF ${MOD} RUNPATH _old_rpath)
    if("${_old_rpath}" STREQUAL "")
        file(READ_ELF ${MOD} RPATH _old_rpath)
    endif()
    string(REPLACE ";" ":" _old_rpath "${_old_rpath}")
    file(RPATH_CHANGE FILE "${MOD}" OLD_RPATH "${_old_rpath}" NEW_RPATH "\$ORIGIN")
endforeach()

foreach(APP ${APPS})
    file(READ_ELF ${APP} RUNPATH _old_rpath)
    if("${_old_rpath}" STREQUAL "")
        file(READ_ELF ${APP} RPATH _old_rpath)
    endif()
    string(REPLACE ";" ":" _old_rpath "${_old_rpath}")
    file(RPATH_CHANGE FILE "${APP}" OLD_RPATH "${_old_rpath}" NEW_RPATH "\$ORIGIN")
endforeach()

foreach(LIB ${LIBS})
    file(READ_ELF ${LIB} RUNPATH _old_rpath)
    if("${_old_rpath}" STREQUAL "")
        file(READ_ELF ${LIB} RPATH _old_rpath)
    endif()
    string(REPLACE ";" ":" _old_rpath "${_old_rpath}")
    file(RPATH_CHANGE FILE "${LIB}" OLD_RPATH "${_old_rpath}" NEW_RPATH "\$ORIGIN")
endforeach()

if(NOT "${_u_deps}" STREQUAL "")
    message(WARNING "Unresolved dependencies: ${_u_deps}")
endif()

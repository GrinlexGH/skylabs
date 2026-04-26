# FILES_TO_PATCH - files to patch ._.
cmake_minimum_required(VERSION 3.20)

if(NOT UNIX)
    return()
endif()

if(NOT FILES_TO_PATCH)
    return()
endif()

find_program(PATCHELF_EXE patchelf)
if(NOT PATCHELF_EXE)
    return()
endif()

foreach(_file IN LISTS FILES_TO_PATCH)
    message("PATCHING ${_file}")

    if(EXISTS "${_file}")
        execute_process(
            COMMAND ${PATCHELF_EXE} --set-rpath "$ORIGIN" "${_file}"
            RESULT_VARIABLE _res
        )
        if(NOT _res EQUAL 0)
            message(STATUS "Failed to patch RUNPATH to $ORIGIN for ${_file}")
        endif()
    endif()
endforeach()

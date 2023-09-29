# src
set(projects
    launcher
    core
    public
)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(LIBS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/${ARCH}/$<CONFIG>/libs/)
set(DLLS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/${ARCH}/$<CONFIG>/bin/)
set(ROOT_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/${ARCH}/$<CONFIG>/)

foreach(SUBDIR ${projects})
    add_subdirectory("${SUBDIR}")
endforeach()

foreach(project ${projects})
if(MSVC)
    target_compile_options(${project} PRIVATE "/std:c17" "/std:c++20" "/WX" "/W4")
else()
    target_compile_options(${project} PRIVATE "-std=c++20" "-Wall" "-Wextra" "-Werror")
    if(WIN32)
        target_link_options(${project} PRIVATE "-municode")
    else()
        target_compile_options(${project} PRIVATE "-fPIC")
    endif()
endif()
endforeach()



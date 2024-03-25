# src
set(projects
    launcher
    core
    public
)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(LIBS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>/libs/)
set(DLLS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>/bin/)
set(ROOT_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/.output/$<CONFIG>/)

foreach(SUBDIR ${projects})
    add_subdirectory("${SUBDIR}")
endforeach()

foreach(project ${projects})
if(MSVC)
    target_compile_options(${project} PRIVATE "/WX" "/W4")
else()
    target_compile_options(${project} PRIVATE "-Wall" "-Werror")
    if(WIN32)
        target_link_options(${project} PRIVATE "-municode")
    else()
        target_compile_options(${project} PRIVATE "-fPIC")
    endif()
endif()
endforeach()


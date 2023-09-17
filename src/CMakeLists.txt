# src
set(projects
    launcher
    core
    public
)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

foreach(SUBDIR ${projects})
    add_subdirectory("${SUBDIR}")
endforeach()

foreach(project ${projects})
if(MSVC)
    target_compile_options(${project} PRIVATE "/std:c17" "/std:c++20" "/WX" "/W4")
else()
    target_compile_options(${project} PRIVATE "-std=c++20" "-Wall" "-Wextra" "-Werror")
    target_link_options(${project} PRIVATE "-municode")
endif()
endforeach()



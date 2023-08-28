# src
set(projects
    launcher
    coreinitializer
    public
)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

foreach(SUBDIR ${projects})
    add_subdirectory("${SUBDIR}")
endforeach()


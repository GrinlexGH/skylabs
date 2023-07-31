# src
set(projects
	tier0
	launcher
)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(${SRC_DIR}/public)

foreach(SUBDIR ${projects})
    add_subdirectory("${SUBDIR}")
endforeach()


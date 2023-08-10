# src
set(projects
	launcher
	coreinitializer
)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(${SRC_DIR}/public)

foreach(SUBDIR ${projects})
    add_subdirectory("${SUBDIR}")
endforeach()


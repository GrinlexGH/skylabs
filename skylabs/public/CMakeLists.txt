# public
set(CURRENT_TARGET_NAME public)

configure_file(
    ${PROJECT_SOURCE_DIR}/cmake/project_info.hpp.in
    ${CMAKE_CURRENT_SOURCE_DIR}/project_info.hpp
    @ONLY
)

set(SOURCES
    project_info.hpp
    application.hpp
    application.cpp
    logging.hpp
    dll_export.hpp
    os.hpp
    os.cpp
    resource_system.hpp
    resource_system.cpp
)

add_library(${CURRENT_TARGET_NAME} SHARED ${SOURCES})

foreach(source IN LISTS SOURCES)
    cmake_path(GET source PARENT_PATH source_directory)
    source_group("Source Files/${source_directory}" FILES "${source}")
endforeach()

set_target_properties(${CURRENT_TARGET_NAME}
    PROPERTIES
    DEFINE_SYMBOL "PUBLIC_EXPORTS"
)

target_link_libraries(${CURRENT_TARGET_NAME}
    PUBLIC
    nowide::nowide
)

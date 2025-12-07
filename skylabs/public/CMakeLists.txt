# public
set(CURRENT_TARGET_NAME public)

configure_file(
    ${PROJECT_SOURCE_DIR}/cmake/project_info.hpp.in
    project_info.hpp
    @ONLY
)

add_library(${CURRENT_TARGET_NAME})
set_target_properties(${CURRENT_TARGET_NAME} PROPERTIES DEFINE_SYMBOL "PUBLIC_EXPORTS")

include(GenerateExportHeader)
generate_export_header(${CURRENT_TARGET_NAME})

target_sources(
    ${CURRENT_TARGET_NAME}
    PRIVATE
        application.cpp
        os.cpp
        resource_system.cpp
    PUBLIC
    FILE_SET HEADERS
        FILES
            application.hpp
            logging.hpp
            dll_export.hpp
            os.hpp
            resource_system.hpp
    FILE_SET generated_headers
        TYPE HEADERS
        BASE_DIRS $<TARGET_PROPERTY:${CURRENT_TARGET_NAME},BINARY_DIR>
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/${CURRENT_TARGET_NAME}_export.h
            ${CMAKE_CURRENT_BINARY_DIR}/project_info.hpp
)

target_link_libraries(${CURRENT_TARGET_NAME} PUBLIC
    Boost::nowide
    stc::stc
    fmt::fmt
)

skylabs_configure_target(${CURRENT_TARGET_NAME})

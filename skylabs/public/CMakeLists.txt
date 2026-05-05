# public
set(CURRENT_TARGET_NAME public)

add_library(${CURRENT_TARGET_NAME})
set_target_properties(${CURRENT_TARGET_NAME} PROPERTIES DEFINE_SYMBOL "PUBLIC_EXPORTS")

include(GenerateExportHeader)
generate_export_header(${CURRENT_TARGET_NAME})

configure_file(
    ${PROJECT_SOURCE_DIR}/cmake/project_info.hpp.in
    project_info.hpp
    @ONLY
)

target_sources(
    ${CURRENT_TARGET_NAME}
    PRIVATE
        sdl/context.cpp
        sdl/filesystem.cpp
        sdl/sdl.cpp
        sdl/window.cpp
        sdl/ttf/context.cpp
        sdl/mixer/context.cpp
        sdl/mixer/mixer.cpp
        sdl/mixer/track.cpp
        sdl/mixer/audio.cpp
        application.cpp
        filesystem.cpp
        logging.cpp
        os.cpp
    PUBLIC
    FILE_SET HEADERS FILES
        sdl/context.hpp
        sdl/filesystem.hpp
        sdl/sdl.hpp
        sdl/window.hpp
        sdl/ttf/context.hpp
        sdl/mixer/context.hpp
        sdl/mixer/mixer.hpp
        sdl/mixer/track.hpp
        sdl/mixer/audio.hpp
        application.hpp
        dll_export.hpp
        filesystem.hpp
        logging.hpp
        os.hpp
        utils.hpp
        window.hpp
    FILE_SET generated_headers
        TYPE HEADERS
        BASE_DIRS $<TARGET_PROPERTY:${CURRENT_TARGET_NAME},BINARY_DIR>
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/${CURRENT_TARGET_NAME}_export.h
            ${CMAKE_CURRENT_BINARY_DIR}/project_info.hpp
)

target_precompile_headers(${CURRENT_TARGET_NAME} PUBLIC pch.hpp)

target_link_libraries(${CURRENT_TARGET_NAME} PUBLIC
    SDL3::SDL3
    SDL3_mixer::SDL3_mixer
    SDL3_ttf::SDL3_ttf
    Vulkan::Headers
    Boost::headers
    Boost::nowide
    Boost::container
    fmt::fmt
    stc::stc
)

skylabs_configure_target(${CURRENT_TARGET_NAME})

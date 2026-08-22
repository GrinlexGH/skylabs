# public
set(CURRENT_TARGET_NAME public)

add_library(${CURRENT_TARGET_NAME} SHARED)
set_target_properties(${CURRENT_TARGET_NAME} PROPERTIES DEFINE_SYMBOL "PUBLIC_EXPORTS")

include(GenerateExportHeader)
generate_export_header(${CURRENT_TARGET_NAME})

configure_file(${PROJECT_SOURCE_DIR}/cmake/project_info.hpp.in project_info.hpp @ONLY)

target_sources(
    ${CURRENT_TARGET_NAME}
    PRIVATE
    FILE_SET SOURCES FILES
        sdl/mixer/audio.cpp
        sdl/mixer/context.cpp
        sdl/mixer/mixer.cpp
        sdl/mixer/track.cpp
        sdl/ttf/context.cpp
        sdl/context.cpp
        sdl/filestream.cpp
        sdl/sdl.cpp
        sdl/window.cpp
        application.cpp
        filesystem.cpp
        logging.cpp
        os.cpp
        sdl/vulkan/vulkan.cpp
        sdl/vulkan/os_connector.cpp
        sdl/event_pump.cpp
    sdl/log_sink.cpp
    PUBLIC
    FILE_SET HEADERS FILES
        pch.hpp
        sdl/mixer/audio.hpp
        sdl/mixer/context.hpp
        sdl/mixer/mixer.hpp
        sdl/mixer/track.hpp
        sdl/ttf/context.hpp
        sdl/context.hpp
        sdl/filestream.hpp
        sdl/sdl.hpp
        sdl/window.hpp
        application.hpp
        dll_export.hpp
        filesystem.hpp
        input.hpp
        logging.hpp
        os.hpp
        utils.hpp
        window.hpp
        vulkan/os_connector.hpp
        sdl/vulkan/vulkan.hpp
        sdl/vulkan/os_connector.hpp
        event_pump.hpp
        sdl/event_pump.hpp
    sdl/log_sink.hpp
    FILE_SET generated
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_BINARY_DIR}
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/${CURRENT_TARGET_NAME}_export.h
            ${CMAKE_CURRENT_BINARY_DIR}/project_info.hpp
)

target_precompile_headers(${CURRENT_TARGET_NAME} PUBLIC pch.hpp)

target_link_libraries(${CURRENT_TARGET_NAME} PUBLIC
    Boost::headers
    Boost::nowide
    Boost::container
    frozen::frozen
    SDL3::SDL3
    SDL3_mixer::SDL3_mixer
    SDL3_ttf::SDL3_ttf
    fmt::fmt
    Vulkan::Headers
)

skylabs_configure_target(${CURRENT_TARGET_NAME}
    RUNTIME_PLUGINS
        SDL3_mixer::ogg
)

# shaders
set(CURRENT_TARGET_NAME shaders)

set(SHADERS
    shader.frag.glsl
    shader.vert.glsl
    shader.comp.glsl
)

set(SHADER_BINARY_DIR ${ROOT_OUTPUT_DIRECTORY}/shaders)

add_custom_command(
    MAIN_DEPENDENCY ${SHADER_BINARY_DIR}
    OUTPUT ${SHADER_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
    VERBATIM
)

if(USE_GLSLC)
    foreach(SHADER ${SHADERS})
        get_filename_component(SHADER_NAME ${SHADER} NAME)

        string(REGEX MATCH "([^.]+)\\.glsl$" _ ${SHADER_NAME})
        if(NOT CMAKE_MATCH_1)
            message(WARNING "Invalid shader name format: ${SHADER_NAME}")
            continue()
        endif()
        set(SHADER_STAGE_SUFFIX ${CMAKE_MATCH_1})

        if(SHADER_STAGE_SUFFIX STREQUAL "vert")
            set(SHADER_STAGE "vertex")
        elseif(SHADER_STAGE_SUFFIX STREQUAL "frag")
            set(SHADER_STAGE "fragment")
        elseif(SHADER_STAGE_SUFFIX STREQUAL "comp")
            set(SHADER_STAGE "compute")
        elseif(SHADER_STAGE_SUFFIX STREQUAL "tesc")
            set(SHADER_STAGE "tesscontrol")
        elseif(SHADER_STAGE_SUFFIX STREQUAL "tese")
            set(SHADER_STAGE "tesseval")
        elseif(SHADER_STAGE_SUFFIX STREQUAL "geom")
            set(SHADER_STAGE "geometry")
        elseif(SHADER_STAGE_SUFFIX STREQUAL "mesh")
            set(SHADER_STAGE "mesh")
        elseif(SHADER_STAGE_SUFFIX STREQUAL "task")
            set(SHADER_STAGE "task")
        elseif(SHADER_STAGE_SUFFIX STREQUAL "rgen")
            set(SHADER_STAGE "rgen")
        else()
            message(WARNING "Unknown shader stage suffix: ${SHADER_STAGE_SUFFIX}")
            continue()
        endif()

        get_filename_component(SHADER_NAME_WE ${SHADER} NAME_WE)
        set(SHADER_OUT_NAME "${SHADER_BINARY_DIR}/${SHADER_NAME_WE}.${SHADER_STAGE_SUFFIX}.spv")
        list(APPEND SHADER_OUT_NAMES ${SHADER_OUT_NAME})

        add_custom_command(
            OUTPUT ${SHADER_OUT_NAME}
            DEPENDS ${SHADER}
            COMMAND Vulkan::glslc
                "-fshader-stage=${SHADER_STAGE}"
                ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
                -o ${SHADER_OUT_NAME}
            COMMENT "Compiling ${SHADER_NAME} [${SHADER_STAGE}]"
            VERBATIM
        )
    endforeach()
else()
    foreach(SHADER ${SHADERS})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SHADER_OUT_NAME "${SHADER_BINARY_DIR}/${SHADER_NAME}.spv")
        list(APPEND SHADER_OUT_NAMES ${SHADER_OUT_NAME})
        add_custom_command(
            MAIN_DEPENDENCY ${SHADER}
            OUTPUT ${SHADER_OUT_NAME}
            COMMAND Vulkan::glslangValidator -V ${SHADER} "-o" ${SHADER_OUT_NAME}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM
        )
    endforeach()
endif()

add_custom_target(${CURRENT_TARGET_NAME} DEPENDS ${SHADER_OUT_NAMES})

foreach(FILE IN LISTS SHADERS)
    get_filename_component(SOURCE_PATH "${FILE}" PATH)
    string(REPLACE "/" "\\" SOURCE_PATH_GROUP "${SOURCE_PATH}")
    source_group("Source Files\\${SOURCE_PATH_GROUP}" FILES "${FILE}")
endforeach()

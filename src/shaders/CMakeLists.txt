# shaders
set(CURRENT_TARGET_NAME shaders)

set(SHADERS
    shader.frag.glsl
    shader.vert.glsl
    shader.comp.glsl
)

foreach(SHADER ${SHADERS})
    get_filename_component(SHADER_NAME "${SHADER}" NAME)
    get_filename_component(SHADER_NAME_WE "${SHADER}" NAME_WE)
    get_filename_component(SHADER_DIR "${SHADER}" DIRECTORY)

    if(SHADER_DIR STREQUAL "")
        set(SHADER_OUT_DIR "${SHADER_OUTPUT_DIRECTORY}")
    else()
        set(SHADER_OUT_DIR "${SHADER_OUTPUT_DIRECTORY}/${SHADER_DIR}")
    endif()

    file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

    # Get shader stage suffix from extension
    # shader.comp.glsl -> comp
    string(REGEX REPLACE ".*\\.([^.]+)\\.glsl$" "\\1" SHADER_STAGE_SUFFIX "${SHADER_NAME}")
    string(TOLOWER "${SHADER_STAGE_SUFFIX}" SHADER_STAGE_SUFFIX)

    # Same as [ "vert", "vertex",  "frag", "fragment" ]
    set(STAGE_MAP
        "vert;vertex"
        "frag;fragment"
        "comp;compute"
        "tesc;tesscontrol"
        "tese;tesseval"
        "geom;geometry"
        "mesh;mesh"
        "task;task"
        "rgen;rgen"
    )

    list(FIND STAGE_MAP "${SHADER_STAGE_SUFFIX}" STAGE_INDEX)

    if(NOT STAGE_INDEX EQUAL -1)
        # Get list element after stage suffix
        math(EXPR STAGE_INDEX "${STAGE_INDEX} + 1")
        list(GET STAGE_MAP ${STAGE_INDEX} SHADER_STAGE)
    else()
        message(WARNING "Unknown shader stage suffix: ${SHADER_STAGE_SUFFIX}")
        continue()
    endif()

    set(SHADER_OUT_NAME "${SHADER_OUT_DIR}/${SHADER_NAME_WE}.${SHADER_STAGE_SUFFIX}.spv")
    list(APPEND SHADER_OUT_NAMES "${SHADER_OUT_NAME}")

    if(USE_GLSLC)
        add_custom_command(
            OUTPUT "${SHADER_OUT_NAME}"
            COMMAND Vulkan::glslc
                "-fshader-stage=${SHADER_STAGE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}"
                -o "${SHADER_OUT_NAME}"
            DEPENDS "${SHADER}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMENT "Compiling ${SHADER_NAME} [${SHADER_STAGE}]"
            VERBATIM
        )
    else()
        add_custom_command(
            OUTPUT "${SHADER_OUT_NAME}"
            COMMAND Vulkan::glslangValidator
                -V "${SHADER}"
                -o "${SHADER_OUT_NAME}"
            DEPENDS "${SHADER}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMENT "Compiling ${SHADER_NAME}"
            VERBATIM
        )
    endif()
endforeach()

add_custom_target(${CURRENT_TARGET_NAME}
    DEPENDS ${SHADER_OUT_NAMES}
    SOURCES ${SHADERS}
)

foreach(FILE IN LISTS SHADERS)
    get_filename_component(SOURCE_PATH "${FILE}" PATH)
    string(REPLACE "/" "\\" SOURCE_PATH_GROUP "${SOURCE_PATH}")
    source_group("Source Files\\${SOURCE_PATH_GROUP}" FILES "${FILE}")
endforeach()

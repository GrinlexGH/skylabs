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

foreach(SHADER ${SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SHADER_OUT_NAME "${SHADER_BINARY_DIR}/${SHADER_NAME}.spv")
    list(APPEND SHADER_OUT_NAMES ${SHADER_OUT_NAME})
    add_custom_command(
        MAIN_DEPENDENCY ${SHADER}
        OUTPUT ${SHADER_OUT_NAME}
        COMMAND Vulkan::glslc ${SHADER} "-o" ${SHADER_OUT_NAME}
        VERBATIM
    )
endforeach()

add_custom_target(${CURRENT_TARGET_NAME} DEPENDS ${SHADER_OUT_NAMES})

foreach(FILE IN LISTS SHADERS)
    get_filename_component(SOURCE_PATH "${FILE}" PATH)
    string(REPLACE "/" "\\" SOURCE_PATH_GROUP "${SOURCE_PATH}")
    source_group("Source Files\\${SOURCE_PATH_GROUP}" FILES "${FILE}")
endforeach()

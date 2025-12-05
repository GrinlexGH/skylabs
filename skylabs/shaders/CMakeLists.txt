# shaders
set(CURRENT_TARGET_NAME shaders)

set(SOURCES
    shader.frag.glsl
    shader.vert.glsl
    shaderSwapchain.frag.glsl
    shaderSwapchain.vert.glsl
    shader.comp.glsl
)

foreach(shader ${SOURCES})
    cmake_path(GET shader FILENAME shader_name)
    cmake_path(GET shader STEM LAST_ONLY shader_name_we)
    cmake_path(GET shader_name_we EXTENSION LAST_ONLY shader_stage)
    string(REGEX REPLACE "^\\." "" shader_stage "${shader_stage}") # remove the first . character
    cmake_path(GET shader PARENT_PATH shader_dir)

    set(shader_out_dir "${SKYLABS_SHADER_OUTPUT_DIRECTORY}/${shader_dir}")

    string(TOLOWER "${shader_stage}" shader_stage)

    # Same as [ "vert", "vertex",  "frag", "fragment" ]
    set(stage_map
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

    list(FIND stage_map "${shader_stage}" stage_index)

    if(NOT stage_index EQUAL -1)
        # Get list element after stage suffix
        math(EXPR stage_index "${stage_index} + 1")
        list(GET stage_map ${stage_index} shader_stage_name)
    else()
        message(WARNING "Unknown shader stage suffix: ${shader_stage}")
        continue()
    endif()

    set(shader_out_path "${shader_out_dir}/${shader_name_we}.spv")
    list(APPEND shader_out_names "${shader_out_path}")

    add_custom_command(
        OUTPUT "${shader_out_path}"
        COMMAND ${CMAKE_COMMAND} -E make_directory ${shader_out_dir}
        COMMAND Vulkan::glslc
            "-fshader-stage=${shader_stage_name}"
            "${shader}"
            -o "${shader_out_path}"
        DEPENDS "${shader}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Compiling ${shader} [${shader_stage_name}]"
    )
endforeach()

add_custom_target(${CURRENT_TARGET_NAME}
    DEPENDS ${shader_out_names}
    SOURCES ${SOURCES}
)

if(ANDROID)
    # Copy shaders to android apk
    add_custom_command(
        TARGET ${CURRENT_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${SHADER_OUTPUT_DIRECTORY}
        ${SKYLABS_ANDROID_GENERATED_ASSETS_DIRECTORY}/shaders
        WORKING_DIRECTORY ${SHADER_OUTPUT_DIRECTORY}
        COMMENT "Copying shaders to apk"
    )
endif()

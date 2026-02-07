# shaders
set(CURRENT_TARGET_NAME shaders)

set(SOURCES
    shader.frag.slang
    shader.vert.slang
    shaderSwapchain.frag.slang
    shaderSwapchain.vert.slang
    shader.comp.slang
)

set(IMPORTS
    main.slang
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
        COMMAND Vulkan::slangc
            "${shader}"
            -entry "main"
            -stage "${shader_stage_name}"
            -o "${shader_out_path}"
        DEPENDS "${shader}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Compiling ${shader} [${shader_stage_name}]"
    )
endforeach()

add_custom_target(${CURRENT_TARGET_NAME}
    DEPENDS ${shader_out_names}
    SOURCES ${SOURCES} ${IMPORTS}
)

skylabs_configure_target(${CURRENT_TARGET_NAME})

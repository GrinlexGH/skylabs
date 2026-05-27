# shaders
set(CURRENT_TARGET_NAME shaders)

set(SOURCES
    shader.frag.slang
    shader.vert.slang
    shaderSwapchain.frag.slang
    shaderSwapchain.vert.slang
    shaderUI.frag.slang
    main.slang
)

set(SLANG_STAGE_vert "vertex")
set(SLANG_STAGE_frag "fragment")
set(SLANG_STAGE_comp "compute")
set(SLANG_STAGE_tesc "tesscontrol")
set(SLANG_STAGE_tese "tesseval")
set(SLANG_STAGE_geom "geometry")
set(SLANG_STAGE_mesh "mesh")
set(SLANG_STAGE_task "task")
set(SLANG_STAGE_rgen "rgen")

set(shader_out_names "")

foreach(shader IN LISTS SOURCES)
    set(shader_abs_path "${CMAKE_CURRENT_SOURCE_DIR}/${shader}")

    cmake_path(GET shader FILENAME shader_name)
    cmake_path(GET shader_name STEM LAST_ONLY shader_name_we)
    cmake_path(GET shader_name_we EXTENSION LAST_ONLY shader_stage)
    string(REGEX REPLACE "^\\." "" shader_stage "${shader_stage}")
    string(TOLOWER "${shader_stage}" shader_stage)

    cmake_path(GET shader PARENT_PATH shader_dir)

    set(shader_out_dir "${CMAKE_CURRENT_BINARY_DIR}/${shader_dir}")
    set(shader_out_path "${shader_out_dir}/${shader_name_we}.spv")

    set(depfile_path "${shader_out_dir}/${shader_name_we}.d")

    set(shader_stage_name "${SLANG_STAGE_${shader_stage}}")
    if(NOT shader_stage_name)
        continue()
    endif()

    list(APPEND shader_out_names "${shader_out_path}")

    add_custom_command(
        OUTPUT "${shader_out_path}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${shader_out_dir}"

        COMMAND slang::slangc
            "${shader_abs_path}"
            -entry "main"
            -stage "${shader_stage_name}"
            -target spirv
            -o "${shader_out_path}"
            -depfile "${depfile_path}"

        DEPENDS "${shader_abs_path}"
        DEPFILE "${depfile_path}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Compiling ${shader} [${shader_stage_name}]"
    )
endforeach()

add_custom_target(${CURRENT_TARGET_NAME} ALL
    DEPENDS ${shader_out_names}
    SOURCES ${SOURCES}
)

skylabs_configure_target(${CURRENT_TARGET_NAME}
    CUSTOM_FILES ${shader_out_names}
    CUSTOM_FILES_DESTINATION shaders
)

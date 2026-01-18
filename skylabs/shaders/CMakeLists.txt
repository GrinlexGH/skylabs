# shaders
set(CURRENT_TARGET_NAME shaders)

set(SOURCES
    shader.slang
    shaderSwapchain.slang
)

foreach(shader ${SOURCES})
    cmake_path(GET shader FILENAME shader_name)
    cmake_path(GET shader STEM LAST_ONLY shader_name_we)
    cmake_path(GET shader PARENT_PATH shader_dir)

    set(shader_out_dir "${SKYLABS_SHADER_OUTPUT_DIRECTORY}/${shader_dir}")

    foreach(stage IN ITEMS vertex fragment)
        set(stageShort vert)
        if(stage STREQUAL fragment)
            set(stageShort frag)
        endif()

        set(shader_out_path "${shader_out_dir}/${shader_name_we}.${stageShort}.spv")
        list(APPEND shader_out_names "${shader_out_path}")

        add_custom_command(
            OUTPUT "${shader_out_path}"
            COMMAND ${CMAKE_COMMAND} -E make_directory ${shader_out_dir}
            COMMAND Vulkan::slangc
            "${shader}"
            -entry "${stageShort}Main"
            -stage "${stage}"
            -o "${shader_out_path}"
            DEPENDS "${shader}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMENT "Compiling ${shader} [${stage}]"
        )
    endforeach()
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

skylabs_configure_target(${CURRENT_TARGET_NAME})

# shaders
set(CURRENT_TARGET_NAME shaders)

# skylabs_add_slang_shader(
#     <artifact_list_var_name> <source_list_var_name>
#     SOURCE <shader_path>
#     STAGE <stage>
#     [OUT_NAME <filename>] [DEFINES <definitions>...] [ENTRY <entry>]
# )
# Parameters:
#   DEFINES                     Each passes to -D
#   ENTRY                       Passes to --entry. Default is ${STAGE}Main
#   OUT_NAME                    Name of the .spv file. Default is [SOURCE STEM].${STAGE}
function(skylabs_add_slang_shader artifact_list source_list)
    set(options "")
    set(oneValueArgs SOURCE STAGE ENTRY OUT_NAME)
    set(multiValueArgs DEFINES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_SOURCE OR NOT ARG_STAGE)
        message(FATAL_ERROR "add_slang_shader: SOURCE and STAGE are required!")
    endif()

    if(NOT ARG_ENTRY)
        set(ARG_ENTRY "${ARG_STAGE}Main")
    endif()

    set(shader_abs_path "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_SOURCE}")

    cmake_path(GET ARG_SOURCE STEM shader_name)
    cmake_path(GET ARG_SOURCE PARENT_PATH shader_dir)

    if(NOT ARG_OUT_NAME)
        set(ARG_OUT_NAME "${shader_name}.${ARG_STAGE}")
    endif()

    set(shader_out_dir "${SKYLABS_BUILD_DIR}/shaders/${shader_dir}")
    set(shader_out_path "${shader_out_dir}/${ARG_OUT_NAME}.spv")
    set(depfile_dir "${CMAKE_CURRENT_BINARY_DIR}/${shader_dir}")
    set(depfile_path "${depfile_dir}/${ARG_OUT_NAME}.d")

    set(compile_defines "")
    foreach(def IN LISTS ARG_DEFINES)
        list(APPEND compile_defines "-D" "${def}")
    endforeach()

    # TODO: runtime compile
    add_custom_command(
        OUTPUT "${shader_out_path}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${shader_out_dir}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${depfile_dir}"
        COMMAND slang::slangc
            "${shader_abs_path}"
            -entry "${ARG_ENTRY}"
            -stage "${ARG_STAGE}"
            -target spirv
            -o "${shader_out_path}"
            -depfile "${depfile_path}"
            ${compile_defines}
        DEPENDS "${shader_abs_path}"
        DEPFILE "${depfile_path}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Compiling Slang: ${ARG_SOURCE} [${ARG_STAGE} / ${ARG_ENTRY}] -> ${ARG_OUT_NAME}.spv"
    )

    list(APPEND ${artifact_list} "${shader_out_path}")
    set(${artifact_list} "${${artifact_list}}" PARENT_SCOPE)

    list(APPEND ${source_list} "${ARG_SOURCE}")
    list(REMOVE_DUPLICATES ${source_list})
    set(${source_list} "${${source_list}}" PARENT_SCOPE)
endfunction()

skylabs_add_slang_shader(shader_outputs shader_sources
    SOURCE shader.slang
    STAGE fragment
    OUT_NAME shader.frag
)
skylabs_add_slang_shader(shader_outputs shader_sources
    SOURCE shader.slang
    STAGE vertex
    OUT_NAME shader.vert
)

skylabs_add_slang_shader(shader_outputs shader_sources
    SOURCE shaderSwapchain.slang
    STAGE fragment
    OUT_NAME shaderSwapchain.frag
)
skylabs_add_slang_shader(shader_outputs shader_sources
    SOURCE shaderSwapchain.slang
    STAGE vertex
    OUT_NAME shaderSwapchain.vert
)

add_custom_target(${CURRENT_TARGET_NAME} ALL
    DEPENDS ${shader_outputs}
    SOURCES ${shader_sources}
)

skylabs_configure_target(${CURRENT_TARGET_NAME})

skylabs_install_directories(${CURRENT_TARGET_NAME}
    DIRECTORIES ${SKYLABS_BUILD_DIR}/shaders
    DESTINATION .
)

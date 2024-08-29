# shaders
set(CURRENT_TARGET_NAME shaders)

set(SHADER_SOURCE_FILES
    shader.frag
    shader.vert
)

# Validate that source files have been passed
list(LENGTH SHADER_SOURCE_FILES FILE_COUNT)
if(FILE_COUNT EQUAL 0)
    message(FATAL_ERROR "Cannot create a shaders target without any source files")
endif()

set(SHADER_COMMAND)
set(SHADER_PRODUCTS)

foreach(SHADER_SOURCE IN LISTS SHADER_SOURCE_FILES)
    cmake_path(ABSOLUTE_PATH SHADER_SOURCE NORMALIZE)
    cmake_path(GET SHADER_SOURCE EXTENSION LAST_ONLY SHADER_NAME)
    string(SUBSTRING "${SHADER_NAME}" 1 -1 SHADER_NAME)

    # Build command
    list(APPEND SHADER_COMMAND COMMAND)
    list(APPEND SHADER_COMMAND Vulkan::glslc)
    list(APPEND SHADER_COMMAND "${SHADER_SOURCE}")
    list(APPEND SHADER_COMMAND "-o")
    list(APPEND SHADER_COMMAND "${ROOT_OUTPUT_DIRECTORY}/shaders/${SHADER_NAME}.spv")

    # Add product
    list(APPEND SHADER_PRODUCTS "${ROOT_OUTPUT_DIRECTORY}/shaders/${SHADER_NAME}.spv")

endforeach()

add_custom_target(
    ${CURRENT_TARGET_NAME} ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ROOT_OUTPUT_DIRECTORY}/shaders/
    COMMAND ${SHADER_COMMAND}
    COMMENT "Compiling Shaders..."
    SOURCES ${SHADER_SOURCE_FILES}
    BYPRODUCTS ${SHADER_PRODUCTS}
)

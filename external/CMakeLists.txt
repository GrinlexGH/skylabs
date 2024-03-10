#external
if(MSVC)
add_compile_options("/W3" "/std:c17")
else()
add_compile_options("-std=c17")
endif()

add_subdirectory(SDL)
add_subdirectory(GLFW)
set(GLAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/GLAD)
add_library(glad STATIC ${GLAD_DIR}/include/glad/gl.h ${GLAD_DIR}/src/gl.c)
target_include_directories(glad PUBLIC ${GLAD_DIR}/include/)

set(CURRENT_TARGET_NAME external)

add_library(${CURRENT_TARGET_NAME} INTERFACE)


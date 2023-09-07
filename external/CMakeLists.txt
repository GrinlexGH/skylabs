#external
if(MSVC)
add_compile_options("/W3" "/std:c17")
else()
add_compile_options("-std=c17")
endif()

add_subdirectory(SDL)

set(CURRENT_TARGET_NAME external)

add_library(${CURRENT_TARGET_NAME} INTERFACE)


#external
add_compile_options("/W3" "/std:c17" "/std:c++20")
add_subdirectory(SDL)

set(CURRENT_TARGET_NAME external)

add_library(${CURRENT_TARGET_NAME} INTERFACE)


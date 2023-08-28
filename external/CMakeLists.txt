#external
set(SKYLABS_BUILD ON)
add_subdirectory(SDL)
set(SKYLABS_BUILD OFF)

set(CURRENT_TARGET_NAME external)

add_library(${CURRENT_TARGET_NAME} INTERFACE)


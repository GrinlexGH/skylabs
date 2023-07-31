# tier0
set(CURRENT_TARGET_NAME tier0)

set(SOURCES
	dbg.cpp
	${PUBLIC_INCLUDE_DIR}/tier0/dbg.hpp
)

add_library(${CURRENT_TARGET_NAME} SHARED ${SOURCES})

set_target_properties(${CURRENT_TARGET_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/.output/${TARGET_ARCHITECTURE}/$<CONFIG>/bin/"
)

foreach(folder ${PUBLIC_FOLDERS})
	target_include_directories(${CURRENT_TARGET_NAME} PRIVATE ${PUBLIC_INCLUDE_DIR}/${folder}/)
endforeach()

target_link_libraries(${CURRENT_TARGET_NAME} SDL2::Core)
target_link_libraries(${CURRENT_TARGET_NAME} SDL2::Main)


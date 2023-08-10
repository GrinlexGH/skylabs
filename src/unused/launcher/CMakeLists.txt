# launcher
set(CURRENT_TARGET_NAME launcher)

set(SOURCES
	launcher.cpp
)

set (CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(${CURRENT_TARGET_NAME} SHARED ${SOURCES})

set_target_properties(${CURRENT_TARGET_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/.output/${ARCH}/$<CONFIG>/bin/"
)


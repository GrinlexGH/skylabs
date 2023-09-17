# core
set(CURRENT_TARGET_NAME core)

set(SOURCES
    main.cpp
)

set (CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(${CURRENT_TARGET_NAME} SHARED ${SOURCES})

set_target_properties(${CURRENT_TARGET_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/.output/${ARCH}/$<CONFIG>/bin/"
)

target_link_libraries(${CURRENT_TARGET_NAME}
    PRIVATE
    external
    public
)


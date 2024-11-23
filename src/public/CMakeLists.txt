# public
set(CURRENT_TARGET_NAME public)

set(SOURCES
    application.hpp
    application.cpp
    console.hpp
    console.cpp
    commandline.hpp
    commandline.cpp
    resourceloader.hpp
    unicode.hpp
    platform.hpp
    stc.hpp
    window.hpp
    version.hpp
)

if(WIN32)
    set(SOURCES
        ${SOURCES}
        winapp.cpp
    )
elseif(UNIX)
    set(SOURCES
        ${SOURCES}
        posixapp.cpp
    )
endif()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
add_library(${CURRENT_TARGET_NAME} SHARED ${SOURCES})

foreach(FILE IN LISTS SOURCES)
    get_filename_component(SOURCE_PATH "${FILE}" PATH)
    string(REPLACE "/" "\\" SOURCE_PATH_GROUP "${SOURCE_PATH}")
    source_group("Source Files\\${SOURCE_PATH_GROUP}" FILES "${FILE}")
endforeach()

set_target_properties(${CURRENT_TARGET_NAME}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBS_OUTPUT_DIRECTORY}/
    RUNTIME_OUTPUT_DIRECTORY ${DLLS_OUTPUT_DIRECTORY}/
    LIBRARY_OUTPUT_DIRECTORY ${DLLS_OUTPUT_DIRECTORY}/
)

target_compile_definitions(${CURRENT_TARGET_NAME} PRIVATE PUBLIC_DLL_EXPORT)

target_include_directories(${CURRENT_TARGET_NAME} INTERFACE
    $<BUILD_INTERFACE:${PUBLIC_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

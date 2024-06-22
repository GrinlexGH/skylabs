# public
set(CURRENT_TARGET_NAME public)

set(SOURCES
    application.hpp
    console.hpp
    console.cpp
    commandline.hpp
    commandline.cpp
    utilities.hpp
    unicode.hpp
    nowide/convert.hpp;
    nowide/is_string_container.hpp
    nowide/utf.hpp
    platform.hpp
    stc.hpp
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

set_target_properties(${CURRENT_TARGET_NAME}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBS_OUTPUT_DIRECTORY}/
    RUNTIME_OUTPUT_DIRECTORY ${DLLS_OUTPUT_DIRECTORY}/
    LIBRARY_OUTPUT_DIRECTORY ${DLLS_OUTPUT_DIRECTORY}/
)

target_compile_definitions(${CURRENT_TARGET_NAME}  PRIVATE PUBLIC_DLL_EXPORT)

target_include_directories(${CURRENT_TARGET_NAME} INTERFACE
    $<BUILD_INTERFACE:${PUBLIC_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# public
set(CURRENT_TARGET_NAME public)

set(SOURCES
    application.hpp
    application.cpp
    console.hpp
    console.cpp
    commandline.hpp
    commandline.cpp
    utilities.hpp
    utilities.cpp
    unicode.hpp
    nowide/convert.hpp;
    nowide/is_string_container.hpp
    nowide/utf.hpp
    platform.hpp
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

add_library(${CURRENT_TARGET_NAME} STATIC ${SOURCES})
target_include_directories(${CURRENT_TARGET_NAME} INTERFACE
    $<BUILD_INTERFACE:${PUBLIC_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)


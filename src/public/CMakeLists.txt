# public
set(CURRENT_TARGET_NAME public)

set(SOURCES
    application.cpp
    application.hpp
    macros.hpp
    icommandline.hpp
    commandline.cpp
    unicode.hpp
    unicode.cpp
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

add_library(${CURRENT_TARGET_NAME} OBJECT ${SOURCES})

set_target_properties(${CURRENT_TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(${CURRENT_TARGET_NAME} INTERFACE
    $<BUILD_INTERFACE:${PUBLIC_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)


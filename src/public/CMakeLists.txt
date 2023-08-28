# public
set(CURRENT_TARGET_NAME public)

set(SOURCES
	baseapplication.cpp
	baseapplication.hpp
	charconverters.cpp
	charconverters.hpp
	exception.cpp
	exception.hpp
)

add_library(${CURRENT_TARGET_NAME} OBJECT ${SOURCES})

set_target_properties(${CURRENT_TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(${CURRENT_TARGET_NAME} INTERFACE
    $<BUILD_INTERFACE:${PUBLIC_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)


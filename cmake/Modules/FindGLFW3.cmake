# Locate the glfw3 library
#
# This module defines the following variables:
#
# GLFW3_LIBRARY the name of the library;
# GLFW3_INCLUDE_DIR where to find glfw include files.
# GLFW3_FOUND true if both the GLFW3_LIBRARY and GLFW3_INCLUDE_DIR have been found.
#
# To help locate the library and include file, you can define a
# variable called GLFW3_ROOT which points to the root of the glfw library
# installation.
#

set(_glfw3_HEADER_SEARCH_DIRS
  "/include"
  "/usr/include"
  "/usr/local/include"
  "/opt/local/include"
  "${PROJECT_SOURCE_DIR}/libs/glfw*/include"
  "${CMAKE_SOURCE_DIR}/includes"
  "C:/Program Files (x86)/glfw/include"
)

set(_glfw3_LIB_SEARCH_DIRS
  "/usr/lib"
  "/usr/local/lib"
  "/opt/local/lib"
  "${PROJECT_SOURCE_DIR}/libs/glfw*/lib-vc2022"
  "${CMAKE_SOURCE_DIR}/lib"
  "${CMAKE_SOURCE_DIR}/libs"
  "C:/Program Files (x86)/glfw/lib"
)

# Check environment for root search directory
set(_glfw3_ENV_ROOT $ENV{GLFW3_ROOT})

if(NOT GLFW3_ROOT AND _glfw3_ENV_ROOT)
  set(GLFW3_ROOT ${_glfw3_ENV_ROOT})
endif()

# Put user specified location at beginning of search
if(GLFW3_ROOT)
  list(INSERT _glfw3_HEADER_SEARCH_DIRS 0 "${GLFW3_ROOT}/include")
  list(INSERT _glfw3_LIB_SEARCH_DIRS 0 "${GLFW3_ROOT}/lib")
endif()

file(GLOB GLFW3_LOCATION "${CMAKE_SOURCE_DIR}/libs/glfw*")

if(WIN32)
  if(MSVC)
    if(MSVC_TOOLSET_VERSION EQUAL 143)
      file(GLOB GLFW3_LIB_DIR "${CMAKE_SOURCE_DIR}/libs/glfw*/lib-vc2022")
    elseif(MSVC_TOOLSET_VERSION EQUAL 142)
      file(GLOB GLFW3_LIB_DIR "${CMAKE_SOURCE_DIR}/libs/glfw*/lib-vc2019")
    elseif(MSVC_TOOLSET_VERSION EQUAL 141)
      file(GLOB GLFW3_LIB_DIR "${CMAKE_SOURCE_DIR}/libs/glfw*/lib-vc2017")
    elseif(MSVC_TOOLSET_VERSION EQUAL 140)
      file(GLOB GLFW3_LIB_DIR "${CMAKE_SOURCE_DIR}/libs/glfw*/lib-vc2015")
    elseif(MSVC_TOOLSET_VERSION EQUAL 120)
      file(GLOB GLFW3_LIB_DIR "${CMAKE_SOURCE_DIR}/libs/glfw*/lib-vc2013")
    endif()
  else()
    file(GLOB GLFW3_LIB_DIR "${CMAKE_SOURCE_DIR}/libs/glfw*/lib-mingw-w64")
  endif()
elseif(APPLE)
  file(GLOB GLFW3_LIB_DIR "${CMAKE_SOURCE_DIR}/libs/glfw*/lib-universal")
endif()

# Search for the header
FIND_PATH(GLFW3_INCLUDE_DIR
  NAMES GLFW/glfw3.h
  PATHS ${_glfw3_HEADER_SEARCH_DIRS}
  HINTS "${GLFW3_LOCATION}/include"
)

# Search for the library
FIND_LIBRARY(GLFW3_LIBRARY
  NAMES glfw3 glfw glfw3.lib glfw.lib
  ENV LD_LIBRARY_PATH
  ENV DYLD_LIBRARY_PATH
  PATHS ${_glfw3_LIB_SEARCH_DIRS}
  HINTS ${GLFW3_LIB_DIR}
)

set(GLFW3_LIBRARIES ${GLFW3_LIBRARY} ${CMAKE_DL_LIBS})
set(GLFW3_INCLUDE_DIRS ${GLFW3_INCLUDE_DIR})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
  GLFW3
  REQUIRED_VARS
  GLFW3_LIBRARY GLFW3_INCLUDE_DIR
)

if(GLFW3_FOUND AND NOT TARGET GLFW3::GLFW3)
  add_library(GLFW3::GLFW3 UNKNOWN IMPORTED)
  set_target_properties(
    GLFW3::GLFW3
    PROPERTIES
    IMPORTED_LOCATION "${GLFW3_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${GLFW3_INCLUDE_DIRS}"
  )
endif()

mark_as_advanced(GLFW3_INCLUDE_DIR GLFW3_LIBRARY)

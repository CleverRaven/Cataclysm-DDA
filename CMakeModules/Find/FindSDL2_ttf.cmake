#.rst:
# FindSDL2_ttf
# -----------
#
# Locate SDL2_ttf library
#
# This module defines:
#
# ::
#
#   SDL2_TTF_LIBRARIES, the name of the library to link against
#   SDL2_TTF_INCLUDE_DIRS, where to find the headers
#   SDL2_TTF_FOUND, if false, do not try to link against
#   SDL2_TTF_VERSION_STRING - human-readable string containing the version of SDL2_ttf
#
#
#
# For backward compatibility the following variables are also set:
#
# ::
#
#   SDL2TTF_LIBRARY (same value as SDL2_TTF_LIBRARIES)
#   SDL2TTF_INCLUDE_DIR (same value as SDL2_TTF_INCLUDE_DIRS)
#   SDL2TTF_FOUND (same value as SDL2_TTF_FOUND)
#
#
#
# $SDL2DIR is an environment variable that would correspond to the
# ./configure --prefix=$SDL2DIR used in building SDL2.
#
# Created by Eric Wing.  This was influenced by the FindSDL2.cmake
# module, but with modifications to recognize OS X frameworks and
# additional Unix paths (FreeBSD, etc).

#=============================================================================
# Copyright 2005-2009 Kitware, Inc.
# Copyright 2012 Benjamin Eikel
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

if(NOT SDL2_TTF_INCLUDE_DIR AND SDL2TTF_INCLUDE_DIR)
  set(SDL2_TTF_INCLUDE_DIR ${SDL2TTF_INCLUDE_DIR} CACHE PATH "directory cache
entry initialized from old variable name")
endif()
find_path(SDL2_TTF_INCLUDE_DIR SDL_ttf.h
  HINTS
    ENV SDL2TTFDIR
    ENV SDL2DIR
    ${CMAKE_SOURCE_DIR}/dep/
  PATH_SUFFIXES SDL2
                # path suffixes to search inside ENV{SDL2DIR}
                include/SDL2 include
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(VC_LIB_PATH_SUFFIX lib/x64)
else()
  set(VC_LIB_PATH_SUFFIX lib/x86)
endif()

if(NOT SDL2_TTF_LIBRARY AND SDL2TTF_LIBRARY)
  set(SDL2_TTF_LIBRARY ${SDL2TTF_LIBRARY} CACHE FILEPATH "file cache entry
initialized from old variable name")
endif()
find_library(SDL2_TTF_LIBRARY
  NAMES SDL2_ttf
  HINTS
    ENV SDL2TTFDIR
    ENV SDL2DIR
    ${CMAKE_SOURCE_DIR}/dep/
  PATH_SUFFIXES lib ${VC_LIB_PATH_SUFFIX}
)

set(SDL2_TTF_LIBRARIES ${SDL2_TTF_LIBRARY})
set(SDL2_TTF_INCLUDE_DIRS ${SDL2_TTF_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL2_ttf
                                  REQUIRED_VARS SDL2_TTF_LIBRARIES SDL2_TTF_INCLUDE_DIRS
                                  VERSION_VAR SDL2_TTF_VERSION_STRING)

# for backward compatibility
set(SDL2TTF_LIBRARY ${SDL2_TTF_LIBRARIES})
set(SDL2TTF_INCLUDE_DIR ${SDL2_TTF_INCLUDE_DIRS})
set(SDL2TTF_FOUND ${SDL2_TTF_FOUND})

mark_as_advanced(SDL2_TTF_LIBRARY SDL2_TTF_INCLUDE_DIR)

if (NOT DYNAMIC_LINKING AND PKG_CONFIG_FOUND)
  if (NOT TARGET SDL2_ttf::SDL2_ttf-static)
    add_library(SDL2_ttf::SDL2_ttf-static STATIC IMPORTED)
    set_target_properties(SDL2_ttf::SDL2_ttf-static PROPERTIES
      IMPORTED_LOCATION ${SDL2_TTF_LIBRARY}
      INTERFACE_INCLUDE_DIRECTORIES ${SDL2_TTF_INCLUDE_DIRS}
    )
  endif()
  message(STATUS "Searching for SDL_ttf deps libraries --")
  if(MSYS2)
      pkg_check_modules(Freetype REQUIRED IMPORTED_TARGET freetype2)
      pkg_check_modules(harfbuzz REQUIRED IMPORTED_TARGET harfbuzz)
      pkg_check_modules(graphite REQUIRED IMPORTED_TARGET graphite2)
      target_link_libraries(PkgConfig::harfbuzz INTERFACE
        PkgConfig::graphite
        rpcrt4
      )
      target_link_libraries(SDL2_ttf::SDL2_ttf-static INTERFACE
        PkgConfig::Freetype
        PkgConfig::harfbuzz
      )
  else()
      find_package(Freetype REQUIRED)
      find_package(Harfbuzz REQUIRED)
      get_target_property(_loc harfbuzz::harfbuzz IMPORTED_LOCATION)
      set_target_properties(harfbuzz::harfbuzz PROPERTIES 
        IMPORTED_IMPLIB_RELEASE ${_loc}
        IMPORTED_IMPLIB_DEBUG ${_loc}
        IMPORTED_IMPLIB_RELWITHDEBINFO ${_loc}
      )
      target_link_libraries(SDL2_ttf::SDL2_ttf-static INTERFACE
        Freetype::Freetype
        harfbuzz::harfbuzz
      )
  endif()
  pkg_check_modules(BROTLI REQUIRED IMPORTED_TARGET libbrotlidec libbrotlicommon)
  if(MSYS2)
    pkg_check_modules(bzip2 REQUIRED IMPORTED_TARGET bzip2)
    target_link_libraries(PkgConfig::Freetype INTERFACE
        PkgConfig::bzip2
    )
  else()
    target_link_libraries(Freetype::Freetype INTERFACE
      PkgConfig::BROTLI
    )
  endif()
elseif(NOT TARGET SDL2_ttf::SDL2_ttf)
    add_library(SDL2_ttf::SDL2_ttf UNKNOWN IMPORTED)
    set_target_properties(SDL2_ttf::SDL2_ttf PROPERTIES
      IMPORTED_LOCATION ${SDL2_TTF_LIBRARY}
      INTERFACE_INCLUDE_DIRECTORIES ${SDL2_TTF_INCLUDE_DIRS}
    )
endif()
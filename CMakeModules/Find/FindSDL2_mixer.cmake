#.rst:
# FindSDL2_mixer
# -------------
#
# Locate SDL2_mixer library
#
# This module defines:
#
# ::
#
#   SDL2_MIXER_LIBRARIES, the name of the library to link against
#   SDL2_MIXER_INCLUDE_DIRS, where to find the headers
#   SDL2_MIXER_FOUND, if false, do not try to link against
#   SDL2_MIXER_VERSION_STRING - human-readable string containing the
#                              version of SDL2_mixer
#
#
#
# For backward compatibility the following variables are also set:
#
# ::
#
#   SDL2MIXER_LIBRARY (same value as SDL2_MIXER_LIBRARIES)
#   SDL2MIXER_INCLUDE_DIR (same value as SDL2_MIXER_INCLUDE_DIRS)
#   SDL2MIXER_FOUND (same value as SDL2_MIXER_FOUND)
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

if(NOT SDL2_MIXER_INCLUDE_DIR AND SDL2MIXER_INCLUDE_DIR)
  set(SDL2_MIXER_INCLUDE_DIR ${SDL2MIXER_INCLUDE_DIR} CACHE PATH "directory cache
entry initialized from old variable name")
endif()
find_path(SDL2_MIXER_INCLUDE_DIR SDL_mixer.h
  HINTS
    ENV SDL2MIXERDIR
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

if(NOT SDL2_MIXER_LIBRARY AND SDL2MIXER_LIBRARY)
  set(SDL2_MIXER_LIBRARY ${SDL2MIXER_LIBRARY} CACHE FILEPATH "file cache entry
initialized from old variable name")
endif()
find_library(SDL2_MIXER_LIBRARY
  NAMES SDL2_mixer
  HINTS
    ENV SDL2MIXERDIR
    ENV SDL2DIR
    ${CMAKE_SOURCE_DIR}/dep/
  PATH_SUFFIXES lib ${VC_LIB_PATH_SUFFIX}
)

set(SDL2_MIXER_LIBRARIES ${SDL2_MIXER_LIBRARY})
set(SDL2_MIXER_INCLUDE_DIRS ${SDL2_MIXER_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL2_mixer
                                  REQUIRED_VARS SDL2_MIXER_LIBRARIES SDL2_MIXER_INCLUDE_DIRS
                                  VERSION_VAR SDL2_MIXER_VERSION_STRING)

# for backward compatibility
set(SDL2MIXER_LIBRARY ${SDL2_MIXER_LIBRARIES})
set(SDL2MIXER_INCLUDE_DIR ${SDL2_MIXER_INCLUDE_DIRS})
set(SDL2MIXER_FOUND ${SDL2_MIXER_FOUND})

mark_as_advanced(SDL2_MIXER_LIBRARY SDL2_MIXER_INCLUDE_DIR)

if(NOT DYNAMIC_LINKING)
  if (NOT TARGET SDL2_mixer::SDL2_mixer-static)
    add_library(SDL2_mixer::SDL2_mixer-static STATIC IMPORTED)
    set_target_properties(SDL2_mixer::SDL2_mixer-static PROPERTIES
      IMPORTED_LOCATION ${SDL2_MIXER_LIBRARY}
      INTERFACE_INCLUDE_DIRECTORIES ${SDL2_MIXER_INCLUDE_DIRS}
    )
  endif()
elseif(NOT TARGET SDL2_mixer::SDL2_mixer)
    add_library(SDL2_mixer::SDL2_mixer STATIC IMPORTED)
    set_target_properties(SDL2_mixer::SDL2_mixer PROPERTIES
      IMPORTED_LOCATION ${SDL2_MIXER_LIBRARY}
      INTERFACE_INCLUDE_DIRECTORIES ${SDL2_MIXER_INCLUDE_DIRS}
    )
endif()

if(PKG_CONFIG_FOUND)
    message(STATUS "Searching for SDL_mixer deps libraries --")
    if(TARGET SDL2_mixer::SDL2_mixer-static)
        pkg_check_modules(FLAC REQUIRED IMPORTED_TARGET flac)
        target_link_libraries(SDL2_mixer::SDL2_mixer-static INTERFACE
            PkgConfig::FLAC
        )
        if(MSYS2)
            pkg_check_modules(libmpg123 REQUIRED IMPORTED_TARGET libmpg123)
            pkg_check_modules(opus REQUIRED IMPORTED_TARGET opus)
            pkg_check_modules(opusfile REQUIRED IMPORTED_TARGET opusfile)
            # SHARED only: find_package(Ogg REQUIRED)
            pkg_check_modules(ogg REQUIRED IMPORTED_TARGET ogg)
            target_link_libraries(PkgConfig::opusfile INTERFACE
                PkgConfig::ogg
                PkgConfig::opus
            )
            target_link_libraries(SDL2_mixer::SDL2_mixer-static INTERFACE
                PkgConfig::libmpg123
                PkgConfig::opusfile
            )
        endif()
    elseif(TARGET SDL2_mixer::SDL2_mixer)
        pkg_check_modules(FLAC REQUIRED IMPORTED_TARGET flac)
        target_link_libraries(SDL2_mixer::SDL2_mixer INTERFACE
            PkgConfig::FLAC
        )
    endif()
endif()

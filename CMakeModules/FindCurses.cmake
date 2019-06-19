#.rst:
# FindCurses
# ----------
#
# Find the curses include file and library
#
#
#
# ::
#
#   CURSES_FOUND - system has Curses
#   CURSES_INCLUDE_DIR - the Curses include directory
#   CURSES_LIBRARIES - The libraries needed to use Curses
#   CURSES_HAVE_CURSES_H - true if curses.h is available
#   CURSES_HAVE_NCURSES_H - true if ncurses.h is available
#   CURSES_HAVE_NCURSES_NCURSES_H - true if ncurses/ncurses.h is available
#   CURSES_HAVE_NCURSES_CURSES_H - true if ncurses/curses.h is available
#   CURSES_LIBRARY - set for backwards compatibility with 2.4 CMake
#
#
#
# Set CURSES_NEED_NCURSES to TRUE before the find_package() command if
# NCurses functionality is required.

#=============================================================================
# Copyright 2001-2009 Kitware, Inc.
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

find_library(CURSES_CURSES_LIBRARY NAMES curses )

find_library(CURSES_NCURSES_LIBRARY NAMES ncurses )
set(CURSES_USE_NCURSES FALSE)

if(CURSES_NCURSES_LIBRARY  AND ((NOT CURSES_CURSES_LIBRARY) OR CURSES_NEED_NCURSES))
  set(CURSES_USE_NCURSES TRUE)
endif()
# http://cygwin.com/ml/cygwin-announce/2010-01/msg00002.html
# cygwin ncurses stopped providing curses.h symlinks see above
# message.  Cygwin is an ncurses package, so force ncurses on
# cygwin if the curses.h is missing
if(CYGWIN)
  if(NOT EXISTS /usr/include/curses.h)
    set(CURSES_USE_NCURSES TRUE)
  endif()
endif()


# Not sure the logic is correct here.
# If NCurses is required, use the function wsyncup() to check if the library
# has NCurses functionality (at least this is where it breaks on NetBSD).
# If wsyncup is in curses, use this one.
# If not, try to find ncurses and check if this has the symbol.
# Once the ncurses library is found, search the ncurses.h header first, but
# some web pages also say that even with ncurses there is not always a ncurses.h:
# http://osdir.com/ml/gnome.apps.mc.devel/2002-06/msg00029.html
# So at first try ncurses.h, if not found, try to find curses.h under the same
# prefix as the library was found, if still not found, try curses.h with the
# default search paths.
if(CURSES_CURSES_LIBRARY  AND  CURSES_NEED_NCURSES)
  include(${CMAKE_CURRENT_LIST_DIR}/CheckLibraryExists.cmake)
  CHECK_LIBRARY_EXISTS("${CURSES_CURSES_LIBRARY}"
    wsyncup "" CURSES_CURSES_HAS_WSYNCUP)

  if(CURSES_NCURSES_LIBRARY  AND NOT  CURSES_CURSES_HAS_WSYNCUP)
    CHECK_LIBRARY_EXISTS("${CURSES_NCURSES_LIBRARY}"
      wsyncup "" CURSES_NCURSES_HAS_WSYNCUP)
    if( CURSES_NCURSES_HAS_WSYNCUP)
      set(CURSES_USE_NCURSES TRUE)
    endif()
  endif()

endif()


if(NOT CURSES_USE_NCURSES)
  find_file(CURSES_HAVE_CURSES_H curses.h )
  find_path(CURSES_CURSES_H_PATH curses.h )
  get_filename_component(_cursesLibDir "${CURSES_CURSES_LIBRARY}" PATH)
  get_filename_component(_cursesParentDir "${_cursesLibDir}" PATH)

  # for compatibility with older FindCurses.cmake this has to be in the cache
  # FORCE must not be used since this would break builds which preload a cache wqith these variables set
  set(CURSES_INCLUDE_PATH "${CURSES_CURSES_H_PATH}"
    CACHE FILEPATH "The curses include path")
  set(CURSES_LIBRARY "${CURSES_CURSES_LIBRARY}"
    CACHE FILEPATH "The curses library")
else()
# we need to find ncurses
  get_filename_component(_cursesLibDir "${CURSES_NCURSES_LIBRARY}" PATH)
  get_filename_component(_cursesParentDir "${_cursesLibDir}" PATH)

  find_file(CURSES_HAVE_NCURSES_H         ncurses.h)
  find_file(CURSES_HAVE_NCURSES_NCURSES_H ncurses/ncurses.h)
  find_file(CURSES_HAVE_NCURSES_CURSES_H  ncurses/curses.h)
  find_file(CURSES_HAVE_CURSES_H          curses.h
    HINTS "${_cursesParentDir}/include")

  find_path(CURSES_NCURSES_INCLUDE_PATH ncurses.h ncurses/ncurses.h
    ncurses/curses.h)
  find_path(CURSES_NCURSES_INCLUDE_PATH curses.h
    HINTS "${_cursesParentDir}/include")

  # for compatibility with older FindCurses.cmake this has to be in the cache
  # FORCE must not be used since this would break builds which preload
  # however if the value of the variable has NOTFOUND in it, then
  # it is OK to force, and we need to force in order to have it work.
  # a cache wqith these variables set
  # only put ncurses include and library into
  # variables if they are found
  if(NOT CURSES_NCURSES_INCLUDE_PATH AND CURSES_HAVE_NCURSES_NCURSES_H)
    get_filename_component(CURSES_NCURSES_INCLUDE_PATH
      "${CURSES_HAVE_NCURSES_NCURSES_H}" PATH)
  endif()
  if(CURSES_NCURSES_INCLUDE_PATH AND CURSES_NCURSES_LIBRARY)
    set( FORCE_IT )
    if(CURSES_INCLUDE_PATH MATCHES NOTFOUND)
      set(FORCE_IT FORCE)
    endif()
    set(CURSES_INCLUDE_PATH "${CURSES_NCURSES_INCLUDE_PATH}"
      CACHE FILEPATH "The curses include path" ${FORCE_IT})
    set( FORCE_IT)
    if(CURSES_LIBRARY MATCHES NOTFOUND)
      set(FORCE_IT FORCE)
    endif()
    set(CURSES_LIBRARY "${CURSES_NCURSES_LIBRARY}"
      CACHE FILEPATH "The curses library" ${FORCE_IT})
  endif()

  CHECK_LIBRARY_EXISTS("${CURSES_NCURSES_LIBRARY}"
    cbreak "" CURSES_NCURSES_HAS_CBREAK)
  if(NOT CURSES_NCURSES_HAS_CBREAK)
    find_library(CURSES_EXTRA_LIBRARY tinfo HINTS "${_cursesLibDir}")
    find_library(CURSES_EXTRA_LIBRARY tinfo )
    CHECK_LIBRARY_EXISTS("${CURSES_EXTRA_LIBRARY}"
      cbreak "" CURSES_TINFO_HAS_CBREAK)
  endif()
endif()

if (NOT CURSES_TINFO_HAS_CBREAK)
  find_library(CURSES_EXTRA_LIBRARY cur_colr HINTS "${_cursesLibDir}")
  find_library(CURSES_EXTRA_LIBRARY cur_colr )
endif()

find_library(CURSES_FORM_LIBRARY form HINTS "${_cursesLibDir}")
find_library(CURSES_FORM_LIBRARY form )

# for compatibility with older FindCurses.cmake this has to be in the cache
# FORCE must not be used since this would break builds which preload a cache
# qith these variables set
set(FORM_LIBRARY "${CURSES_FORM_LIBRARY}"
  CACHE FILEPATH "The curses form library")

# Need to provide the *_LIBRARIES
set(CURSES_LIBRARIES ${CURSES_LIBRARY})

if(CURSES_EXTRA_LIBRARY)
  set(CURSES_LIBRARIES ${CURSES_LIBRARIES} ${CURSES_EXTRA_LIBRARY})
endif()

if(CURSES_FORM_LIBRARY)
  set(CURSES_LIBRARIES ${CURSES_LIBRARIES} ${CURSES_FORM_LIBRARY})
endif()

# Proper name is *_INCLUDE_DIR
set(CURSES_INCLUDE_DIR ${CURSES_INCLUDE_PATH})

# handle the QUIETLY and REQUIRED arguments and set CURSES_FOUND to TRUE if
# all listed variables are TRUE
include(${CMAKE_CURRENT_LIST_DIR}/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Curses DEFAULT_MSG
  CURSES_LIBRARY CURSES_INCLUDE_PATH)

mark_as_advanced(
  CURSES_INCLUDE_PATH
  CURSES_LIBRARY
  CURSES_CURSES_INCLUDE_PATH
  CURSES_CURSES_LIBRARY
  CURSES_NCURSES_INCLUDE_PATH
  CURSES_NCURSES_LIBRARY
  CURSES_EXTRA_LIBRARY
  FORM_LIBRARY
  CURSES_LIBRARIES
  CURSES_INCLUDE_DIR
  CURSES_CURSES_HAS_WSYNCUP
  CURSES_NCURSES_HAS_WSYNCUP
  CURSES_NCURSES_HAS_CBREAK
  CURSES_TINFO_HAS_CBREAK
  )


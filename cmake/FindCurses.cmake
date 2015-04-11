# - Find the curses include file and library
#
#  CURSES_FOUND - system has Curses
#  CURSES_INCLUDE_DIR - the Curses include directory
#  CURSES_LIBRARIES - The libraries needed to use Curses
#  CURSES_HAVE_CURSES_H - true if curses.h is available
#  CURSES_HAVE_NCURSES_H - true if ncurses.h is available
#  CURSES_HAVE_NCURSES_NCURSES_H - true if ncurses/ncurses.h is available
#  CURSES_HAVE_NCURSES_CURSES_H - true if ncurses/curses.h is available
#  CURSES_LIBRARY - set for backwards compatibility with 2.4 CMake
#
# Set CURSES_NEED_NCURSES to TRUE before the FIND_PACKAGE() command if NCurses 
# functionality is required.

# Set CURSES_NEED_WIDE to TRUE before the FIND_PACKAGE() command if unicode
# functionality is required

SET(CURSES_LIBRARY_NAME "curses")
SET(NCURSES_LIBRARY_NAME "ncurses")
IF(CURSES_NEED_WIDE)
  SET(CURSES_LIBRARY_NAME "cursesw")
  SET(NCURSES_LIBRARY_NAME "ncursesw")
ENDIF(CURSES_NEED_WIDE)

FIND_LIBRARY(CURSES_CURSES_LIBRARY "${CURSES_LIBRARY_NAME}")
# MESSAGE(STATUS "CURSES! " ${CURSES_CURSES_LIBRARY})

FIND_LIBRARY(CURSES_NCURSES_LIBRARY "${NCURSES_LIBRARY_NAME}")
# MESSAGE(STATUS "NCURSES! " ${CURSES_NCURSES_LIBRARY})

SET(CURSES_USE_NCURSES FALSE)

IF(CURSES_NCURSES_LIBRARY  AND NOT  CURSES_CURSES_LIBRARY)
  SET(CURSES_USE_NCURSES TRUE)
ENDIF(CURSES_NCURSES_LIBRARY  AND NOT  CURSES_CURSES_LIBRARY)


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
IF(CURSES_CURSES_LIBRARY  AND  CURSES_NEED_NCURSES)
  INCLUDE(CheckLibraryExists)
  CHECK_LIBRARY_EXISTS("${CURSES_CURSES_LIBRARY}" 
    wsyncup "" CURSES_CURSES_HAS_WSYNCUP)

  IF(CURSES_NCURSES_LIBRARY  AND NOT  CURSES_CURSES_HAS_WSYNCUP)
    CHECK_LIBRARY_EXISTS("${CURSES_NCURSES_LIBRARY}" 
      wsyncup "" CURSES_NCURSES_HAS_WSYNCUP)
    IF( CURSES_NCURSES_HAS_WSYNCUP)
      SET(CURSES_USE_NCURSES TRUE)
    ENDIF( CURSES_NCURSES_HAS_WSYNCUP)
  ENDIF(CURSES_NCURSES_LIBRARY  AND NOT  CURSES_CURSES_HAS_WSYNCUP)

ENDIF(CURSES_CURSES_LIBRARY  AND  CURSES_NEED_NCURSES)


IF(NOT CURSES_USE_NCURSES)
  FIND_FILE(CURSES_HAVE_CURSES_H curses.h )
  FIND_FILE(CURSES_HAVE_CURSESW_H cursesw.h )
  FIND_PATH(CURSES_CURSES_H_PATH curses.h )
  FIND_PATH(CURSES_CURSESW_H_PATH cursesw.h )
  GET_FILENAME_COMPONENT(_cursesLibDir "${CURSES_CURSES_LIBRARY}" PATH)
  GET_FILENAME_COMPONENT(_cursesParentDir "${_cursesLibDir}" PATH)

  # for compatibility with older FindCurses.cmake this has to be in the cache
  # FORCE must not be used since this would break builds which preload a cache wqith these variables set
  SET(CURSES_INCLUDE_PATH "${CURSES_CURSES_H_PATH} ${CURSES_CURSESW_H_PATH}"
    CACHE FILEPATH "The curses include path")
  SET(CURSES_LIBRARY "${CURSES_CURSES_LIBRARY}"
    CACHE FILEPATH "The curses library")
ELSE(NOT CURSES_USE_NCURSES)
# we need to find ncurses
  GET_FILENAME_COMPONENT(_cursesLibDir "${CURSES_NCURSES_LIBRARY}" PATH)
  GET_FILENAME_COMPONENT(_cursesParentDir "${_cursesLibDir}" PATH)

  FIND_FILE(CURSES_HAVE_NCURSES_H         ncurses.h)
  FIND_FILE(CURSES_HAVE_NCURSES_NCURSES_H ncurses/ncurses.h)
  FIND_FILE(CURSES_HAVE_NCURSES_CURSES_H  ncurses/curses.h)
  FIND_FILE(CURSES_HAVE_CURSES_H          curses.h 
    HINTS "${_cursesParentDir}/include")

  FIND_FILE(CURSES_HAVE_NCURSESW_H         ncursesw.h)
  FIND_FILE(CURSES_HAVE_NCURSESW_NCURSES_H ncursesw/ncurses.h)
  FIND_FILE(CURSES_HAVE_NCURSESW_CURSES_H  ncursesw/curses.h)
  FIND_FILE(CURSES_HAVE_CURSESW_H          cursesw.h 
    HINTS "${_cursesParentDir}/include")

  #FIND_PATH(CURSES_NCURSES_INCLUDE_PATH ncurses.h ncurses/ncurses.h 
  #  ncurses/curses.h ncursesw.h ncursesw/ncurses.h ncursesw/curses.h cursesw.h)
  FIND_PATH(CURSES_NCURSES_INCLUDE_PATH curses.h
    HINTS "${_cursesParentDir}/include/ncursesw" "${_cursesParentDir}/include" "${_cursesParentDir}")

  # for compatibility with older FindCurses.cmake this has to be in the cache
  # FORCE must not be used since this would break builds which preload
  # a cache wqith these variables set
  # only put ncurses include and library into 
  # variables if they are found
  IF(CURSES_NCURSES_INCLUDE_PATH AND CURSES_NCURSES_LIBRARY)

    SET(CURSES_INCLUDE_PATH "${CURSES_NCURSES_INCLUDE_PATH}"
      CACHE FILEPATH "The curses include path")
    SET(CURSES_LIBRARY "${CURSES_NCURSES_LIBRARY}" 
      CACHE FILEPATH "The curses library")
  ENDIF(CURSES_NCURSES_INCLUDE_PATH AND CURSES_NCURSES_LIBRARY)

ENDIF(NOT CURSES_USE_NCURSES)



FIND_LIBRARY(CURSES_EXTRA_LIBRARY cur_colr HINTS "${_cursesLibDir}")
FIND_LIBRARY(CURSES_EXTRA_LIBRARY cur_colr )

SET(CURSES_FORM_LIBRARY_NAME "form")
IF(CURSES_NEED_WIDE)
  SET(CURSES_FORM_LIBRARY_NAME "formw")
ENDIF(CURSES_NEED_WIDE)

FIND_LIBRARY(CURSES_CURSES_LIBRARY "${CURSES_LIBRARY_NAME}")
FIND_LIBRARY(CURSES_FORM_LIBRARY "${CURSES_FORM_LIBRARY_NAME}" HINTS "${_cursesLibDir}")
FIND_LIBRARY(CURSES_FORM_LIBRARY "${CURSES_FORM_LIBRARY_NAME}" )

# for compatibility with older FindCurses.cmake this has to be in the cache
# FORCE must not be used since this would break builds which preload a cache
# qith these variables set
SET(FORM_LIBRARY "${CURSES_FORM_LIBRARY}"         
  CACHE FILEPATH "The curses form library")

# Need to provide the *_LIBRARIES
SET(CURSES_LIBRARIES ${CURSES_LIBRARY})

IF(CURSES_EXTRA_LIBRARY)
  SET(CURSES_LIBRARIES ${CURSES_LIBRARIES} ${CURSES_EXTRA_LIBRARY})
ENDIF(CURSES_EXTRA_LIBRARY)

IF(CURSES_FORM_LIBRARY)
  SET(CURSES_LIBRARIES ${CURSES_LIBRARIES} ${CURSES_FORM_LIBRARY})
ENDIF(CURSES_FORM_LIBRARY)

# Proper name is *_INCLUDE_DIR
SET(CURSES_INCLUDE_DIR ${CURSES_INCLUDE_PATH})

# handle the QUIETLY and REQUIRED arguments and set CURSES_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Curses DEFAULT_MSG
  CURSES_LIBRARY CURSES_INCLUDE_PATH)

MARK_AS_ADVANCED(
  CURSES_INCLUDE_PATH
  CURSES_LIBRARY
  CURSES_CURSES_INCLUDE_PATH
  CURSES_CURSES_LIBRARY
  CURSES_NCURSES_INCLUDE_PATH
  CURSES_NCURSES_LIBRARY
  CURSES_EXTRA_LIBRARY
  FORM_LIBRARY
  CURSES_FORM_LIBRARY
  CURSES_LIBRARIES
  CURSES_INCLUDE_DIR
  CURSES_CURSES_HAS_WSYNCUP
  CURSES_NCURSES_HAS_WSYNCUP
  CURSES_HAVE_CURSESW_H
  CURSES_HAVE_CURSES_H
  CURSES_HAVE_NCURSESW_CURSES_H
  CURSES_HAVE_NCURSESW_H
  CURSES_HAVE_NCURSESW_NCURSES_H
  CURSES_HAVE_NCURSES_CURSES_H
  CURSES_HAVE_NCURSES_H
  CURSES_HAVE_NCURSES_NCURSES_H
  )


#.rst:
# FindSDL2_image
# -------------
#
# Locate SDL2_image library
#
# This module defines:
#
# ::
#
#   SDL2_IMAGE_LIBRARIES, the name of the library to link against
#   SDL2_IMAGE_INCLUDE_DIRS, where to find the headers
#   SDL2_IMAGE_FOUND, if false, do not try to link against
#   SDL2_IMAGE_VERSION_STRING - human-readable string containing the
#                              version of SDL2_image
#
#
#
# For backward compatibility the following variables are also set:
#
# ::
#
#   SDL2IMAGE_LIBRARY (same value as SDL2_IMAGE_LIBRARIES)
#   SDL2IMAGE_INCLUDE_DIR (same value as SDL2_IMAGE_INCLUDE_DIRS)
#   SDL2IMAGE_FOUND (same value as SDL2_IMAGE_FOUND)
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

if(NOT SDL2_IMAGE_INCLUDE_DIR AND SDL2IMAGE_INCLUDE_DIR)
  set(SDL2_IMAGE_INCLUDE_DIR ${SDL2IMAGE_INCLUDE_DIR} CACHE PATH "directory cache
entry initialized from old variable name")
endif()
find_path(SDL2_IMAGE_INCLUDE_DIR SDL_image.h
  HINTS
    ENV SDL2IMAGEDIR
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

if(NOT SDL2_IMAGE_LIBRARY AND SDL2IMAGE_LIBRARY)
  set(SDL2_IMAGE_LIBRARY ${SDL2IMAGE_LIBRARY} CACHE FILEPATH "file cache entry
initialized from old variable name")
endif()
find_library(SDL2_IMAGE_LIBRARY
  NAMES SDL2_image
  HINTS
    ENV SDL2IMAGEDIR
    ENV SDL2DIR
    ${CMAKE_SOURCE_DIR}/dep/
  PATH_SUFFIXES lib ${VC_LIB_PATH_SUFFIX}
)

set(SDL2_IMAGE_LIBRARIES ${SDL2_IMAGE_LIBRARY})
set(SDL2_IMAGE_INCLUDE_DIRS ${SDL2_IMAGE_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL2_image
                                  REQUIRED_VARS SDL2_IMAGE_LIBRARIES SDL2_IMAGE_INCLUDE_DIRS
                                  VERSION_VAR SDL2_IMAGE_VERSION_STRING)

# for backward compatibility
set(SDL2IMAGE_LIBRARY ${SDL2_IMAGE_LIBRARIES})
set(SDL2IMAGE_INCLUDE_DIR ${SDL2_IMAGE_INCLUDE_DIRS})
set(SDL2IMAGE_FOUND ${SDL2_IMAGE_FOUND})

mark_as_advanced(SDL2_IMAGE_LIBRARY SDL2_IMAGE_INCLUDE_DIR)

if(NOT DYNAMIC_LINKING AND PKG_CONFIG_FOUND)
    if (NOT TARGET SDL2_image:SDL2_image-static)
      add_library(SDL2_image::SDL2_image-static STATIC IMPORTED)
      set_property(TARGET SDL2_image::SDL2_image-static
        PROPERTY IMPORTED_LOCATION ${SDL2_IMAGE_LIBRARY}
      )
    endif()
    message(STATUS "Searching for SDL_images deps libraries --")
    find_package(JPEG REQUIRED)
    find_package(PNG REQUIRED)
    find_package(TIFF REQUIRED)
    pkg_check_modules(WEBP REQUIRED IMPORTED_TARGET libwebp)
    target_link_libraries(TIFF::TIFF INTERFACE
        PkgConfig::WEBP
    )
    find_library(JBIG jbig REQUIRED)
    find_package(LibLZMA REQUIRED)
    target_link_libraries(SDL2_image::SDL2_image-static INTERFACE
      JPEG::JPEG
      PNG::PNG
      TIFF::TIFF
      ${JBIG}
      LibLZMA::LibLZMA
      ${ZSTD}
    )
    pkg_check_modules(ZIP REQUIRED IMPORTED_TARGET libzip)
    pkg_check_modules(ZSTD REQUIRED IMPORTED_TARGET libzstd)
    pkg_check_modules(DEFLATE REQUIRED IMPORTED_TARGET libdeflate)
    target_link_libraries(SDL2_image::SDL2_image-static INTERFACE
      PkgConfig::ZIP
      PkgConfig::ZSTD
      PkgConfig::DEFLATE
    )
    if(MSYS2)
       # only SHARED: find_package(libavif REQUIRED) 
       find_library(AVIF avif)
       add_library(libavif STATIC IMPORTED)
       set_target_properties(libavif PROPERTIES
            IMPORTED_LOCATION ${AVIF}
       )
       pkg_check_modules(libyuv REQUIRED IMPORTED_TARGET libyuv)
       pkg_check_modules(dav1d REQUIRED IMPORTED_TARGET dav1d)
       pkg_check_modules(rav1e REQUIRED IMPORTED_TARGET rav1e)
       pkg_check_modules(SvtAv1Enc REQUIRED IMPORTED_TARGET SvtAv1Enc)
       target_link_libraries(PkgConfig::rav1e INTERFACE
            ntdll
       )
       pkg_check_modules(aom REQUIRED IMPORTED_TARGET aom)
       target_link_libraries(libavif INTERFACE
            PkgConfig::libyuv
            PkgConfig::dav1d
            PkgConfig::rav1e
            PkgConfig::SvtAv1Enc
            PkgConfig::aom
       )
       pkg_check_modules(JXL REQUIRED IMPORTED_TARGET libjxl libjxl_threads)
       # only SHARED: find_package(hwy REQUIRED)
       pkg_check_modules(hwy REQUIRED IMPORTED_TARGET libhwy)
       target_link_libraries(PkgConfig::JXL INTERFACE
            PkgConfig::hwy
            PkgConfig::BROTLI
       )
       find_package(libjpeg-turbo REQUIRED)
       pkg_check_modules(Lerc REQUIRED IMPORTED_TARGET Lerc)
       target_link_libraries(SDL2_image::SDL2_image-static INTERFACE
           libavif
           PkgConfig::JXL
           PkgConfig::Lerc
           libjpeg-turbo::turbojpeg
       )
       pkg_check_modules(WEBPDEMUX REQUIRED IMPORTED_TARGET libwebpdemux)
       pkg_check_modules(WEBPDECODER REQUIRED IMPORTED_TARGET libwebpdecoder)
       target_link_libraries(TIFF::TIFF INTERFACE
           WEBPDEMUX
           WEBPDECODER
           JPEG::JPEG
       )
       pkg_check_modules(sharpyuv REQUIRED IMPORTED_TARGET libsharpyuv)
       target_link_libraries(PkgConfig::WEBP INTERFACE
            PkgConfig::sharpyuv
       )
    endif()
elseif(NOT TARGET SDL2_image::SDL2_image)
      add_library(SDL2_image::SDL2_image UNKNOWN IMPORTED)
      set_target_properties(SDL2_image::SDL2_image PROPERTIES
          IMPORTED_LOCATION ${SDL2_IMAGE_LIBRARY}
          INTERFACE_INCLUDE_DIRECTORIES ${SDL2_IMAGE_INCLUDE_DIRS}
      )
    target_link_libraries(SDL2_image::SDL2_image INTERFACE
      z
    )
endif()

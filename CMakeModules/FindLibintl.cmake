# Try to find Libintl functionality
# Once done this will define
#
#  LIBINTL_FOUND - system has Libintl
#  LIBINTL_INCLUDE_DIR - Libintl include directory
#  LIBINTL_LIBRARIES - Libraries needed to use Libintl
#
# TODO: This will enable translations only if Gettext functionality is
# present in libc. Must have more robust system for release, where Gettext
# functionality can also reside in standalone Gettext library, or the one
# embedded within kdelibs (cf. gettext.m4 from Gettext source).
 
# Copyright (c) 2006, Chusslove Illich, <caslav.ilic@gmx.net>
# Copyright (c) 2007, Alexander Neundorf, <neundorf@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(LIBINTL_INCLUDE_DIR AND LIBINTL_LIB_FOUND)
	set(Libintl_FIND_QUIETLY TRUE)
endif(LIBINTL_INCLUDE_DIR AND LIBINTL_LIB_FOUND)
 
find_path(LIBINTL_INCLUDE_DIR libintl.h)
 
set(LIBINTL_LIB_FOUND FALSE)
 
if(LIBINTL_INCLUDE_DIR)
	include(CheckFunctionExists)
	check_function_exists(dgettext LIBINTL_LIBC_HAS_DGETTEXT)

	if (LIBINTL_LIBC_HAS_DGETTEXT)
		set(LIBINTL_LIBRARIES)
		set(LIBINTL_LIB_FOUND TRUE)
	else (LIBINTL_LIBC_HAS_DGETTEXT)
		find_library(LIBINTL_LIBRARIES intl libintl libintl-8)
		if(LIBINTL_LIBRARIES)
			set(LIBINTL_LIB_FOUND TRUE)
		endif(LIBINTL_LIBRARIES)
	endif (LIBINTL_LIBC_HAS_DGETTEXT)
endif(LIBINTL_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libintl DEFAULT_MSG LIBINTL_LIBRARIES LIBINTL_INCLUDE_DIR)

mark_as_advanced(LIBINTL_INCLUDE_DIR LIBINTL_LIBRARIES LIBINTL_LIBC_HAS_DGETTEXT LIBINTL_LIB_FOUND)

#pragma once
#ifndef CATA_PLATFORM_WIN_H
#define CATA_PLATFORM_WIN_H

#if defined(_WIN32) || defined(_WIN64)
#   if !defined(WIN32_LEAN_AND_MEAN)
#       define WIN32_LEAN_AND_MEAN
#   endif
#
#   if !defined(NOMINMAX)
#       define NOMINMAX
#   endif
#
#   if !defined(STRICT)
#       define STRICT
#   endif
#
#   //unless overridden by the user, use win7
#   if !defined(NTDDI_VERSION) && !defined(_WIN32_WINNT)
#       define NTDDI_VERSION 0x06010000 //NTDDI_WIN7
#       define _WIN32_WINNT  0x0601     //_WIN32_WINNT_WIN7
#   endif
#
#   include <windows.h>
#endif

#endif //CATA_PLATFORM_WIN_H

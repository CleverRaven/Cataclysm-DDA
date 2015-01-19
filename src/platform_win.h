#ifndef CATA_PLATFORM_WIN_H
#define CATA_PLATFORM_WIN_H

#if defined(_WIN32) || defined(_WIN64)
#   define CATA_OS_WINDOWS
#
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
#   include <sdkddkver.h>
#
#   if !defined(NTDDI_VERSION) && !defined(_WIN32_WINNT)
#       define NTDDI_VERSION NTDDI_WIN7
#       define _WIN32_WINNT _WIN32_WINNT_WIN7
#   endif
#
#   include <windows.h>
#endif

#endif //CATA_PLATFORM_WIN_H

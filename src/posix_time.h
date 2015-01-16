#ifndef TIME_SPEC_H
#define TIME_SPEC_H

/* Windows lacks the nanosleep() function. The following code was stuffed
   together from GNUlib (http://www.gnu.org/software/gnulib/), which is
   licensed under the GPLv3. */
#include "platform_win.h"

#include <time.h>
#include <errno.h>

enum { BILLION = 1000 * 1000 * 1000 };

#if (defined CATA_OS_WINDOWS) && ! defined __CYGWIN__
/* Windows platforms.  */

#   ifdef __cplusplus
extern "C" {
#   endif

// Apparently this is defined by pthread.h, if that header had been included.
// _INC_TIME is defined in time.h for MSVC
#if !defined(_TIMESPEC_DEFINED) && !defined(_INC_TIME)
#define _TIMESPEC_DEFINED
struct timespec
{
  time_t tv_sec;
  long int tv_nsec;
};
#endif

#   ifdef __cplusplus
}
#   endif

#include <windows.h>

/* The Win32 function Sleep() has a resolution of about 15 ms and takes
   at least 5 ms to execute.  We use this function for longer time periods.
   Additionally, we use busy-looping over short time periods, to get a
   resolution of about 0.01 ms.  In order to measure such short timespans,
   we use the QueryPerformanceCounter() function.  */

int
nanosleep (const struct timespec *requested_delay,
           struct timespec *remaining_delay);

#endif
#endif

#ifndef _TIME_SPEC_H_
#define _TIME_SPEC_H_
/* Windows lacks the nanosleep() function. The following code was stuffed
   together from GNUlib (http://www.gnu.org/software/gnulib/), which is
   licensed under the GPLv3. */
#include <time.h>
#include <errno.h>

enum { BILLION = 1000 * 1000 * 1000 };

#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__
/* Windows platforms.  */
#ifndef _TIMESPEC_DEFINED

#   ifdef __cplusplus
extern "C" {
#   endif

struct timespec
{
  time_t tv_sec;
  long int tv_nsec;
};

#   ifdef __cplusplus
}
#   endif

#endif // _TIMESPEC_DEFINED

#ifndef NOMINMAX
#define NOMINMAX
#endif
# define WIN32_LEAN_AND_MEAN
# include <windows.h>

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

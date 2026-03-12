#include "get_version.h" // IWYU pragma: associated

#if (defined(_WIN32) || defined(MINGW)) && !defined(GIT_VERSION) && !defined(CROSS_LINUX) && !defined(_MSC_VER)

#ifndef VERSION
#define VERSION "0.J"
#endif

#else

#include "version.h"

#endif

const char *getVersionString()
{
    return VERSION;
}

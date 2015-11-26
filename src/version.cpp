#if (defined _WIN32 || defined WINDOWS || defined MINGW) && ! defined GIT_VERSION && ! defined CROSS_LINUX && !defined _MSC_VER

#ifndef VERSION
#define VERSION "0.C"
#endif

#else

#include "version.h"

#endif

#include "get_version.h"

const char *getVersionString()
{
    return VERSION;
}

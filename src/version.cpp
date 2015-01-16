#include "platform_win.h"
#if (defined CATA_OS_WINDOWS || defined MINGW) && ! defined CROSS_LINUX && !defined CATA_COMPILER_MSVC

#ifndef VERSION
#define VERSION "0.B" // FIXME: automatically generate VERSION based on git
#endif

#else

#include "version.h"

#endif

#include "get_version.h"

const char *getVersionString()
{
    return VERSION;
}

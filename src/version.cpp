#if (defined _WIN32 || defined WINDOWS || defined MINGW) && ! defined CROSS_LINUX && !defined _MSC_VER

#ifndef VERSION
#define VERSION "0.9" // FIXME: automatically generate VERSION based on git
#endif

#else

#include "version.h"

#endif

#include "get_version.h"

const char* getVersionString() { return VERSION; }

#if (defined _WIN32 || defined WINDOWS || defined MINGW) && ! defined CROSS_LINUX && !defined _MSC_VER

#ifndef VERSION
#define VERSION "0.7.1" // FIXME: automatically generate VERSION based on git
#endif

#else

#include "version.h"

#endif

const char* getVersionString() { return VERSION; }

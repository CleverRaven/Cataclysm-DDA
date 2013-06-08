#if (defined _WIN32 || defined WINDOWS || defined MINGW) && ! defined CROSS_LINUX && !defined _MSC_VER

#define VERSION "0.6-git" // FIXME: automatically generate VERSION based on git

#else

#include "version.h"

#endif

const char* getVersionString() { return VERSION; }

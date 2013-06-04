#if (defined _WIN32 || defined WINDOWS || defined MINGW)

#define VERSION "0.6-git" // FIXME: automatically generate VERSION based on git

#else

#include "version.h"

#endif

const char* getVersionString() { return VERSION; }

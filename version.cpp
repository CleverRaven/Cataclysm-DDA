#if (defined _WIN32 || defined WINDOWS)

#define VERSION "0.3" // FIXME: automatically generate VERSION based on git

#else

#include "version.h"

#endif

const char* getVersionString() { return VERSION; }

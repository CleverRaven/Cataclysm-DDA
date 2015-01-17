#ifndef CATA_COMPATIBILITY_H
#define CATA_COMPATIBILITY_H

//--------------------------------------------------------------------------------------------------
// HACK:
// std::to_string is broken on MinGW (as of 13/01/2015), but is fixed in MinGW-w64 gcc 4.8.
// See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52015
// This is a minimal workaround pending the issue begin solved (if ever).
// For proper linking these must be declared inline here, or moved to a cpp file.
//--------------------------------------------------------------------------------------------------
#include <string>

#define CATA_GCC_VER (__GNUC__ * 10000) + (__GNUC_MINOR__ * 100) + (__GNUC_PATCHLEVEL__)

#if defined(__MINGW32__) && !defined(__MINGW64__)
#   define CATA_NO_CPP11_STRING_CONVERSIONS
#elif defined(__MINGW64__) && (CATA_GCC_VER < 40800)
#   define CATA_NO_CPP11_STRING_CONVERSIONS
#endif

#if defined(CATA_NO_CPP11_STRING_CONVERSIONS)
#include <cstdio>
#include <limits>

inline std::string to_string(long const n)
{
    //- and \0
    constexpr int size = std::numeric_limits<long>::digits10 + 2;
    char buffer[size];
    std::snprintf(buffer, size, "%ld");
    return buffer;
}

inline std::string to_string(double const n)
{
    //- . \0 + snprintf default precision.
    constexpr int size = std::numeric_limits<double>::max_exponent10 + 6 + 3;
    char buffer[size];
    std::snprintf(buffer, size, "%f");
    return buffer;
}
#else //all other platforms
using std::to_string;
#endif //CATA_NO_CPP11_STRING_CONVERSIONS

#endif //CATA_COMPATIBILITY_H

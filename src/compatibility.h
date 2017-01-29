#pragma once
#ifndef CATA_COMPATIBILITY_H
#define CATA_COMPATIBILITY_H

//--------------------------------------------------------------------------------------------------
// HACK:
// std::to_string is broken on MinGW (as of 13/01/2015), but is fixed in MinGW-w64 gcc 4.8.
// It is also broken in Cygwin with no fix at this time.
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
#elif defined(__GNUC__) && !defined(__clang__) && (CATA_GCC_VER < 40800)
#   define CATA_NO_ADVANCE
#endif
// CATA_NO_CPP11_STRING_CONVERSIONS is also defined in Makefile for TARGETSYSTEM=CYGWIN

#if defined(CATA_NO_CPP11_STRING_CONVERSIONS)
#include <cstdio>
#include <limits>

inline std::string to_string( long const n )
{
    //- and \0
    constexpr int size = std::numeric_limits<long>::digits10 + 2;
    char buffer[size];
    snprintf( buffer, size, "%ld", n );
    return buffer;
}

inline std::string to_string( int const n )
{
    //- and \0
    constexpr int size = std::numeric_limits<int>::digits10 + 2;
    char buffer[size];
    snprintf( buffer, size, "%d", n );
    return buffer;
}

inline std::string to_string( unsigned int const n )
{
    //+ and \0 (no -)
    constexpr int size = std::numeric_limits<unsigned int>::digits10 + 2;
    char buffer[size];
    snprintf( buffer, size, "%u", n );
    return buffer;
}

inline std::string to_string( double const n )
{
    //- . \0 + snprintf default precision.
    constexpr int size = std::numeric_limits<double>::max_exponent10 + 6 + 3;
    char buffer[size];
    snprintf( buffer, size, "%f", n );
    return buffer;
}
#else //all other platforms
#include <type_traits>

//mirrors the valid overloads of std::to_string
template < typename T, typename std::enable_if < std::is_arithmetic<T>::value &&
           !std::is_same<T, bool>::value &&!std::is_same<T, wchar_t>::value &&
           !std::is_same<T, char>::value &&!std::is_same<T, char16_t>::value &&
           !std::is_same<T, char32_t>::value >::type * = nullptr >
std::string to_string( T const n )
{
    return std::to_string( n );
}
#endif //CATA_NO_CPP11_STRING_CONVERSIONS

#if defined(CATA_NO_ADVANCE)
template<typename I>
inline void std::advance( I iter, int num )
{
    while( num-- > 0 ) {
        iter++;
    }
}
#endif //CATA_NO_ADVANCE

#endif //CATA_COMPATIBILITY_H

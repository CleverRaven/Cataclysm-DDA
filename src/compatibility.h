#pragma once
#ifndef CATA_SRC_COMPATIBILITY_H
#define CATA_SRC_COMPATIBILITY_H

//--------------------------------------------------------------------------------------------------
// HACK:
// std::to_string is broken on MinGW (as of 13/01/2015), but is fixed in MinGW-w64 gcc 4.8.
// It is also broken in Cygwin with no fix at this time.
// See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52015
// This is a minimal workaround pending the issue begin solved (if ever).
// For proper linking these must be declared inline here, or moved to a cpp file.
//--------------------------------------------------------------------------------------------------
#include <string>

#define CATA_GCC_VER ((__GNUC__ * 10000) + (__GNUC_MINOR__ * 100) + (__GNUC_PATCHLEVEL__))

#if defined(__MINGW32__) && !defined(__MINGW64__)
#   define CATA_NO_CPP11_STRING_CONVERSIONS
#elif defined(__MINGW64__) && (CATA_GCC_VER < 40800)
#   define CATA_NO_CPP11_STRING_CONVERSIONS
#elif defined(__GNUC__) && !defined(__clang__) && (CATA_GCC_VER < 40800)
#   define CATA_NO_ADVANCE
#endif
// CATA_NO_CPP11_STRING_CONVERSIONS is also defined in Makefile for TARGETSYSTEM=CYGWIN

#if defined(CATA_NO_CPP11_STRING_CONVERSIONS)

// NOLINTNEXTLINE(cata-no-long)
std::string to_string( const long n );
// NOLINTNEXTLINE(cata-no-long)
std::string to_string( const unsigned long n );
std::string to_string( const long long n );
std::string to_string( const unsigned long long n );
std::string to_string( const int n );
std::string to_string( unsigned const int n );
std::string to_string( const double n );
#else //all other platforms
#include <type_traits>

//mirrors the valid overloads of std::to_string
template < typename T, typename std::enable_if < std::is_arithmetic<T>::value &&
           !std::is_same<T, bool>::value &&!std::is_same<T, wchar_t>::value &&
           !std::is_same<T, char>::value &&!std::is_same<T, char16_t>::value &&
           !std::is_same<T, char32_t>::value >::type * = nullptr >
std::string to_string( T n )
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

#endif // CATA_SRC_COMPATIBILITY_H

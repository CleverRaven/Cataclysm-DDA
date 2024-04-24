#pragma once
#ifndef CATA_SRC_CATA_UNREACHABLE_H
#define CATA_SRC_CATA_UNREACHABLE_H

// https://stackoverflow.com/a/65258501
#ifdef __GNUC__ // GCC 4.8+, Clang, Intel and other compilers compatible with GCC (-std=c++0x or above)
[[noreturn]] inline __attribute__( ( always_inline ) ) void cata_unreachable()
{
    __builtin_unreachable();
}
#elif defined(_MSC_VER) // MSVC
[[noreturn]] __forceinline void cata_unreachable()
{
    __assume( false );
}
#else // ???
inline void cata_unreachable() {}
#endif

#endif // CATA_SRC_CATA_UNREACHABLE_H

#pragma once
// NOLINTNEXTLINE(cata-header-guard)
// Due to an inability to suppress assert popups when building against mingw-w64
// and running on wine, and to suppress unused variable warnings in release builds,
// we are wrapping the assert macro so that we can substitute functional behavior.

// This copies the semantics of cassert, re-including the file re-defines the macro.
#undef cata_assert

// Might as well handle NDEBUG at the top level instead of just wrapping one variant.
#ifdef NDEBUG
// Use the expression to (hopefully) avoid unused variable warnings, hint compiler with
// unreachable intrinsics, and place the code in decltype to avoid actual evaluation.
#if defined(_MSC_VER)
#define cata_assert(exp) decltype((exp) ? void() : __assume(0))()
#elif defined(__GNUC__) || defined(__clang__)
#define cata_assert(exp) decltype((exp) ? void() : __builtin_unreachable())()
#else
#include <cstdlib>
#define cata_assert(exp) decltype((exp) ? void() : std::abort())()
#endif
#else
#ifdef _WIN32
#include <cstdlib>
#include <cstdio>
#define cata_assert(expression) \
    do { \
        if( expression ) { \
            break; \
        } \
        fprintf( stderr, "%s at %s:%d: Assertion `%s` failed.\n", __func__, __FILE__, __LINE__, #expression ); \
        std::abort(); \
    } while( false )
#else
#include <cassert>
#define cata_assert(expression) assert(expression)
#endif // _WIN32
#endif // NDEBUG

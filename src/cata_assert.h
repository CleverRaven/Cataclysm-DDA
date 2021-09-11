// NOLINTNEXTLINE(cata-header-guard)
// Due to an inability to suppress assert popups when building against mingw-w64 and running on wine
// We are wrapping the assert macro so that we can substitute functional behavior with that setup

// This copies the semantics of cassert, re-including the file re-defines the macro.
#undef cata_assert

// Might as well handle NDEBUG at the top level instead of just wrapping one variant.
#ifdef NDEBUG
#define cata_assert(expression) ((void)0)
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

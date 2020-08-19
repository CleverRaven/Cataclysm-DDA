// NOLINTNEXTLINE(cata-header-guard)
// Due to an inability to supress assert popups when building against mingw-w64 and running on wine
// We are wrapping the assert macro so that we can substitute functional behavior with that setup

// This copies the semantics of cassert, re-including the file re-defines the macro.
#undef cata_assert

// Might as well handle NDEBUG at the top level instead of just wrapping one variant.
#ifdef NDEBUG
#define cata_assert(_Expression) ((void)0)
#else
#ifdef _WIN32
#include <cstdlib>
#include <cstdio>
#define cata_assert(_Expression) \
    do { \
        if( _Expression ) { \
            break; \
        } \
        fprintf( stderr, "%s:%d: Assertion `%s` failed.\n", __FILE__, __LINE__, #_Expression ); \
        std::abort(); \
    } while( false );
#else
#include <cassert>
#define cata_assert(_Expression) assert(_Expression)
#endif // _WIN32
#endif // NDEBUG

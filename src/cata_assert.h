// NOLINTNEXTLINE(cata-header-guard)
// Due to an inability to suppress assert popups when building against mingw-w64 and running on wine
// We are wrapping the assert macro so that we can substitute functional behavior with that setup

// This copies the semantics of cassert, re-including the file re-defines the macro.
#undef cata_assert

// Might as well handle NDEBUG at the top level instead of just wrapping one variant.
#ifdef NDEBUG
#ifdef __GNUC__
#define cata_assert(expression) \
    do { \
        if( !( expression ) ) { \
            __builtin_unreachable(); \
        } \
    } while(false)
#else
// Goes the convoluted way to avoid unused variable warnings. The inner declval
// and decltype expressions are to workaround incorrect "left operand has no
// effect" warning on some compilers.
#define cata_assert(expression) \
    ( static_cast<void>( decltype( std::declval<decltype( expression )>(), int() )() ) )
#endif // __GNUC__
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

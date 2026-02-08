#include "cata_allocator_c.h"

#ifdef _MSC_VER
#define ENABLE_OVERRIDE 0
#else
// For posix_madvise
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _POSIX_C_SOURCE 200112L
// For MAP_ANONYMOUS
#ifdef __APPLE__
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _DARWIN_C_SOURCE
#else
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _DEFAULT_SOURCE
#endif
#endif

#include <rpmalloc/rpmalloc.c> // NOLINT(bugprone-suspicious-include)

alloc_funcs cata_get_alloc_funcs( void )
{
    alloc_funcs funcs = {
        rpmalloc,
        rpcalloc,
        rprealloc,
        rpfree,
    };
    return funcs;
}


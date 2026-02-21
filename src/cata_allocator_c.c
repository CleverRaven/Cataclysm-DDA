#include "cata_allocator_c.h"

#include <stdlib.h>

alloc_funcs cata_get_alloc_funcs( void )
{
    alloc_funcs funcs = {
        .mallocp = malloc,
        .callocp = calloc,
        .reallocp = realloc,
        .freep = free,
    };
    return funcs;
}


#include "cata_allocator_c.h"

alloc_funcs cata_get_alloc_funcs( void )
{
    alloc_funcs funcs = {
        .mallocp = NULL,
        .callocp = NULL,
        .reallocp = NULL,
        .freep = NULL,
    };
    return funcs;
}


#pragma once
#ifndef CATA_SRC_CATA_ALLOCATOR_C_H
#define CATA_SRC_CATA_ALLOCATOR_C_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// NOLINTNEXTLINE(modernize-use-using)
typedef struct {
    void *( *mallocp )( size_t );
    void *( *callocp )( size_t, size_t );
    void *( *reallocp )( void *, size_t );
    void ( *freep )( void * );
} alloc_funcs;

extern alloc_funcs cata_get_alloc_funcs( void );

#ifdef __cplusplus
}
#endif

#endif // CATA_SRC_CATA_ALLOCATOR_C_H

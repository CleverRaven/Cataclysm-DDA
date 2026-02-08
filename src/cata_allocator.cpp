#include "cata_allocator.h"
#include "cata_allocator_c.h"

// mimalloc internal asserts by default check NDEBUG which we don't set.
#define MI_DEBUG 0

#include <mimalloc/static.c> // NOLINT(bugprone-suspicious-include)
#include <mimalloc/mimalloc-new-delete.h> // IWYU pragma: keep

#if defined(TILES) || defined(SDL_SOUND)
#define SDL_SET_MEMORY_FUNCTIONS
#if defined(_MSC_VER) && defined(USE_VCPKG)
#include <SDL2/SDL_stdinc.h>
#else
#include <SDL_stdinc.h>
#endif
#endif

void cata::init_allocator()
{
    [[maybe_unused]] alloc_funcs funcs = cata_get_alloc_funcs();
#ifdef SDL_SET_MEMORY_FUNCTIONS
    SDL_SetMemoryFunctions(
        mi_new,
        mi_calloc,
        mi_realloc,
        mi_free );
#endif
}


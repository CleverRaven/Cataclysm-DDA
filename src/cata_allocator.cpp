#include "cata_allocator.h"
#include "cata_allocator_c.h"

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
        funcs.mallocp,
        funcs.callocp,
        funcs.reallocp,
        funcs.freep );
#endif
}

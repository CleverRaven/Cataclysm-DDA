#include "cata_allocator.h"

#ifndef __SANITIZE_ADDRESS__
#define SNMALLOC_USE_WAIT_ON_ADDRESS 1
#ifdef __ANDROID__
#define MALLOC_USABLE_SIZE_QUALIFIER const
#endif
#ifndef _WIN32
#include <snmalloc/override/malloc.cc> // NOLINT(bugprone-suspicious-include)
#endif
#include <snmalloc/override/new.cc> // NOLINT(bugprone-suspicious-include)
#endif

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
#ifdef SDL_SET_MEMORY_FUNCTIONS
#ifndef __SANITIZE_ADDRESS__
    SDL_SetMemoryFunctions(
        snmalloc::libc::malloc,
        snmalloc::libc::calloc,
        snmalloc::libc::realloc,
        snmalloc::libc::free );
#endif
#endif
}

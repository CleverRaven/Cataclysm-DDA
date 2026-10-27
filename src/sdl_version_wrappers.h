#pragma once
#ifndef CATA_SRC_SDL_VERSION_WRAPPERS_H
#define CATA_SRC_SDL_VERSION_WRAPPERS_H

// Minimal header for SDL version queries. Intentionally does NOT include
// sdl_wrappers.h, which pulls the whole of SDL for callers that only need the
// version numbers.

#include <SDL3/SDL_version.h>

struct SDLVersionInfo {
    int major;
    int minor;
    int patch;
};

// Returns the SDL version the program was compiled against.
// SDL_VERSION is a compile-time int constant; decompose with SDL_VERSIONNUM_MAJOR/MINOR/MICRO.
inline SDLVersionInfo GetCompiledSDLVersion()
{
    const int v = SDL_VERSION;
    return { SDL_VERSIONNUM_MAJOR( v ), SDL_VERSIONNUM_MINOR( v ), SDL_VERSIONNUM_MICRO( v ) };
}

// Returns the SDL version linked at runtime.
// SDL_GetVersion() returns a single int; decompose with SDL_VERSIONNUM_MAJOR/MINOR/MICRO.
inline SDLVersionInfo GetLinkedSDLVersion()
{
    const int v = SDL_GetVersion();
    return { SDL_VERSIONNUM_MAJOR( v ), SDL_VERSIONNUM_MINOR( v ), SDL_VERSIONNUM_MICRO( v ) };
}

#endif // CATA_SRC_SDL_VERSION_WRAPPERS_H

#pragma once
#ifndef CATA_SRC_SDL_VERSION_WRAPPERS_H
#define CATA_SRC_SDL_VERSION_WRAPPERS_H

// Minimal header for SDL version queries. Intentionally does NOT include
// sdl_wrappers.h, which defines SDL_MAIN_HANDLED and would break entry-point
// handling in main.cpp (WinMain / SDL_main plumbing).

#if defined(_MSC_VER) && defined(USE_VCPKG)
#   include <SDL2/SDL_version.h>
#else
#   include <SDL_version.h>
#endif

struct SDLVersionInfo {
    int major;
    int minor;
    int patch;
};

// Returns the SDL version the program was compiled against.
// SDL2: SDL_VERSION() macro fills SDL_version struct.
// SDL3: SDL_VERSION is a compile-time int constant; decompose with SDL_VERSIONNUM_MAJOR/MINOR/MICRO.
inline SDLVersionInfo GetCompiledSDLVersion()
{
#if SDL_MAJOR_VERSION >= 3
    const int v = SDL_VERSION;
    return { SDL_VERSIONNUM_MAJOR( v ), SDL_VERSIONNUM_MINOR( v ), SDL_VERSIONNUM_MICRO( v ) };
#else
    SDL_version v;
    SDL_VERSION( &v );
    return { v.major, v.minor, v.patch };
#endif
}

// Returns the SDL version linked at runtime.
// SDL2: SDL_GetVersion() fills an SDL_version struct.
// SDL3: SDL_GetVersion() returns a single int; decompose with SDL_VERSIONNUM_MAJOR/MINOR/MICRO.
inline SDLVersionInfo GetLinkedSDLVersion()
{
#if SDL_MAJOR_VERSION >= 3
    const int v = SDL_GetVersion();
    return { SDL_VERSIONNUM_MAJOR( v ), SDL_VERSIONNUM_MINOR( v ), SDL_VERSIONNUM_MICRO( v ) };
#else
    SDL_version v;
    SDL_GetVersion( &v );
    return { v.major, v.minor, v.patch };
#endif
}

#endif // CATA_SRC_SDL_VERSION_WRAPPERS_H

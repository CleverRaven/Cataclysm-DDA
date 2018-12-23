#pragma once
#ifndef SDL_SOUND_H
#define SDL_SOUND_H

#ifdef SDL_SOUND
#    if defined(_MSC_VER) && defined(USE_VCPKG)
#        include <SDL2/SDL_mixer.h>
#    else
#        include <SDL_mixer.h>
#    endif
#    include "sounds.h"
/**
 * Attempt to initialize an audio device.  Returns false if initialization fails.
 */
bool init_sound( void );
void shutdown_sound( void );
void play_music( std::string playlist );
void update_music_volume( void );
void load_soundset( void );

#else

inline bool init_sound( void )
{
    return false;
}
inline void shutdown_sound( void ) { }
inline void play_music( std::string playlist )
{
    ( void )playlist;
}
inline void update_music_volume( void ) { }
inline void load_soundset( void ) { }

#endif

#endif

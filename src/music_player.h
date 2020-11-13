#pragma once
#ifndef MUSIC_SRC_PLAYER_H
#define MUSIC_SRC_PLAYER_H

#if defined(SDL_SOUND)

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING 1

/* Open music playuer interface. */
/* Function returns bool value music_is_playing */
bool music_player_interface( const Character &p );

/* This function called from iuse.cpp */
/* if player uses mp3 player or somethinhg music electronics. */
/* Starts playing next music if  current music is over */
void music_player_next_music();

/* This function called from iuse.cpp */
/* after used mp3 player or somethinhg music electronics. */
/* Stops playing music */
void music_player_stop();

#else

inline bool music_player_interface( const Character & /*p*/ )
{
    return false;
}
inline void music_player_next_music( void ) { }
inline void music_player_stop( void ) { }

#endif // defined(SDL_SOUND)

#endif // MUSIC_SRC_PLAYER_H

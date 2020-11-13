#pragma once
#ifndef MUSIC_PLAYER_H
#define MUSIC_PLAYER_H

#include <string>

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING 1

/* Open music playuer interface. */
/* Function returns bool value music_is_playing */
bool music_player_interface( const Character &p );

/* This function called from iuse.cpp */
/* if player uses mp3 player or somethinhg music electronics. */
/* Starts playing next music if  current music is over */
void music_player_next_music( void );

/* This function called from iuse.cpp */
/* after used mp3 player or somethinhg music electronics. */
/* Stops playing music */
void music_player_stop( void );

/* Get random music file path from directory &dir */
std::string get_random_music_file( const std::string &dir );

/* draw <--|-----> scrollbar */
void draw_music_scrollbar( const catacurses::window &window, const point &start_point,
                           const nc_color &scrollbar_color, const int &length, const int &percent );



#endif // MUSIC_PLAYER_H

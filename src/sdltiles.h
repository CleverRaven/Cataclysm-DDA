#pragma once
#ifndef SDL_TILES_H
#define SDL_TILES_H

#if (defined TILES)
// defined in sdltiles.cpp
void to_map_font_dimension( int &w, int &h );
void from_map_font_dimension( int &w, int &h );
void to_overmap_font_dimension( int &w, int &h );
void reinitialize_framebuffer();
#else
// unchanged, nothing to be translated without tiles
void to_map_font_dimension( int &, int & ) { }
void from_map_font_dimension( int &, int & ) { }
void to_overmap_font_dimension( int &, int & ) { }
//in pure curses, the framebuffer won't need reinitializing
void reinitialize_framebuffer() { }
#endif
#endif

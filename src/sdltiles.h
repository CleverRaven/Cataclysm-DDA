#pragma once
#ifndef SDL_TILES_H
#define SDL_TILES_H

// defined in sdltiles.cpp
void to_map_font_dimension( int &w, int &h );
void from_map_font_dimension( int &w, int &h );
void to_overmap_font_dimension( int &w, int &h );
void reinitialize_framebuffer();

#endif

#pragma once
#ifndef GAME_UI_H
#define GAME_UI_H

namespace game_ui
{
void init_ui();
} // namespace game_ui

// defined in sdltiles.cpp
void to_map_font_dim_width( int &w );
void to_map_font_dim_height( int &h );
void to_map_font_dimension( int &w, int &h );
void from_map_font_dimension( int &w, int &h );
void to_overmap_font_dimension( int &w, int &h );
void reinitialize_framebuffer();

#endif // GAME_UI_H

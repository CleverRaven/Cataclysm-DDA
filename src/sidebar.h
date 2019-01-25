#pragma once
#ifndef SIDEBAR_H
#define SIDEBAR_H

#include "color.h"

class player;
namespace catacurses
{
class window;
} // namespace catacurses

void draw_HP( const player &p, const catacurses::window &w_HP, const bool wide );
void print_stamina_bar( const catacurses::window &w, const int y, const int x, const player &p,
                        const bool narrow );
void draw_time( const catacurses::window &time_window, const int y, const int x,
                const bool has_watch,
                const bool can_see_sun );
void draw_location( const catacurses::window &w, const int y, const int x,
                    const std::string &location_name );
void draw_ui_weather( const catacurses::window &w, const int y, const int x, const bool underground,
                      const nc_color &weather_color, const std::string &weather );
void draw_temperature( const catacurses::window &w, const int y, const int x,
                       const std::string &temperature );
void draw_moon( const catacurses::window &w, const int y, const int x, const int iPhase );
void draw_lighting( const catacurses::window &w, const int y, const int x );
void draw_sound( const catacurses::window &w, const int y, const int x, const bool is_deaf,
                 const int sound );
void draw_date( const catacurses::window &w, const int y, const int x );
void draw_safe_mode( const catacurses::window &w, const int y, const int x,
                     const bool safe_mode_off, const int iPercent );
void draw_ui_power( const catacurses::window &w, const int y, const int x, const int power,
                    const int max_power );
int get_int_digits( const int &digits );
#endif

#pragma once
#ifndef SIDEBAR_H
#define SIDEBAR_H

class player;
namespace catacurses
{
class window;
} // namespace catacurses

void draw_HP( const player &p, const catacurses::window &w_HP );
void print_stamina_bar( const player &p, const catacurses::window &w );
void draw_time( const catacurses::window &time_window, const bool has_watch,
                const bool can_see_sun );
int get_int_digits( const int &digits );
#endif

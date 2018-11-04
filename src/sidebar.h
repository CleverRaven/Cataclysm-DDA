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

#endif

#pragma once
#ifndef SIDEBAR_H
#define SIDEBAR_H

#include "player.h"

namespace catacurses
{
class window;
} // namespace catacurses

/** Print the player's stamina bar. **/
void print_stamina_bar( const player &p, const catacurses::window &w );

#endif

#pragma once
#ifndef DEBUG_MENU_H

#include "enums.h"

class player;

namespace debug_menu
{

void teleport_short();
void teleport_long();
void teleport_overmap();

void character_edit_menu();
void wishitem( player *p = nullptr, int x = -1, int y = -1, int z = -1 );
void wishmonster( const tripoint &p = tripoint_min );
void wishmutate( player *p );
void wishskill( player *p );
void mutation_wish();

class mission_debug;

}

#endif // DEBUG_MENU_H

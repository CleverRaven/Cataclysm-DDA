#pragma once
#ifndef DEBUG_MENU_H
#define DEBUG_MENU_H

struct tripoint;

namespace cata
{
template<typename T>
class optional;
} // namespace cata

class player;

namespace debug_menu
{

void teleport_short();
void teleport_long();
void teleport_overmap();

void character_edit_menu();
void wishitem( player *p = nullptr, int x = -1, int y = -1, int z = -1 );
void wishmonster( const cata::optional<tripoint> &p );
void wishmutate( player *p );
void wishskill( player *p );
void mutation_wish();
void draw_benchmark( int max_difference );

void debug();

} // namespace debug_menu

#endif // DEBUG_MENU_H

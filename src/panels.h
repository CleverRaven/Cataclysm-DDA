#pragma once
#ifndef PANELS_H
#define PANELS_H
#include "color.h"
#include "game.h"
#include "input.h"


class player;
namespace catacurses
{
class window;
} // namespace catacurses
enum face_type : int {
    face_human = 0,
    face_bird,
    face_bear,
    face_cat,
    num_face_types
};
//int power;
//const std::map<std::string, std::string> drug_map;
void decorate_panel( const std::string name, const catacurses::window &w );
void draw_limb( player &p, const catacurses::window &w );
void draw_char( player &p, const catacurses::window &w );
void draw_stat( player &p, const catacurses::window &w );
void draw_env1( const player &u, const catacurses::window &w );
void draw_env2( const player &u, const catacurses::window &w );
void draw_environment( const player &u, const catacurses::window &w );
void draw_messages( const catacurses::window &w );
void draw_mod1( const player &u, const catacurses::window &w );
void draw_mod2( const player &u, const catacurses::window &w );
void draw_modifiers( const player &u, const catacurses::window &w );
void draw_modifiers( const player &u, const catacurses::window &w );
void draw_lookaround( const catacurses::window &w );
void draw_mminimap( const catacurses::window &w );
void draw_compass( const catacurses::window &w );
void draw_panel_adm( const catacurses::window &w );
void shift_view();

nc_color stat_color( int stat );
nc_color value_color( int value );
std::string time_approx();
std::string get_temp( const player &u );
std::string get_moon();
std::string get_effects( const player &u );
std::string wind_string();
std::pair<nc_color, int> morale_stat( const player &u );
std::pair<nc_color, std::string> hunger_stat( const player &u );
std::pair<nc_color, std::string> temp_stat( const player &u );
std::pair<nc_color, std::string> thirst_stat( const player &u );
std::pair<nc_color, std::string> rest_stat( const player &u );
std::pair<nc_color, std::string> pain_stat( const player &u );
std::pair<nc_color, std::string> power_stat( const player &u );
face_type get_face_type( const player &u );
std::string morale_emotion( const int morale_cur, const face_type face,
                            const bool horizontal_style );
nc_color safe_color();
std::string get_armor( const player &u, body_part bp, const catacurses::window &w );
int get_int_digits( const int &digits );
#endif

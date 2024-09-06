#pragma once
#ifndef CATA_SRC_OVERMAP_UI_H
#define CATA_SRC_OVERMAP_UI_H

#include "avatar.h"
#include "coords_fwd.h"
#include "input_context.h"
#include "regional_settings.h"
#include "string_id.h"
#include "ui_manager.h"

constexpr int RANDOM_CITY_ENTRY = INT_MIN;

class uilist;

namespace catacurses
{
class window;
} // namespace catacurses

class input_context;
class nc_color;

struct weather_type;
using weather_type_id = string_id<weather_type>;

namespace ui
{

namespace omap
{

/**
 * Display overmap centered at the player's position.
 */
void display();
/**
 * Display overmap centered at the given NPC's position and visually move across their intended OMT path.
 */
void display_npc_path( tripoint_abs_omt starting_pos,
                       const std::vector<tripoint_abs_omt> &display_path );
/**
 * Display overmap like with @ref display() and display hordes.
 */
void display_hordes();
/**
 * Display overmap like with @ref display() and display the weather.
 */
void display_weather();
/**
 * Display overmap like with @ref display() and display the weather that is within line of sight.
 */
void display_visible_weather();
/**
 * Display overmap like with @ref display() and display scent traces.
 */
void display_scents();
/**
 * Display overmap like with @ref display() and display the given zone.
 */
void display_zones( const tripoint_abs_omt &center, const tripoint_abs_omt &select,
                    int iZoneIndex );
/**
 * Display overmap like with @ref display() and enable the overmap editor.
 */
void display_editor();

/**
 * Interactive point choosing; used as the map screen.
 * The map is initially center at the players position.
 * @returns The absolute coordinates of the chosen point or
 * invalid_point if canceled with Escape (or similar key).
 */
tripoint_abs_omt choose_point( bool show_debug_info = false );

/**
 * Same as above but start at z-level z instead of players
 * current z-level, x and y are taken from the players position.
 */
tripoint_abs_omt choose_point( int z, bool show_debug_info = false );
/**
 * Interactive point choosing; used as the map screen.
 * The map is initially centered on the @ref origin.
 * @returns The absolute coordinates of the chosen point or
 * invalid_point if canceled with Escape (or similar key).
 */
tripoint_abs_omt choose_point( const tripoint_abs_omt &origin, bool show_debug_info = false );

void setup_cities_menu( uilist &cities_menu, std::vector<city> &cities_container );

std::optional<city> select_city( uilist &cities_menu, std::vector<city> &cities_container,
                                 bool random = false );

void force_quit();
} // namespace omap

} // namespace ui

namespace overmap_ui
{
// drawing relevant data, e.g. what to draw.
struct overmap_draw_data_t {
    // draw editor.
    bool debug_editor = false;
    // draw scent traces.
    bool debug_scent = false;
    // draw debug info.
    bool debug_info = false;
    // darken explored tiles
    bool show_explored = true;
    // currently fast traveling
    bool fast_traveling = false;

    // draw zone location.
    tripoint_abs_omt select = tripoint_abs_omt( -1, -1, -1 );
    int iZoneIndex = -1;
    std::vector<tripoint_abs_omt> display_path = {};
    //center of UI view; usually player OMT position
    tripoint_abs_omt origin_pos = tripoint_abs_omt( -1, -1, -1 );
    //UI view cursor position
    tripoint_abs_omt cursor_pos = tripoint_abs_omt( -1, -1, -1 );
    //the UI adaptor for the overmap; this can keep the overmap displayed while turns are processed
    std::shared_ptr<ui_adaptor> ui;
    input_context ictxt;

    overmap_draw_data_t() {
        ictxt = input_context( "OVERMAP" );
    }
};

#if defined(TILES)
struct tiles_redraw_info {
    tripoint_abs_omt center;
    bool blink = false;
};
extern tiles_redraw_info redraw_info;
#endif

weather_type_id get_weather_at_point( const tripoint_abs_omt &pos );
std::tuple<char, nc_color, size_t> get_note_display_info( std::string_view note );

} // namespace overmap_ui
#endif // CATA_SRC_OVERMAP_UI_H

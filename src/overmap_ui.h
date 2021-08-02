#pragma once
#ifndef CATA_SRC_OVERMAP_UI_H
#define CATA_SRC_OVERMAP_UI_H

#include "coordinates.h"
#include "string_id.h"

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
tripoint_abs_omt choose_point();

/**
 * Same as above but start at z-level z instead of players
 * current z-level, x and y are taken from the players position.
 */
tripoint_abs_omt choose_point( int z );
/**
 * Interactive point choosing; used as the map screen.
 * The map is initially centered on the @ref origin.
 * @returns The absolute coordinates of the chosen point or
 * invalid_point if canceled with Escape (or similar key).
 */
tripoint_abs_omt choose_point( const tripoint_abs_omt &origin );

} // namespace omap

} // namespace ui

namespace overmap_ui
{
// drawing relevant data, e.g. what to draw.
struct draw_data_t {
    // draw editor.
    bool debug_editor = false;
    // draw scent traces.
    bool debug_scent = false;
    // draw zone location.
    tripoint_abs_omt select = tripoint_abs_omt( -1, -1, -1 );
    int iZoneIndex = -1;
};

weather_type_id get_weather_at_point( const tripoint_abs_omt &pos );
std::tuple<char, nc_color, size_t> get_note_display_info( const std::string &note );
void draw(
    const catacurses::window &w, const catacurses::window &wbar, const tripoint_abs_omt &center,
    const tripoint_abs_omt &orig, bool blink, bool show_explored, bool fast_scroll,
    input_context *inp_ctxt, const draw_data_t &data );
void create_note( const tripoint_abs_omt &curs );
} // namespace overmap_ui
#endif // CATA_SRC_OVERMAP_UI_H

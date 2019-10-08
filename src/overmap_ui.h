#pragma once
#ifndef OVERMAP_UI_H
#define OVERMAP_UI_H

#include "point.h"

namespace catacurses
{
class window;
} // namespace catacurses

class input_context;

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
void display_zones( const tripoint &center, const tripoint &select, int iZoneIndex );
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
tripoint choose_point();

/**
 * Same as above but start at z-level z instead of players
 * current z-level, x and y are taken from the players position.
 */
tripoint choose_point( int z );
/**
 * Interactive point choosing; used as the map screen.
 * The map is initially centered on the @ref origin.
 * @returns The absolute coordinates of the chosen point or
 * invalid_point if canceled with Escape (or similar key).
 */
tripoint choose_point( const tripoint &origin );

} // namespace omap

} // namespace ui

namespace overmap_ui
{
// drawing relevant data, e.g. what to draw.
struct draw_data_t {
    // draw monster groups on the overmap.
    bool debug_mongroup = false;
    // draw weather, e.g. clouds etc.
    bool debug_weather = false;
    // draw weather only around player position
    bool visible_weather = false;
    // draw editor.
    bool debug_editor = false;
    // draw scent traces.
    bool debug_scent = false;
    // draw zone location.
    tripoint select = tripoint( -1, -1, -1 );
    int iZoneIndex = -1;
};

void draw( const catacurses::window &w, const catacurses::window &wbar, const tripoint &center,
           const tripoint &orig, bool blink, bool show_explored, bool fast_scroll, input_context *inp_ctxt,
           const draw_data_t &data );
void create_note( const tripoint &curs );
} // namespace overmap_ui
#endif /* OVERMAP_UI_H */

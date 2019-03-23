#pragma once
#ifndef OVERMAP_UI_H
#define OVERMAP_UI_H

#include "enums.h"

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
void display_zones( const tripoint &center, const tripoint &select, const int iZoneIndex );
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

#endif /* OVERMAP_UI_H */

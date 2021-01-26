#pragma once
#ifndef CATA_SRC_CACHED_OPTIONS_H
#define CATA_SRC_CACHED_OPTIONS_H

// A collection of options which are accessed frequently enough that we don't
// want to pay the overhead of a string lookup each time one is tested.
// They should be updated when the corresponding option is changed (in options.cpp).

/**
 * Set to true when running unit tests.
 * Not a game option, but it's frequently used in the anyway.
 */
extern bool test_mode;

/**
 * Extended debugging mode, can be toggled during game.
 * If enabled some debug messages in the normal player message log are shown,
 * and other windows might have verbose display (e.g. vehicle window).
 */
extern bool debug_mode;

/**
 * Use tiles for display. Always false for ncurses build,
 * but can be toggled in sdl build.
 */
extern bool use_tiles;

/** Flow direction for the message log in the sidebar. */
extern bool log_from_top;
extern int message_ttl;
extern int message_cooldown;

/**
 * Circular distances.
 * If true, calculate distance in a realistic way [sqrt(dX^2 + dY^2)].
 * If false, use roguelike distance [maximum of dX and dY].
 */
extern bool trigdist;

/** 3D FoV enabled/disabled. */
extern bool fov_3d;

/** 3D FoV range, in Z levels, in both directions. */
extern int fov_3d_z_range;

/** Using isometric tileset. */
extern bool tile_iso;

/**
 * Whether to show the pixel minimap. Always false for ncurses build,
 * but can be toggled during game in sdl build.
 */
extern bool pixel_minimap_option;

/**
 * Items on the map with at most this distance to the player are considered
 * available for crafting, see inventory::form_from_map
*/
extern int PICKUP_RANGE;

#endif // CATA_SRC_CACHED_OPTIONS_H

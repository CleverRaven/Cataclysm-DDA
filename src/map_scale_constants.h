#pragma once
#ifndef CATA_SRC_MAP_SCALE_CONSTANTS_H
#define CATA_SRC_MAP_SCALE_CONSTANTS_H


// the size of the reality bubble in submaps (defined in SEEX)
// this should be an odd number, to ensure there's a little bit of room around
// the actual viewable playspace space due to how MAX_VIEW_DISTANCE evaluates
// MAPSIZE 11 results in a reality bubble approximately 60 tiles in radius
// MAPSIZE 21 results in approximately 120 tiles in radius
constexpr int MAPSIZE = 11;
constexpr int HALF_MAPSIZE = static_cast<int>( MAPSIZE / 2 );

// SEEX/SEEY define the size of a nonant, or grid.
// All map segments will need to be at least this wide.
constexpr int SEEX = 12;
constexpr int SEEY = SEEX;

constexpr int MAPSIZE_X = SEEX * MAPSIZE;
constexpr int MAPSIZE_Y = SEEY * MAPSIZE;

constexpr int HALF_MAPSIZE_X = SEEX * HALF_MAPSIZE;
constexpr int HALF_MAPSIZE_Y = SEEY * HALF_MAPSIZE;

// for MAPSIZE = 11 and SEEX = 12, this evaluates to 60
// MAPSIZE 21 results in 120
constexpr int MAX_VIEW_DISTANCE = SEEX * HALF_MAPSIZE;

// Maximum range at which ranged attacks can be executed.
constexpr int RANGE_HARD_CAP = MAX_VIEW_DISTANCE;


/**
 * Size of the overmap. This is the number of overmap terrain tiles per dimension in one overmap,
 * it's just like SEEX/SEEY for submaps.
*/
constexpr int OMAPX = 180;
constexpr int OMAPY = OMAPX;

// Size of a square unit of terrain saved to a directory.
constexpr int SEG_SIZE = 32;

// Size of a square unit of tile memory saved in a single file, in mm_submaps.
constexpr int MM_REG_SIZE = 8;

// Number of z-levels below 0 (not including 0).
constexpr int OVERMAP_DEPTH = 10;
// Number of z-levels above 0 (not including 0).
constexpr int OVERMAP_HEIGHT = 10;
// Total number of z-levels.
constexpr int OVERMAP_LAYERS = 1 + OVERMAP_DEPTH + OVERMAP_HEIGHT;

#endif // CATA_SRC_MAP_SCALE_CONSTANTS_H

#pragma once
#ifndef CATA_SRC_ANIMATION_H
#define CATA_SRC_ANIMATION_H

#include <optional>
#include <string>

#include "color.h"

enum explosion_neighbors {
    N_NO_NEIGHBORS = 0,
    N_NORTH = 1,

    N_SOUTH = 2,
    N_NS = 3,

    N_WEST = 4,
    N_NW = 5,
    N_SW = 6,
    N_NSW = 7,

    N_EAST = 8,
    N_NE = 9,
    N_SE = 10,
    N_NSE = 11,
    N_WE = 12,
    N_NWE = 13,
    N_SWE = 14,
    N_NSWE = 15
};

struct explosion_tile {
    explosion_neighbors neighborhood;
    nc_color color;
    // we use this if we don't want to use a color
    std::optional<std::string> tile_name;
};

#endif // CATA_SRC_ANIMATION_H

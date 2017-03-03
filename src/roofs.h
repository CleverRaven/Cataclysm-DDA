#pragma once
#ifndef ROOFS_H
#define ROOFS_H

#include <map>
#include <memory>
#include "string_id.h"

class map;
class game;
struct tripoint;

struct support_cache;

struct ter_t;
using ter_id = int_id<ter_t>;

enum direction : unsigned;

    class roofs
{
    private:
        game &g;
        // @todo Roofs should collapse even outside bubble, so map dependency is wrong
        map &m;

        std::unique_ptr<support_cache> cache;
    public:
        roofs( game &gm, map &mp );
        roofs( roofs && ) = delete;
        roofs( const roofs & ) = delete;
        ~roofs();

        /**
         * Get weight (in grams) a roof above given tile can hold before collapsing.
         * Returns -1 if there is no roof.
         */
        int get_support( const tripoint &origin );

        /**
         * Gets weight (in grams) a roof above given tile has of support from a given direction.
         */
        int get_support( const tripoint &origin, direction from_direction );

        /**
         * Generates a map of support quality around a given point.
         * Support quality propagates from supporting, non-collapsing tiles, through supporting, collapsing tiles.
         * @todo Make collapsing tiles decrease support quality of tiles they depend on for support.
         * @param origin Point around which support quality is calculated.
         */
        roofs &calc_support( const tripoint &origin );

        /**
         * Replaces specific terrain tiles with different terrain types for purposes of roof calculations.
         * Useful for predicting if destroying a given tile will cause other tiles to collapse.
         */
        roofs &replace_tiles( const std::map<tripoint, ter_id> &replacement );
};

#endif

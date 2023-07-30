#pragma once
#ifndef CATA_SRC_REACHABILITY_CACHE_H
#define CATA_SRC_REACHABILITY_CACHE_H

#include <bitset>
#include <cstdint>
#include <limits>

#include "coordinates.h"
#include "enums.h"
#include "game_constants.h"
#include "mdarray.h"

class reachability_cache_layer;
struct level_cache;
struct point;

// Implementation note:
// Code could be somewhat simpler if virtual inheritance was used, but cache is a performance-critical structure
// so templates were used as an attempt to reduce the overhead of virtual method calls.

template<bool Horizontal, typename ... Params>
struct reachability_cache_specialization {};

// specialization for horizontal cache
template<>
struct reachability_cache_specialization<true, level_cache> {

    static bool dynamic_fun( reachability_cache_layer &layer, const point &p,
                             const point &dir, const level_cache &this_lc );

    // returns "false" if there is no LOS, "true" is "maybe there is LOS"
    static bool test( int d, int cache_v );

    // returns true if transparency cache is still dirty
    static bool source_cache_dirty( const level_cache &this_lc );
};

// specialization for vertical cache
template<>
struct reachability_cache_specialization<false, level_cache, level_cache> {
    static bool dynamic_fun( reachability_cache_layer &layer, const point &p,
                             const point &dir, const level_cache &this_lc, const level_cache &floor_lc );

    // returns "false" if there is no LOS, "true" is "maybe there is LOS"
    static bool test( int d, int cache_v );

    // returns true if transparency or floor cache is still dirty
    static bool source_cache_dirty( const level_cache &this_lc, const level_cache &floor_lc );
};

class reachability_cache_layer
{
    public:
        using Q = reachability_cache_quadrant;
        using ElType = uint8_t;
        using QLayers = std::array<reachability_cache_layer, enum_traits<Q>::size>;
        // max distance the cache supports
        // all checks for distance that exceeds MAX_D will return "false"
        static constexpr int MAX_D = std::numeric_limits<ElType>::max() - 3;
    private:
        cata::mdarray<ElType, point_bub_ms> cache;

        bool update( const point &p, ElType value );
        ElType &operator[]( const point &p );
        const ElType &operator[]( const point &p ) const;
        ElType get_or( const point &p, ElType def ) const;

        template<bool Horizontal, typename ... Params>
        friend class reachability_cache;
        template<bool Horizontal, typename ... Params>
        friend struct reachability_cache_specialization;
};

/**
 *  This is DP-based cache/filter that can reject bresenham-based LOS checks in O(1) time.
 *
 *  Each tile stores 4 numbers (single byte each), representing the four quadrants,
 *  each number is the maximal distance it's possible to reach from the "current" tile
 *  into the corresponding quadrant, **using only two cardinal directions.**
 *
 *  I.e. for NW quadrant, two cardinal directions would be N and W,
 *  and "tested" paths could only consist of N and W, i.e. N→N→N is a valid path of distance 3,
 *  W→N→W→N is a valid path of distance 4.  W→N→N→E→N→N is an invalid path and is not considered.
 *  (For the sake of diagonal transparency  N→W is considered a valid path if NW diagonal direction is valid).
 *
 *  It's easy to see that bresenham line starting from the "current tile" could only reach the "target tile",
 *  if  there exists a path (that uses above rules) from "current tile" to the "target tile".
 *  Bresenham line is a special case of such path.
 *
 *  The idea is to test the |dx| + |dy| distance between two points against stored maximal
 *  "path distance" for the quadrant. If the stored maximal distance is less than the actual distance,
 *  we know in advance that the result of bresenham will be negative.
 *
 * @tparam Horizontal  true if cache is horizontal (for points on the same z-level)
 *                      false if cache is for inter-z-lev checks
 * @tparam Params param list needed for cache building
 */
template<bool Horizontal, typename ... Params>
class reachability_cache
{
    private:
        using Q = reachability_cache_layer::Q;
        using Spec = reachability_cache_specialization<Horizontal, Params...>;
        reachability_cache_layer::QLayers layers = {};

        // fields:
        // marks parts of this cache is dirty (with reduced granularity)
        std::bitset<MAPSIZE *MAPSIZE> dirty;
        bool dirty_any = true;

        // converts 2d coords into 1d dirty bitset coords
        static int dirty_idx( const point &p );
        void rebuild( Q quad, const Params &... params );
        void rebuild( const Params &... params );
        reachability_cache_layer &operator[]( const Q &quad );
        const reachability_cache_layer &operator[]( const Q &quad ) const;

    public:
        reachability_cache() {
            dirty.set();
        }

        void invalidate();
        void invalidate( const point &p );
        int get_value( Q q, const point &p ) const;

        /**
         * For "horizontal" cache, param is a current level_cache.
         * For "vertical" cache, there are two params:
         *    first param is "current" level_cache, where transparency is checked;
         *
         *    second param is the "floor" level_cache,
         *    if second param is the same as "current" level cache, "down" direction is calculated,
         *    if second param points to the level cache above current, "upward" direction is calculated.
         */
        bool has_potential_los( const point &from, const point &to, const Params &... params );
};

class reachability_cache_horizontal: public reachability_cache<true, level_cache> {};
class reachability_cache_vertical: public reachability_cache<false, level_cache, level_cache> {};

#endif // CATA_SRC_REACHABILITY_CACHE_H

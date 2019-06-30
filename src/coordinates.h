#pragma once
#ifndef CATA_SRC_COORDINATES_H
#define CATA_SRC_COORDINATES_H

#include <cstdlib>

#include "coordinate_conversions.h"
#include "enums.h"
#include "game_constants.h"
#include "point.h"
#include "debug.h"

namespace coords
{

enum class scale {
    map_square,
    submap,
    overmap_terrain,
    segment,
    overmap,
    vehicle
};

constexpr scale ms = scale::map_square;
constexpr scale sm = scale::submap;
constexpr scale omt = scale::overmap_terrain;
constexpr scale seg = scale::segment;
constexpr scale om = scale::overmap;

constexpr int map_squares_per( scale s )
{
    static_assert( SEEX == SEEY, "we assume submaps are square" );
    static_assert( OMAPX == OMAPY, "we assume overmaps are square" );

    switch( s ) {
        case scale::map_square:
            return 1;
        case scale::submap:
            return SEEX;
        case scale::overmap_terrain:
            return SEEX * 2;
        case scale::segment:
            return SEG_SIZE * map_squares_per( scale::overmap_terrain );
        case scale::overmap:
            return OMAPX * map_squares_per( scale::overmap_terrain );
        default:
            debugmsg( "Requested scale of %d", s );
            abort();
    }
}

enum class origin {
    relative, // this is a special origin that can be added to any other
    abs, // the global absolute origin for the entire game
    submap, // from corner of submap
    overmap_terrain, // from corner of overmap_terrain
    overmap, // from corner of overmap
};

constexpr origin origin_from_scale( scale s )
{
    switch( s ) {
        case scale::submap:
            return origin::submap;
        case scale::overmap_terrain:
            return origin::overmap_terrain;
        case scale::overmap:
            return origin::overmap;
        default:
            debugmsg( "Requested origin for of %d", s );
            abort();
    }
}

// A generic coordinate-type-safe point.
// Point should be the underlying representation type (either point or
// tripoint).
// Scale and Origin define the coordinate system for the point.
template<typename Point, scale Scale, origin Origin>
class coord_point
{
    public:
        coord_point() = default;
        explicit coord_point( const Point &p ) :
            raw_( p )
        {}

        Point &raw() {
            return raw_;
        }
        const Point &raw() const {
            return raw_;
        }
    private:
        Point raw_;
};

template<int ScaleUp, int ScaleDown, scale ResultScale>
struct project_to_impl;

template<int ScaleUp, scale ResultScale>
struct project_to_impl<ScaleUp, 0, ResultScale> {
    template<typename Point, origin Origin, scale SourceScale>
    coord_point<Point, ResultScale, Origin> operator()(
        const coord_point<Point, SourceScale, Origin> &src ) {
        return coord_point<Point, ResultScale, Origin>( multiply_xy( src.raw(), ScaleUp ) );
    }
};

template<int ScaleDown, scale ResultScale>
struct project_to_impl<0, ScaleDown, ResultScale> {
    template<typename Point, origin Origin, scale SourceScale>
    coord_point<Point, ResultScale, Origin> operator()(
        const coord_point<Point, SourceScale, Origin> &src ) {
        return coord_point<Point, ResultScale, Origin>(
                   divide_xy_round_to_minus_infinity( src.raw(), ScaleDown ) );
    }
};

template<scale ResultScale, typename Point, origin Origin, scale SourceScale>
inline coord_point<Point, ResultScale, Origin> project_to(
    const coord_point<Point, SourceScale, Origin> &src )
{
    constexpr int scale_down = map_squares_per( ResultScale ) / map_squares_per( SourceScale );
    constexpr int scale_up = map_squares_per( SourceScale ) / map_squares_per( ResultScale );
    return project_to_impl<scale_up, scale_down, ResultScale>()( src );
}

template<typename Point, scale CoarseScale, scale FineScale, origin Origin>
struct quotient_remainder {
    constexpr static origin RemainderOrigin = origin_from_scale( CoarseScale );
    using quotient_type = coord_point<Point, CoarseScale, Origin>;
    quotient_type quotient;
    using remainder_type = coord_point<Point, FineScale, RemainderOrigin>;
    remainder_type remainder;

    // For assigning to std::tie( q, r );
    operator std::tuple<quotient_type &, remainder_type &>() {
        return std::tie( quotient, remainder );
    }
};

template<scale ResultScale, typename Point, origin Origin, scale SourceScale>
inline quotient_remainder<Point, ResultScale, SourceScale, Origin> project_remain(
    const coord_point<Point, SourceScale, Origin> &src )
{
    constexpr int ScaleDown = map_squares_per( ResultScale ) / map_squares_per( SourceScale );
    static_assert( ScaleDown > 0, "You can only project to coarser coordinate systems" );
    constexpr static origin RemainderOrigin = origin_from_scale( ResultScale );
    coord_point<Point, ResultScale, Origin> quotient(
        divide_xy_round_to_minus_infinity( src.raw(), ScaleDown ) );
    coord_point<Point, SourceScale, RemainderOrigin> remainder(
        src.raw() - quotient.raw() * ScaleDown );

    return { quotient, remainder };
}

} // namespace coords

using point_ms_abs = coords::coord_point<point, coords::ms, coords::origin::abs>;
using point_ms_sm = coords::coord_point<point, coords::ms, coords::origin::submap>;
using point_ms_omt = coords::coord_point<point, coords::ms, coords::origin::overmap_terrain>;
using point_sm_abs = coords::coord_point<point, coords::sm, coords::origin::abs>;
using point_sm_omt = coords::coord_point<point, coords::sm, coords::origin::overmap_terrain>;
using point_sm_om = coords::coord_point<point, coords::sm, coords::origin::overmap>;
using point_omt_abs = coords::coord_point<point, coords::omt, coords::origin::abs>;
using point_omt_om = coords::coord_point<point, coords::omt, coords::origin::overmap>;
using point_seg_abs = coords::coord_point<point, coords::seg, coords::origin::abs>;
using point_om_abs = coords::coord_point<point, coords::om, coords::origin::abs>;

using tripoint_ms_abs = coords::coord_point<tripoint, coords::ms, coords::origin::abs>;
using tripoint_ms_sm = coords::coord_point<tripoint, coords::ms, coords::origin::submap>;
using tripoint_ms_omt = coords::coord_point<tripoint, coords::ms, coords::origin::overmap_terrain>;
using tripoint_sm_abs = coords::coord_point<tripoint, coords::sm, coords::origin::abs>;
using tripoint_omt_abs = coords::coord_point<tripoint, coords::omt, coords::origin::abs>;
using tripoint_seg_abs = coords::coord_point<tripoint, coords::seg, coords::origin::abs>;
using tripoint_om_abs = coords::coord_point<tripoint, coords::om, coords::origin::abs>;

using coords::project_to;
using coords::project_remain;

/* find appropriate subdivided coordinates for absolute tile coordinate.
 * This is less obvious than one might think, for negative coordinates, so this
 * was created to give a definitive answer.
 *
 * 'absolute' is defined as the -actual- submap x,y * SEEX + position in submap, and
 * can be obtained from map.getabs(x, y);
 *   usage:
 *    real_coords rc( g->m.getabs(g->u.posx(), g->u.posy() ) );
 */
struct real_coords {
    static const int tiles_in_sub = SEEX;
    static const int tiles_in_sub_n = tiles_in_sub - 1;
    static const int subs_in_om = OMAPX * 2;
    static const int subs_in_om_n = subs_in_om - 1;

    point abs_pos;     // 1 per tile, starting from tile 0,0 of submap 0,0 of overmap 0,0
    point abs_sub;     // submap: 12 tiles.
    point abs_om;      // overmap: 360 submaps.

    point sub_pos;     // coordinate (0-11) in submap / abs_pos constrained to % 12.

    point om_pos;      // overmap tile: 2x2 submaps.
    point om_sub;      // submap (0-359) in overmap / abs_sub constrained to % 360. equivalent to g->levx

    real_coords() = default;

    real_coords( const point &ap ) {
        fromabs( ap );
    }

    void fromabs( const point &abs ) {
        const point norm( std::abs( abs.x ), std::abs( abs.y ) );
        abs_pos = abs;

        if( abs.x < 0 ) {
            abs_sub.x = ( abs.x - SEEX + 1 ) / SEEX;
            sub_pos.x = SEEX - 1 - ( ( norm.x - 1 ) % SEEX );
            abs_om.x = ( abs_sub.x - subs_in_om_n ) / subs_in_om;
            om_sub.x = subs_in_om_n - ( ( ( norm.x - 1 ) / SEEX ) % subs_in_om );
        } else {
            abs_sub.x = norm.x / SEEX;
            sub_pos.x = abs.x % SEEX;
            abs_om.x = abs_sub.x / subs_in_om;
            om_sub.x = abs_sub.x % subs_in_om;
        }
        om_pos.x = om_sub.x / 2;

        if( abs.y < 0 ) {
            abs_sub.y = ( abs.y - SEEY + 1 ) / SEEY;
            sub_pos.y = SEEY - 1 - ( ( norm.y - 1 ) % SEEY );
            abs_om.y = ( abs_sub.y - subs_in_om_n ) / subs_in_om;
            om_sub.y = subs_in_om_n - ( ( ( norm.y - 1 ) / SEEY ) % subs_in_om );
        } else {
            abs_sub.y = norm.y / SEEY;
            sub_pos.y = abs.y % SEEY;
            abs_om.y = abs_sub.y / subs_in_om;
            om_sub.y = abs_sub.y % subs_in_om;
        }
        om_pos.y = om_sub.y / 2;
    }

    // specifically for the subjective position returned by overmap::draw
    void fromomap( const point &rel_om, const point &rel_om_pos ) {
        const point a = om_to_omt_copy( rel_om ) + rel_om_pos;
        fromabs( omt_to_ms_copy( a ) );
    }

    // helper functions to return abs_pos of submap/overmap tile/overmap's start

    point begin_sub() {
        return point( abs_sub.x * tiles_in_sub, abs_sub.y * tiles_in_sub );
    }
    point begin_om_pos() {
        return point( ( abs_om.x * subs_in_om * tiles_in_sub ) + ( om_pos.x * 2 * tiles_in_sub ),
                      ( abs_om.y * subs_in_om * tiles_in_sub ) + ( om_pos.y * 2 * tiles_in_sub ) );
    }
    point begin_om() {
        return point( abs_om.x * subs_in_om * tiles_in_sub, abs_om.y * subs_in_om * tiles_in_sub );
    }
};
#endif // CATA_SRC_COORDINATES_H

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
            constexpr_fatal( 0, "Requested scale of %d", s );
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
            constexpr_fatal( origin::abs, "Requested origin for scale %d", s );
    }
}

// A generic coordinate-type-safe point.
// Point should be the underlying representation type (either point or
// tripoint).
// Scale and Origin define the coordinate system for the point.
template<typename Point, origin Origin, scale Scale>
class coord_point
{
    public:
        static constexpr int dimension = Point::dimension;

        constexpr coord_point() = default;
        explicit constexpr coord_point( const Point &p ) :
            raw_( p )
        {}
        template<typename T>
        constexpr coord_point( T x, T y ) : raw_( x, y ) {}
        template<typename T>
        constexpr coord_point( T x, T y, T z ) : raw_( x, y, z ) {}
        template<typename T>
        constexpr coord_point( const coord_point<point, Origin, Scale> &xy, T z ) :
            raw_( xy.raw(), z )
        {}

        constexpr Point &raw() {
            return raw_;
        }
        constexpr const Point &raw() const {
            return raw_;
        }

        constexpr auto &x() {
            return raw_.x;
        }
        constexpr auto x() const {
            return raw_.x;
        }
        constexpr auto &y() {
            return raw_.y;
        }
        constexpr auto y() const {
            return raw_.y;
        }
        constexpr auto xy() const {
            return coord_point<point, Origin, Scale>( raw_.xy() );
        }
        constexpr auto &z() {
            return raw_.z;
        }
        constexpr auto z() const {
            return raw_.z;
        }

        std::string to_string() const {
            return raw_.to_string();
        }

        void serialize( JsonOut &jsout ) const {
            raw().serialize( jsout );
        }
        void deserialize( JsonIn &jsin ) {
            raw().deserialize( jsin );
        }

        coord_point &operator+=( const coord_point<Point, origin::relative, Scale> &r ) {
            raw_ += r.raw();
            return *this;
        }

        coord_point &operator-=( const coord_point<Point, origin::relative, Scale> &r ) {
            raw_ -= r.raw();
            return *this;
        }

        coord_point &operator+=( const point &r ) {
            raw_ += r;
            return *this;
        }

        coord_point &operator-=( const point &r ) {
            raw_ -= r;
            return *this;
        }

        coord_point &operator+=( const tripoint &r ) {
            raw_ += r;
            return *this;
        }

        coord_point &operator-=( const tripoint &r ) {
            raw_ -= r;
            return *this;
        }

        friend inline coord_point operator+( const coord_point &l, const point &r ) {
            return coord_point( l.raw() + r );
        }

        friend inline coord_point operator+( const coord_point &l, const tripoint &r ) {
            return coord_point( l.raw() + r );
        }

        friend inline coord_point operator-( const coord_point &l, const point &r ) {
            return coord_point( l.raw() - r );
        }

        friend inline coord_point operator-( const coord_point &l, const tripoint &r ) {
            return coord_point( l.raw() - r );
        }
    private:
        Point raw_;
};

template<typename Point, origin Origin, scale Scale>
constexpr inline bool operator==( const coord_point<Point, Origin, Scale> &l,
                                  const coord_point<Point, Origin, Scale> &r )
{
    return l.raw() == r.raw();
}

template<typename Point, origin Origin, scale Scale>
constexpr inline bool operator!=( const coord_point<Point, Origin, Scale> &l,
                                  const coord_point<Point, Origin, Scale> &r )
{
    return l.raw() != r.raw();
}

template<typename Point, origin Origin, scale Scale>
constexpr inline bool operator<( const coord_point<Point, Origin, Scale> &l,
                                 const coord_point<Point, Origin, Scale> &r )
{
    return l.raw() < r.raw();
}

template<typename PointL, typename PointR, origin OriginL, scale Scale>
constexpr inline auto operator+(
    const coord_point<PointL, OriginL, Scale> &l,
    const coord_point<PointR, origin::relative, Scale> &r )
{
    using PointResult = decltype( PointL() + PointR() );
    return coord_point<PointResult, OriginL, Scale>( l.raw() + r.raw() );
}

template < typename PointL, typename PointR, origin OriginR, scale Scale,
           // enable_if to prevent ambiguity with above when both args are
           // relative
           typename = std::enable_if_t < OriginR != origin::relative >>
constexpr inline auto operator+(
    const coord_point<PointL, origin::relative, Scale> &l,
    const coord_point<PointR, OriginR, Scale> &r )
{
    using PointResult = decltype( PointL() + PointR() );
    return coord_point<PointResult, OriginR, Scale>( l.raw() + r.raw() );
}

template<typename PointL, typename PointR, origin OriginL, scale Scale>
constexpr inline auto operator-(
    const coord_point<PointL, OriginL, Scale> &l,
    const coord_point<PointR, origin::relative, Scale> &r )
{
    using PointResult = decltype( PointL() + PointR() );
    return coord_point<PointResult, OriginL, Scale>( l.raw() - r.raw() );
}

template < typename PointL, typename PointR, origin Origin, scale Scale,
           // enable_if to prevent ambiguity with above when both args are
           // relative
           typename = std::enable_if_t < Origin != origin::relative >>
constexpr inline auto operator-(
    const coord_point<PointL, Origin, Scale> &l,
    const coord_point<PointR, Origin, Scale> &r )
{
    using PointResult = decltype( PointL() + PointR() );
    return coord_point<PointResult, origin::relative, Scale>( l.raw() - r.raw() );
}

// Only relative points can be multiplied by a constant
template<typename Point, scale Scale>
constexpr inline coord_point<Point, origin::relative, Scale> operator*(
    int l, const coord_point<Point, origin::relative, Scale> &r )
{
    return coord_point<Point, origin::relative, Scale>( l * r.raw() );
}

template<typename Point, scale Scale>
constexpr inline coord_point<Point, origin::relative, Scale> operator*(
    const coord_point<Point, origin::relative, Scale> &r, int l )
{
    return coord_point<Point, origin::relative, Scale>( r.raw() * l );
}

template<typename Point, origin Origin, scale Scale>
inline std::ostream &operator<<( std::ostream &os, const coord_point<Point, Origin, Scale> &p )
{
    return os << p.raw();
}

template<int ScaleUp, int ScaleDown, scale ResultScale>
struct project_to_impl;

template<int ScaleUp, scale ResultScale>
struct project_to_impl<ScaleUp, 0, ResultScale> {
    template<typename Point, origin Origin, scale SourceScale>
    coord_point<Point, Origin, ResultScale> operator()(
        const coord_point<Point, Origin, SourceScale> &src ) {
        return coord_point<Point, Origin, ResultScale>( multiply_xy( src.raw(), ScaleUp ) );
    }
};

template<int ScaleDown, scale ResultScale>
struct project_to_impl<0, ScaleDown, ResultScale> {
    template<typename Point, origin Origin, scale SourceScale>
    coord_point<Point, Origin, ResultScale> operator()(
        const coord_point<Point, Origin, SourceScale> &src ) {
        return coord_point<Point, Origin, ResultScale>(
                   divide_xy_round_to_minus_infinity( src.raw(), ScaleDown ) );
    }
};

template<scale ResultScale, typename Point, origin Origin, scale SourceScale>
inline coord_point<Point, Origin, ResultScale> project_to(
    const coord_point<Point, Origin, SourceScale> &src )
{
    constexpr int scale_down = map_squares_per( ResultScale ) / map_squares_per( SourceScale );
    constexpr int scale_up = map_squares_per( SourceScale ) / map_squares_per( ResultScale );
    return project_to_impl<scale_up, scale_down, ResultScale>()( src );
}

template<typename Point, origin Origin, scale CoarseScale, scale FineScale>
struct quotient_remainder {
    constexpr static origin RemainderOrigin = origin_from_scale( CoarseScale );
    using quotient_type = coord_point<Point, Origin, CoarseScale>;
    quotient_type quotient;
    using remainder_type = coord_point<Point, RemainderOrigin, FineScale>;
    remainder_type remainder;

    // For assigning to std::tie( q, r );
    operator std::tuple<quotient_type &, remainder_type &>() {
        return std::tie( quotient, remainder );
    }
};

template<scale ResultScale, typename Point, origin Origin, scale SourceScale>
inline quotient_remainder<Point, Origin, ResultScale, SourceScale> project_remain(
    const coord_point<Point, Origin, SourceScale> &src )
{
    constexpr int ScaleDown = map_squares_per( ResultScale ) / map_squares_per( SourceScale );
    static_assert( ScaleDown > 0, "You can only project to coarser coordinate systems" );
    constexpr static origin RemainderOrigin = origin_from_scale( ResultScale );
    coord_point<Point, Origin, ResultScale> quotient(
        divide_xy_round_to_minus_infinity( src.raw(), ScaleDown ) );
    coord_point<Point, RemainderOrigin, SourceScale> remainder(
        src.raw() - quotient.raw() * ScaleDown );

    return { quotient, remainder };
}

} // namespace coords

namespace std
{

template<typename Point, coords::origin Origin, coords::scale Scale>
struct hash<coords::coord_point<Point, Origin, Scale>> {
    std::size_t operator()( const coords::coord_point<Point, Origin, Scale> &p ) const {
        const hash<Point> h{};
        return h( p.raw() );
    }
};

} // namespace std

/** Typedefs for point types with coordinate mnemonics.
 *
 * Each name is of the form (tri)point_<origin>_<scale> where <origin> tells you the
 * context in which the point has meaning, and <scale> tells you what one unit
 * of the point means.
 *
 * For example:
 * point_omt_ms is the position of a map square within an overmap terrain.
 * tripoint_rel_sm is a relative tripoint submap offset.
 */
/*@{*/
using point_rel_ms = coords::coord_point<point, coords::origin::relative, coords::ms>;
using point_abs_ms = coords::coord_point<point, coords::origin::abs, coords::ms>;
using point_sm_ms = coords::coord_point<point, coords::origin::submap, coords::ms>;
using point_omt_ms = coords::coord_point<point, coords::origin::overmap_terrain, coords::ms>;
using point_abs_sm = coords::coord_point<point, coords::origin::abs, coords::sm>;
using point_omt_sm = coords::coord_point<point, coords::origin::overmap_terrain, coords::sm>;
using point_om_sm = coords::coord_point<point, coords::origin::overmap, coords::sm>;
using point_abs_omt = coords::coord_point<point, coords::origin::abs, coords::omt>;
using point_om_omt = coords::coord_point<point, coords::origin::overmap, coords::omt>;
using point_abs_seg = coords::coord_point<point, coords::origin::abs, coords::seg>;
using point_abs_om = coords::coord_point<point, coords::origin::abs, coords::om>;

using tripoint_rel_ms = coords::coord_point<tripoint, coords::origin::relative, coords::ms>;
using tripoint_abs_ms = coords::coord_point<tripoint, coords::origin::abs, coords::ms>;
using tripoint_sm_ms = coords::coord_point<tripoint, coords::origin::submap, coords::ms>;
using tripoint_omt_ms = coords::coord_point<tripoint, coords::origin::overmap_terrain, coords::ms>;
using tripoint_abs_sm = coords::coord_point<tripoint, coords::origin::abs, coords::sm>;
using tripoint_abs_omt = coords::coord_point<tripoint, coords::origin::abs, coords::omt>;
using tripoint_abs_seg = coords::coord_point<tripoint, coords::origin::abs, coords::seg>;
using tripoint_abs_om = coords::coord_point<tripoint, coords::origin::abs, coords::om>;
/*@}*/

using coords::project_to;
using coords::project_remain;

template<typename Point, coords::origin Origin, coords::scale Scale>
std::vector<coords::coord_point<Point, Origin, Scale>>
        closest_points_first( const coords::coord_point<Point, Origin, Scale> &loc,
                              int min_dist, int max_dist )
{
    std::vector<Point> raw_result = closest_points_first( loc.raw(), min_dist, max_dist );
    std::vector<coords::coord_point<Point, Origin, Scale>> result;
    result.reserve( raw_result.size() );
    std::transform( raw_result.begin(), raw_result.end(), std::back_inserter( result ),
    []( const Point & p ) {
        return coords::coord_point<Point, Origin, Scale>( p );
    } );
    return result;
}
template<typename Point, coords::origin Origin, coords::scale Scale>
std::vector<coords::coord_point<Point, Origin, Scale>>
        closest_points_first( const coords::coord_point<Point, Origin, Scale> &loc,
                              int max_dist )
{
    return closest_points_first( loc, 0, max_dist );
}

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

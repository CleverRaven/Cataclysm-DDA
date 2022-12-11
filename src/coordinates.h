#pragma once
#ifndef CATA_SRC_COORDINATES_H
#define CATA_SRC_COORDINATES_H

#include <algorithm>
#include <cstdlib>
#include <iterator>

#include "coordinate_conversions.h"
#include "cuboid_rectangle.h"
#include "enums.h"
#include "game_constants.h"
#include "line.h"
#include "point.h"
#include "debug.h"

class JsonValue;

enum class direction : unsigned;

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
    reality_bubble, // from corner of a reality bubble (aka 'map' or 'tinymap')
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

constexpr scale scale_from_origin( origin o )
{
    switch( o ) {
        case origin::submap:
            return scale::submap;
        case origin::overmap_terrain:
            return scale::overmap_terrain;
        case origin::overmap:
            return scale::overmap;
        default:
            constexpr_fatal( ms, "Requested scale for origin %d", o );
    }
}

/**
 * A generic coordinate-type-safe point.
 *
 * Generally you wouldn't use this class directly, but via the typedefs below
 * (point_abs_ms, etc.).
 *
 * Point should be the underlying representation type (either point or
 * tripoint).
 *
 * Origin and Scale define the coordinate system for the point.
 *
 * For more details see doc/POINTS_COORDINATES.md.
 */
template<typename Point, origin Origin, scale Scale>
class coord_point
{
    public:
        static constexpr int dimension = Point::dimension;
        using this_as_tripoint = coord_point<tripoint, Origin, Scale>;

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
        std::string to_string_writable() const {
            return raw_.to_string_writable();
        }

        void serialize( JsonOut &jsout ) const {
            raw().serialize( jsout );
        }
        void deserialize( const JsonValue &jv ) {
            raw().deserialize( jv );
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

        friend inline this_as_tripoint operator+( const coord_point &l, const tripoint &r ) {
            return this_as_tripoint( l.raw() + r );
        }

        friend inline coord_point operator+( const point &l, const coord_point &r ) {
            return coord_point( l + r.raw() );
        }

        friend inline this_as_tripoint operator+( const tripoint &l, const coord_point &r ) {
            return this_as_tripoint( l + r.raw() );
        }

        friend inline coord_point operator-( const coord_point &l, const point &r ) {
            return coord_point( l.raw() - r );
        }

        friend inline this_as_tripoint operator-( const coord_point &l, const tripoint &r ) {
            return this_as_tripoint( l.raw() - r );
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

template <typename Point, origin Origin, scale Scale>
constexpr inline coord_point<Point, Origin, Scale>
coord_min( const coord_point<Point, Origin, Scale> &l, const coord_point<Point, Origin, Scale> &r )
{
    return { std::min( l.x(), r.x() ), std::min( l.y(), r.y() ), std::min( l.z(), r.z() ) };
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

template<origin Origin, scale CoarseScale, scale FineScale>
struct quotient_remainder_helper {
    constexpr static origin RemainderOrigin = origin_from_scale( CoarseScale );
    using quotient_type = coord_point<point, Origin, CoarseScale>;
    using quotient_type_tripoint = coord_point<tripoint, Origin, CoarseScale>;
    using remainder_type = coord_point<point, RemainderOrigin, FineScale>;
    using remainder_type_tripoint = coord_point<tripoint, RemainderOrigin, FineScale>;
};

template<origin Origin, scale CoarseScale, scale FineScale>
struct quotient_remainder_point {
    using helper = quotient_remainder_helper<Origin, CoarseScale, FineScale>;
    using quotient_type = typename helper::quotient_type;
    using remainder_type = typename helper::remainder_type;

    quotient_type quotient;
    remainder_type remainder;

    // For assigning to std::tie( q, r );
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator std::tuple<quotient_type &, remainder_type &>() {
        return std::tie( quotient, remainder );
    }
};

template<origin Origin, scale CoarseScale, scale FineScale>
struct quotient_remainder_tripoint {
    using helper = quotient_remainder_helper<Origin, CoarseScale, FineScale>;
    using quotient_type = typename helper::quotient_type;
    using quotient_type_tripoint = typename helper::quotient_type_tripoint;
    using remainder_type = typename helper::remainder_type;
    using remainder_type_tripoint = typename helper::remainder_type_tripoint;

    // Annoyingly, for the conversion operators below to work correctly, we
    // need to have point and tripoint version of both quotient and remainder
    // ready here, so that we can take references to any of them.
    // Luckily the entire lifetime of this struct should be pretty short, so
    // the compiler can do its magic to remove the overhead of initializing the
    // ones that don't actually get used.
    quotient_type quotient;
    remainder_type remainder;
    quotient_type_tripoint quotient_tripoint;
    remainder_type_tripoint remainder_tripoint;

    // For assigning to std::tie( q, r );
    // Exactly one of the two resulting types should be a tripoint, so that the
    // z-coordinate doesn't get duplicated.
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator std::tuple<quotient_type_tripoint &, remainder_type &>() {
        return std::tie( quotient_tripoint, remainder );
    }
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator std::tuple<quotient_type &, remainder_type_tripoint &>() {
        return std::tie( quotient, remainder_tripoint );
    }
};

// project_remain returns a helper struct, intended to be used with std::tie
// to pull out the two components of the result.
// For example, when splitting a point:
//  point_abs_sm val;
//  point_abs_om quotient;
//  point_om_sm remainder;
//  std::tie( quotient, remainder ) = project_remain<coords::om>( val );
// If passing a tripoint to project_remain, you must choose exactly one of the
// quotient or remainder to get the z coordinate.  Both of these should work:
//  tripoint_abs_sm val;
//  tripoint_abs_om quotient;
//  point_om_sm remainder;
//  std::tie( quotient, remainder ) = project_remain<coords::om>( val );
//
//  point_abs_om quotient;
//  tripoint_om_sm remainder;
//  std::tie( quotient, remainder ) = project_remain<coords::om>( val );
template<scale ResultScale, origin Origin, scale SourceScale>
inline quotient_remainder_point<Origin, ResultScale, SourceScale> project_remain(
    const coord_point<point, Origin, SourceScale> &src )
{
    constexpr int ScaleDown = map_squares_per( ResultScale ) / map_squares_per( SourceScale );
    static_assert( ScaleDown > 0, "You can only project to coarser coordinate systems" );
    constexpr static origin RemainderOrigin = origin_from_scale( ResultScale );
    coord_point<point, Origin, ResultScale> quotient(
        divide_xy_round_to_minus_infinity( src.raw(), ScaleDown ) );
    coord_point<point, RemainderOrigin, SourceScale> remainder(
        src.raw() - quotient.raw() * ScaleDown );

    return { quotient, remainder };
}

template<scale ResultScale, origin Origin, scale SourceScale>
inline quotient_remainder_tripoint<Origin, ResultScale, SourceScale> project_remain(
    const coord_point<tripoint, Origin, SourceScale> &src )
{
    quotient_remainder_point<Origin, ResultScale, SourceScale> point_result =
        project_remain<ResultScale>( src.xy() );
    return { point_result.quotient, point_result.remainder,
        { point_result.quotient, src.z() }, { point_result.remainder, src.z() } };
}

template<typename PointL, typename PointR, origin CoarseOrigin, scale CoarseScale,
         origin FineOrigin, scale FineScale>
inline auto project_combine(
    const coord_point<PointL, CoarseOrigin, CoarseScale> &coarse,
    const coord_point<PointR, FineOrigin, FineScale> &fine )
{
    static_assert( origin_from_scale( CoarseScale ) == FineOrigin,
                   "given point types are not compatible for combination" );
    static_assert( PointL::dimension != 3 || PointR::dimension != 3,
                   "two tripoints should not be combined; it's unclear how to handle z" );
    using PointResult = decltype( PointL() + PointR() );
    const coord_point<PointL, CoarseOrigin, FineScale> refined_coarse =
        project_to<FineScale>( coarse );
    return coord_point<PointResult, CoarseOrigin, FineScale>( refined_coarse.raw() + fine.raw() );
}

template<scale FineScale, origin Origin, scale CoarseScale>
inline auto project_bounds( const coord_point<point, Origin, CoarseScale> &coarse )
{
    constexpr point one( 1, 1 ); // NOLINT(cata-use-named-point-constants)
    return half_open_rectangle<coord_point<point, Origin, FineScale>>(
               project_to<FineScale>( coarse ), project_to<FineScale>( coarse + one ) );
}

template<scale FineScale, origin Origin, scale CoarseScale>
inline auto project_bounds( const coord_point<tripoint, Origin, CoarseScale> &coarse )
{
    constexpr point one( 1, 1 ); // NOLINT(cata-use-named-point-constants)
    return half_open_cuboid<coord_point<tripoint, Origin, FineScale>>(
               project_to<FineScale>( coarse ), project_to<FineScale>( coarse + one ) );
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
 *
 * For more details see doc/POINTS_COORDINATES.md.
 */
/*@{*/
using point_rel_ms = coords::coord_point<point, coords::origin::relative, coords::ms>;
using point_abs_ms = coords::coord_point<point, coords::origin::abs, coords::ms>;
using point_sm_ms = coords::coord_point<point, coords::origin::submap, coords::ms>;
using point_omt_ms = coords::coord_point<point, coords::origin::overmap_terrain, coords::ms>;
using point_bub_ms = coords::coord_point<point, coords::origin::reality_bubble, coords::ms>;
using point_rel_sm = coords::coord_point<point, coords::origin::relative, coords::sm>;
using point_abs_sm = coords::coord_point<point, coords::origin::abs, coords::sm>;
using point_omt_sm = coords::coord_point<point, coords::origin::overmap_terrain, coords::sm>;
using point_om_sm = coords::coord_point<point, coords::origin::overmap, coords::sm>;
using point_bub_sm = coords::coord_point<point, coords::origin::reality_bubble, coords::sm>;
using point_rel_omt = coords::coord_point<point, coords::origin::relative, coords::omt>;
using point_abs_omt = coords::coord_point<point, coords::origin::abs, coords::omt>;
using point_om_omt = coords::coord_point<point, coords::origin::overmap, coords::omt>;
using point_abs_seg = coords::coord_point<point, coords::origin::abs, coords::seg>;
using point_rel_om = coords::coord_point<point, coords::origin::relative, coords::om>;
using point_abs_om = coords::coord_point<point, coords::origin::abs, coords::om>;

using tripoint_rel_ms = coords::coord_point<tripoint, coords::origin::relative, coords::ms>;
using tripoint_abs_ms = coords::coord_point<tripoint, coords::origin::abs, coords::ms>;
using tripoint_sm_ms = coords::coord_point<tripoint, coords::origin::submap, coords::ms>;
using tripoint_omt_ms = coords::coord_point<tripoint, coords::origin::overmap_terrain, coords::ms>;
using tripoint_bub_ms = coords::coord_point<tripoint, coords::origin::reality_bubble, coords::ms>;
using tripoint_rel_sm = coords::coord_point<tripoint, coords::origin::relative, coords::sm>;
using tripoint_abs_sm = coords::coord_point<tripoint, coords::origin::abs, coords::sm>;
using tripoint_om_sm = coords::coord_point<tripoint, coords::origin::overmap, coords::sm>;
using tripoint_bub_sm = coords::coord_point<tripoint, coords::origin::reality_bubble, coords::sm>;
using tripoint_rel_omt = coords::coord_point<tripoint, coords::origin::relative, coords::omt>;
using tripoint_abs_omt = coords::coord_point<tripoint, coords::origin::abs, coords::omt>;
using tripoint_om_omt = coords::coord_point<tripoint, coords::origin::overmap, coords::omt>;
using tripoint_abs_seg = coords::coord_point<tripoint, coords::origin::abs, coords::seg>;
using tripoint_abs_om = coords::coord_point<tripoint, coords::origin::abs, coords::om>;
/*@}*/

using coords::project_to;
using coords::project_remain;
using coords::project_combine;
using coords::project_bounds;

template<typename Point, coords::origin Origin, coords::scale Scale>
inline int square_dist( const coords::coord_point<Point, Origin, Scale> &loc1,
                        const coords::coord_point<Point, Origin, Scale> &loc2 )
{
    return square_dist( loc1.raw(), loc2.raw() );
}

template<typename Point, coords::origin Origin, coords::scale Scale>
inline int trig_dist( const coords::coord_point<Point, Origin, Scale> &loc1,
                      const coords::coord_point<Point, Origin, Scale> &loc2 )
{
    return trig_dist( loc1.raw(), loc2.raw() );
}

template<typename Point, coords::origin Origin, coords::scale Scale>
inline int rl_dist( const coords::coord_point<Point, Origin, Scale> &loc1,
                    const coords::coord_point<Point, Origin, Scale> &loc2 )
{
    return rl_dist( loc1.raw(), loc2.raw() );
}

template<typename Point, coords::origin Origin, coords::scale Scale>
inline int manhattan_dist( const coords::coord_point<Point, Origin, Scale> &loc1,
                           const coords::coord_point<Point, Origin, Scale> &loc2 )
{
    return manhattan_dist( loc1.raw(), loc2.raw() );
}

template<typename Point, coords::origin Origin, coords::scale Scale>
inline int octile_dist( const coords::coord_point<Point, Origin, Scale> &loc1,
                        const coords::coord_point<Point, Origin, Scale> &loc2, int multiplier = 1 )
{
    return octile_dist( loc1.raw(), loc2.raw(), multiplier );
}

template<typename Point, coords::origin Origin, coords::scale Scale>
direction direction_from( const coords::coord_point<Point, Origin, Scale> &loc1,
                          const coords::coord_point<Point, Origin, Scale> &loc2 )
{
    return direction_from( loc1.raw(), loc2.raw() );
}

template<typename Point, coords::origin Origin, coords::scale Scale>
std::vector<coords::coord_point<Point, Origin, Scale>>
        line_to( const coords::coord_point<Point, Origin, Scale> &loc1,
                 const coords::coord_point<Point, Origin, Scale> &loc2 )
{
    std::vector<Point> raw_result = line_to( loc1.raw(), loc2.raw() );
    std::vector<coords::coord_point<Point, Origin, Scale>> result;
    std::transform( raw_result.begin(), raw_result.end(), std::back_inserter( result ),
    []( const Point & p ) {
        return coords::coord_point<Point, Origin, Scale>( p );
    } );
    return result;
}

template<typename Point, coords::origin Origin, coords::scale Scale>
coords::coord_point<Point, Origin, Scale>
midpoint( const coords::coord_point<Point, Origin, Scale> &loc1,
          const coords::coord_point<Point, Origin, Scale> &loc2 )
{
    return coords::coord_point<Point, Origin, Scale>( ( loc1.raw() + loc2.raw() ) / 2 );
}

template<typename Point>
Point midpoint( const inclusive_rectangle<Point> &box )
{
    constexpr point one( 1, 1 ); // NOLINT(cata-use-named-point-constants)
    return midpoint( box.p_min, box.p_max + one );
}

template<typename Point>
Point midpoint( const half_open_rectangle<Point> &box )
{
    return midpoint( box.p_min, box.p_max );
}

template<typename Tripoint>
Tripoint midpoint( const inclusive_cuboid<Tripoint> &box )
{
    constexpr tripoint one( 1, 1, 1 );
    return midpoint( box.p_min, box.p_max + one );
}

template<typename Tripoint>
Tripoint midpoint( const half_open_cuboid<Tripoint> &box )
{
    return midpoint( box.p_min, box.p_max );
}

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

    explicit real_coords( const point &ap ) {
        fromabs( ap );
    }

    void fromabs( const point &abs );

    // specifically for the subjective position returned by overmap::draw
    void fromomap( const point &rel_om, const point &rel_om_pos ) {
        const point a = om_to_omt_copy( rel_om ) + rel_om_pos;
        fromabs( omt_to_ms_copy( a ) );
    }

    point_abs_omt abs_omt() const {
        return project_to<coords::omt>( point_abs_sm( abs_sub ) );
    }

    // helper functions to return abs_pos of submap/overmap tile/overmap's start

    point begin_sub() const {
        return point( abs_sub.x * tiles_in_sub, abs_sub.y * tiles_in_sub );
    }
    point begin_om_pos() const {
        return point( ( abs_om.x * subs_in_om * tiles_in_sub ) + ( om_pos.x * 2 * tiles_in_sub ),
                      ( abs_om.y * subs_in_om * tiles_in_sub ) + ( om_pos.y * 2 * tiles_in_sub ) );
    }
    point begin_om() const {
        return point( abs_om.x * subs_in_om * tiles_in_sub, abs_om.y * subs_in_om * tiles_in_sub );
    }
};
#endif // CATA_SRC_COORDINATES_H

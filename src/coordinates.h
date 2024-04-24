#pragma once
#ifndef CATA_SRC_COORDINATES_H
#define CATA_SRC_COORDINATES_H

#include <functional>
#include <iosfwd>
#include <iterator>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "coordinate_conversions.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "game_constants.h"
#include "line.h"  // IWYU pragma: keep
#include "point.h"

class JsonOut;
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
 * InBounds define if the point is guaranteed to be inbounds.
 *
 * For more details see doc/POINTS_COORDINATES.md.
 */
template<typename Point>
class coord_point_base
{
    public:
        std::string to_string() const {
            return raw_.to_string();
        }
        std::string to_string_writable() const {
            return raw_.to_string_writable();
        }

        void serialize( JsonOut &jsout ) const {
            raw_.serialize( jsout );
        }

    protected:
        constexpr coord_point_base() = default;

        template <typename... Ts>
        explicit constexpr coord_point_base( Ts &&... ts ) : raw_( std::forward<Ts>( ts )... ) {}

        Point raw_;
};

template<typename Point, typename Subpoint, bool InBounds>
class coord_point_mut : public coord_point_base<Point>
{
        using base = coord_point_base<Point>;

    public:
        constexpr coord_point_mut() = default;
        explicit constexpr coord_point_mut( const Point &p ) : base( p ) {}
        template <typename T>
        constexpr coord_point_mut( const Subpoint &p, T z ) : base( p.raw(), z ) {}
        template<typename T>
        constexpr coord_point_mut( T x, T y ) : base( x, y ) {}
        template<typename T>
        constexpr coord_point_mut( T x, T y, T z ) : base( x, y, z ) {}

        // TODO: move the const accessors into base when cata-unsequenced-calls is fixed.
        constexpr const Point &raw() const {
            return this->raw_;
        }

        constexpr auto x() const {
            return raw().x;
        }
        constexpr auto y() const {
            return raw().y;
        }
        constexpr auto z() const {
            return raw().z;
        }

        constexpr Point &raw() {
            return this->raw_;
        }

        constexpr auto &x() {
            return raw().x;
        }
        constexpr auto &y() {
            return raw().y;
        }
        constexpr auto &z() {
            return raw().z;
        }

        void deserialize( const JsonValue &jv ) {
            raw().deserialize( jv );
        }
};

template<typename Point, typename Subpoint>
class coord_point_mut< Point, Subpoint, true> : public coord_point_base<Point>
{
        using base = coord_point_base<Point>;

    public:
        // Default constructed point is always inbounds.
        constexpr coord_point_mut() = default;

        // TODO: move the const accessors into base when cata-unsequenced-calls is fixed.
        constexpr const Point &raw() const {
            return this->raw_;
        }

        constexpr auto x() const {
            return raw().x;
        }
        constexpr auto y() const {
            return raw().y;
        }
        constexpr auto z() const {
            return raw().z;
        }

    protected:
        // Hide the constructors so they are not used by accident.
        //
        // Use make_unchecked instead.
        explicit constexpr coord_point_mut( const Point &p ) : base( p ) {}
        template <typename T>
        constexpr coord_point_mut( const Subpoint &p, T z ) : base( p.raw(), z ) {}
        template<typename T>
        constexpr coord_point_mut( T x, T y ) : base( x, y ) {}
        template<typename T>
        constexpr coord_point_mut( T x, T y, T z ) : base( x, y, z ) {}
};

template<typename Point, origin Origin, scale Scale, bool InBounds = false>
class coord_point : public
    coord_point_mut<Point, coord_point<point, Origin, Scale, InBounds>, InBounds>
{
        using base = coord_point_mut<Point, coord_point<point, Origin, Scale, InBounds>, InBounds>;
        using base::base;

    public:
        static constexpr int dimension = Point::dimension;

        using this_as_tripoint_ob = coord_point<tripoint, Origin, Scale>;
        using this_as_point = coord_point<point, Origin, Scale, InBounds>;
        using this_as_ob = coord_point<Point, Origin, Scale>;

        // Explicit functions to construct inbounds versions without doing any
        // bounds checking. Only use these with a very good reason, when you
        // are completely sure the result will be inbounds.
        template <typename T>
        static constexpr coord_point make_unchecked( T x, T y ) {
            return coord_point( x, y );
        }
        template <typename T>
        static constexpr coord_point make_unchecked( T x, T y, T z ) {
            return coord_point( x, y, z );
        }
        static constexpr coord_point make_unchecked( const this_as_ob &other ) {
            return coord_point( other.raw() );
        }
        static constexpr coord_point make_unchecked( const Point &other ) {
            return coord_point( other );
        }
        template <typename T>
        static constexpr coord_point make_unchecked( const this_as_point &other, T z ) {
            return coord_point( other, z );
        }
        template <typename T>
        static constexpr coord_point make_unchecked( const point &other, T z ) {
            return coord_point( other.x, other.y, z );
        }

        // Allow implicit conversions from inbounds to out of bounds.
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator this_as_ob() const {
            return this_as_ob( this->raw() );
        }

        constexpr auto xy() const {
            return this_as_point::make_unchecked( this->raw().xy() );
        }

        friend inline this_as_ob operator+( const coord_point &l, const point &r ) {
            return this_as_ob( l.raw() + r );
        }

        friend inline this_as_tripoint_ob operator+( const coord_point &l, const tripoint &r ) {
            return this_as_tripoint_ob( l.raw() + r );
        }

        friend inline this_as_ob operator+( const point &l, const coord_point &r ) {
            return this_as_ob( l + r.raw() );
        }

        friend inline this_as_tripoint_ob operator+( const tripoint &l, const coord_point &r ) {
            return this_as_tripoint_ob( l + r.raw() );
        }

        friend inline this_as_ob operator-( const coord_point &l, const point &r ) {
            return this_as_ob( l.raw() - r );
        }

        friend inline this_as_tripoint_ob operator-( const coord_point &l, const tripoint &r ) {
            return this_as_tripoint_ob( l.raw() - r );
        }
};

template<typename Point, origin Origin, scale Scale>
constexpr inline coord_point<Point, Origin, Scale> &operator+=( coord_point<Point, Origin, Scale>
        &me, const coord_point<Point, origin::relative, Scale> &r )
{
    me.raw() += r.raw();
    return me;
}

template<typename Point, origin Origin, scale Scale>
constexpr inline coord_point<Point, Origin, Scale> &operator-=( coord_point<Point, Origin, Scale>
        &me, const coord_point<Point, origin::relative, Scale> &r )
{
    me.raw() -= r.raw();
    return me;
}

template<typename Point, origin Origin, scale Scale>
constexpr inline coord_point<Point, Origin, Scale> &operator+=( coord_point<Point, Origin, Scale>
        &me, const point &r )
{
    me.raw() += r;
    return me;
}

template<typename Point, origin Origin, scale Scale>
constexpr inline coord_point<Point, Origin, Scale> &operator-=( coord_point<Point, Origin, Scale>
        &me, const point &r )
{
    me.raw() -= r;
    return me;
}

template<typename Point, origin Origin, scale Scale>
constexpr inline coord_point<Point, Origin, Scale> &operator+=( coord_point<Point, Origin, Scale>
        &me, const tripoint &r )
{
    me.raw() += r;
    return me;
}

template<typename Point, origin Origin, scale Scale>
constexpr inline coord_point<Point, Origin, Scale> &operator-=( coord_point<Point, Origin, Scale>
        &me, const tripoint &r )
{
    me.raw() -= r;
    return me;
}

template<typename Point, origin Origin, scale Scale, bool LhsInBounds, bool RhsInBounds>
constexpr inline bool operator==( const coord_point<Point, Origin, Scale, LhsInBounds> &l,
                                  const coord_point<Point, Origin, Scale, RhsInBounds> &r )
{
    return l.raw() == r.raw();
}

template<typename Point, origin Origin, scale Scale, bool LhsInBounds, bool RhsInBounds>
constexpr inline bool operator!=( const coord_point<Point, Origin, Scale, LhsInBounds> &l,
                                  const coord_point<Point, Origin, Scale, RhsInBounds> &r )
{
    return l.raw() != r.raw();
}

template<typename Point, origin Origin, scale Scale, bool LhsInBounds, bool RhsInBounds>
constexpr inline bool operator<( const coord_point<Point, Origin, Scale, LhsInBounds> &l,
                                 const coord_point<Point, Origin, Scale, RhsInBounds> &r )
{
    return l.raw() < r.raw();
}

template<typename PointL, typename PointR, origin OriginL, scale Scale, bool InBounds>
constexpr inline auto operator+(
    const coord_point<PointL, OriginL, Scale, InBounds> &l,
    const coord_point<PointR, origin::relative, Scale> &r )
{
    using PointResult = decltype( PointL() + PointR() );
    return coord_point<PointResult, OriginL, Scale>( l.raw() + r.raw() );
}

template < typename PointL, typename PointR, origin OriginR, scale Scale, bool InBounds,
           // enable_if to prevent ambiguity with above when both args are
           // relative
           typename = std::enable_if_t < OriginR != origin::relative >>
constexpr inline auto operator+(
    const coord_point<PointL, origin::relative, Scale> &l,
    const coord_point<PointR, OriginR, Scale, InBounds> &r )
{
    using PointResult = decltype( PointL() + PointR() );
    return coord_point<PointResult, OriginR, Scale>( l.raw() + r.raw() );
}

template<typename PointL, typename PointR, origin OriginL, scale Scale, bool InBounds>
constexpr inline auto operator-(
    const coord_point<PointL, OriginL, Scale, InBounds> &l,
    const coord_point<PointR, origin::relative, Scale> &r )
{
    using PointResult = decltype( PointL() + PointR() );
    return coord_point<PointResult, OriginL, Scale>( l.raw() - r.raw() );
}

template < typename PointL, typename PointR, origin Origin, scale Scale, bool LhsInBounds,
           bool RhsInBounds,
           // enable_if to prevent ambiguity with above when both args are
           // relative
           typename = std::enable_if_t < Origin != origin::relative >>
constexpr inline auto operator-(
    const coord_point<PointL, Origin, Scale, LhsInBounds> &l,
    const coord_point<PointR, Origin, Scale, RhsInBounds> &r )
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

template<typename Point, origin Origin, scale Scale, bool InBounds>
inline std::ostream &operator<<( std::ostream &os,
                                 const coord_point<Point, Origin, Scale, InBounds> &p )
{
    return os << p.raw();
}

template <typename Point, origin Origin, scale Scale, bool LhsInBounds, bool RhsInBounds>
constexpr inline coord_point < Point, Origin, Scale, LhsInBounds &&RhsInBounds >
coord_min( const coord_point<Point, Origin, Scale, LhsInBounds> &l,
           const coord_point<Point, Origin, Scale, RhsInBounds> &r )
{
    return coord_point < Point, Origin, Scale, LhsInBounds &&
           RhsInBounds >::make_unchecked( std::min( l.x(), r.x() ), std::min( l.y(), r.y() ), std::min( l.z(),
                                          r.z() ) );
}

template <typename Point, origin Origin, scale Scale, bool LhsInBounds, bool RhsInBounds>
constexpr inline coord_point < Point, Origin, Scale, LhsInBounds &&RhsInBounds >
coord_max( const coord_point<Point, Origin, Scale, LhsInBounds> &l,
           const coord_point<Point, Origin, Scale, RhsInBounds> &r )
{
    return coord_point < Point, Origin, Scale, LhsInBounds &&
           RhsInBounds >::make_unchecked( std::max( l.x(), r.x() ), std::max( l.y(), r.y() ), std::max( l.z(),
                                          r.z() ) );
}

template<int ScaleUp, int ScaleDown, scale ResultScale, bool InBounds>
struct project_to_impl;

template<int ScaleUp, scale ResultScale, bool InBounds>
struct project_to_impl<ScaleUp, 0, ResultScale, InBounds> {
    template<typename Point, origin Origin, scale SourceScale>
    coord_point<Point, Origin, ResultScale, InBounds> operator()(
        const coord_point<Point, Origin, SourceScale, InBounds> &src ) {
        // Inbounds points are guaranteed to be inbounds after scaling up.
        //
        // e.g. point_bub_sm_ib scaled up to point_bub_ms_ib points to the top
        // left of the submap, which is still inbounds.
        return coord_point<Point, Origin, ResultScale, InBounds>::make_unchecked( multiply_xy( src.raw(),
                ScaleUp ) );
    }
};

template<int ScaleDown, scale ResultScale>
struct project_to_impl<0, ScaleDown, ResultScale, false> {
    template<typename Point, origin Origin, scale SourceScale>
    coord_point<Point, Origin, ResultScale> operator()(
        const coord_point<Point, Origin, SourceScale> &src ) {
        return coord_point<Point, Origin, ResultScale>(
                   divide_xy_round_to_minus_infinity( src.raw(), ScaleDown ) );
    }
};

template<int ScaleDown, scale ResultScale>
struct project_to_impl<0, ScaleDown, ResultScale, true> {
    template<typename Point, origin Origin, scale SourceScale>
    coord_point<Point, Origin, ResultScale, true> operator()(
        const coord_point<Point, Origin, SourceScale, true> &src ) {
        // Inbounds points are guaranteed to be inbounds after scaling down.
        //
        // They are also guaranteed to be >= 0, so we can use a more effecient method of scaling.
        return coord_point<Point, Origin, ResultScale, true>::make_unchecked(
                   divide_xy_round_to_minus_infinity_non_negative( src.raw(), ScaleDown ) );
    }
};

template<scale ResultScale, typename Point, origin Origin, scale SourceScale, bool InBounds>
inline auto project_to( const coord_point<Point, Origin, SourceScale, InBounds> &src )
{
    constexpr int scale_down = map_squares_per( ResultScale ) / map_squares_per( SourceScale );
    constexpr int scale_up = map_squares_per( SourceScale ) / map_squares_per( ResultScale );
    return project_to_impl<scale_up, scale_down, ResultScale, InBounds>()( src );
}

// Resolves the remainer type for project_remain. In most cases the result shares
// the same InBounds as the source, however some cases let us get inbounds results
// from out of bounds input.
//
// This needs to be special cased, as due to the existance of tinymap we cannot
// do this for origin::reality_bubble
template<typename Point, origin RemainderOrigin, scale SourceScale, bool InBounds>
struct remainder_inbounds {
    using type = coord_point<Point, RemainderOrigin, SourceScale, InBounds>;
};

// *_sm_ms are always inbounds as remainder.
template<typename Point, bool InBounds>
struct remainder_inbounds<Point, origin::submap, scale::map_square, InBounds> {
    using type = coord_point<Point, origin::submap, scale::map_square, true>;
};

template<typename Point, origin RemainderOrigin, scale SourceScale, bool InBounds>
using remainder_inbounds_t = typename
                             remainder_inbounds<Point, RemainderOrigin, SourceScale, InBounds>::type;

template<origin Origin, scale CoarseScale, scale FineScale, bool InBounds>
struct quotient_remainder_helper {
    constexpr static origin RemainderOrigin = origin_from_scale( CoarseScale );
    using quotient_type = coord_point<point, Origin, CoarseScale, InBounds>;
    using quotient_type_tripoint = coord_point<tripoint, Origin, CoarseScale, InBounds>;
    using remainder_type = remainder_inbounds_t<point, RemainderOrigin, FineScale, InBounds>;
    using remainder_type_tripoint =
        remainder_inbounds_t<tripoint, RemainderOrigin, FineScale, InBounds>;
};

template<origin Origin, scale CoarseScale, scale FineScale, bool InBounds>
struct quotient_remainder_point {
    using helper = quotient_remainder_helper<Origin, CoarseScale, FineScale, InBounds>;
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

template<origin Origin, scale CoarseScale, scale FineScale, bool InBounds>
struct quotient_remainder_tripoint {
    using helper = quotient_remainder_helper<Origin, CoarseScale, FineScale, InBounds>;
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
//
// If passing a tripoint to project_remain, you must choose exactly one of the
// quotient or remainder to get the z coordinate.  Both of these should work:
//  tripoint_abs_sm val;
//
//  tripoint_abs_om quotient;
//  point_om_sm remainder;
//  std::tie( quotient, remainder ) = project_remain<coords::om>( val );
//
//  point_abs_om quotient;
//  tripoint_om_sm remainder;
//  std::tie( quotient, remainder ) = project_remain<coords::om>( val );
template<scale ResultScale, origin Origin, scale SourceScale, bool InBounds>
inline quotient_remainder_point<Origin, ResultScale, SourceScale, InBounds> project_remain(
    const coord_point<point, Origin, SourceScale, InBounds> &src )
{
    constexpr int ScaleDown = map_squares_per( ResultScale ) / map_squares_per( SourceScale );
    static_assert( ScaleDown > 0, "You can only project to coarser coordinate systems" );
    constexpr static origin RemainderOrigin = origin_from_scale( ResultScale );
    coord_point<point, Origin, ResultScale, InBounds> quotient = project_to< ResultScale>( src );
    remainder_inbounds_t<point, RemainderOrigin, SourceScale, InBounds> remainder =
        remainder_inbounds_t<point, RemainderOrigin, SourceScale, InBounds>::make_unchecked(
            src.raw() - quotient.raw() * ScaleDown );

    return { quotient, remainder };
}

template<scale ResultScale, origin Origin, scale SourceScale, bool InBounds>
inline quotient_remainder_tripoint<Origin, ResultScale, SourceScale, InBounds> project_remain(
    const coord_point<tripoint, Origin, SourceScale, InBounds> &src )
{
    using qrp = quotient_remainder_tripoint<Origin, ResultScale, SourceScale, InBounds>;
    using quotient_type_tripoint = typename qrp::quotient_type_tripoint;
    using remainder_type_tripoint = typename qrp::remainder_type_tripoint;
    quotient_remainder_point<Origin, ResultScale, SourceScale, InBounds> point_result =
        project_remain<ResultScale>( src.xy() );
    return { point_result.quotient, point_result.remainder,
             quotient_type_tripoint::make_unchecked( point_result.quotient, src.z() ),
             remainder_type_tripoint::make_unchecked( point_result.remainder, src.z() ) };
}

template<typename PointL, typename PointR, origin CoarseOrigin, scale CoarseScale,
         origin FineOrigin, scale FineScale, bool CoarseInBounds, bool FineInBounds>
inline auto project_combine(
    const coord_point<PointL, CoarseOrigin, CoarseScale, CoarseInBounds> &coarse,
    const coord_point<PointR, FineOrigin, FineScale, FineInBounds> &fine )
{
    static_assert( origin_from_scale( CoarseScale ) == FineOrigin,
                   "given point types are not compatible for combination" );
    static_assert( PointL::dimension != 3 || PointR::dimension != 3,
                   "two tripoints should not be combined; it's unclear how to handle z" );
    using PointResult = decltype( PointL() + PointR() );
    const coord_point<PointL, CoarseOrigin, FineScale> refined_coarse =
        project_to<FineScale>( coarse );
    return coord_point < PointResult, CoarseOrigin, FineScale, CoarseInBounds &&
           FineInBounds >::make_unchecked( refined_coarse.raw() + fine.raw() );
}

template<scale FineScale, origin Origin, scale CoarseScale, bool InBounds>
inline auto project_bounds( const coord_point<point, Origin, CoarseScale, InBounds> &coarse )
{
    constexpr point one( 1, 1 ); // NOLINT(cata-use-named-point-constants)
    return half_open_rectangle<coord_point<point, Origin, FineScale>>(
               project_to<FineScale>( coarse ), project_to<FineScale>( coarse + one ) );
}

template<scale FineScale, origin Origin, scale CoarseScale, bool InBounds>
inline auto project_bounds( const coord_point<tripoint, Origin, CoarseScale, InBounds> &coarse )
{
    constexpr point one( 1, 1 ); // NOLINT(cata-use-named-point-constants)
    return half_open_cuboid<coord_point<tripoint, Origin, FineScale>>(
               project_to<FineScale>( coarse ), project_to<FineScale>( coarse + one ) );
}

} // namespace coords

template<typename Point, coords::origin Origin, coords::scale Scale, bool InBounds>
// NOLINTNEXTLINE(cert-dcl58-cpp)
struct std::hash<coords::coord_point<Point, Origin, Scale, InBounds>> {
    std::size_t operator()( const coords::coord_point<Point, Origin, Scale, InBounds> &p ) const {
        const hash<Point> h{};
        return h( p.raw() );
    }
};

/** Typedefs for point types with coordinate mnemonics.
 *
 * Each name is of the form (tri)point_<origin>_<scale>(_ib) where <origin> tells you
 * the context in which the point has meaning, and <scale> tells you what one unit
 * of the point means. The optional "_ib" suffix denotes that the type is guaranteed
 * to be inbounds.
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
using point_sm_ms_ib = coords::coord_point<point, coords::origin::submap, coords::ms, true>;
using point_omt_ms = coords::coord_point<point, coords::origin::overmap_terrain, coords::ms>;
using point_bub_ms = coords::coord_point<point, coords::origin::reality_bubble, coords::ms>;
using point_bub_ms_ib =
    coords::coord_point<point, coords::origin::reality_bubble, coords::ms, true>;
using point_rel_sm = coords::coord_point<point, coords::origin::relative, coords::sm>;
using point_abs_sm = coords::coord_point<point, coords::origin::abs, coords::sm>;
using point_omt_sm = coords::coord_point<point, coords::origin::overmap_terrain, coords::sm>;
using point_om_sm = coords::coord_point<point, coords::origin::overmap, coords::sm>;
using point_bub_sm = coords::coord_point<point, coords::origin::reality_bubble, coords::sm>;
using point_bub_sm_ib =
    coords::coord_point<point, coords::origin::reality_bubble, coords::sm, true>;
using point_rel_omt = coords::coord_point<point, coords::origin::relative, coords::omt>;
using point_abs_omt = coords::coord_point<point, coords::origin::abs, coords::omt>;
using point_om_omt = coords::coord_point<point, coords::origin::overmap, coords::omt>;
using point_abs_seg = coords::coord_point<point, coords::origin::abs, coords::seg>;
using point_rel_om = coords::coord_point<point, coords::origin::relative, coords::om>;
using point_abs_om = coords::coord_point<point, coords::origin::abs, coords::om>;

using tripoint_rel_ms = coords::coord_point<tripoint, coords::origin::relative, coords::ms>;
using tripoint_abs_ms = coords::coord_point<tripoint, coords::origin::abs, coords::ms>;
using tripoint_sm_ms = coords::coord_point<tripoint, coords::origin::submap, coords::ms>;
using tripoint_sm_ms_ib = coords::coord_point<tripoint, coords::origin::submap, coords::ms, true>;
using tripoint_omt_ms = coords::coord_point<tripoint, coords::origin::overmap_terrain, coords::ms>;
using tripoint_bub_ms = coords::coord_point<tripoint, coords::origin::reality_bubble, coords::ms>;
using tripoint_bub_ms_ib =
    coords::coord_point<tripoint, coords::origin::reality_bubble, coords::ms, true>;
using tripoint_rel_sm = coords::coord_point<tripoint, coords::origin::relative, coords::sm>;
using tripoint_abs_sm = coords::coord_point<tripoint, coords::origin::abs, coords::sm>;
using tripoint_om_sm = coords::coord_point<tripoint, coords::origin::overmap, coords::sm>;
using tripoint_bub_sm = coords::coord_point<tripoint, coords::origin::reality_bubble, coords::sm>;
using tripoint_bub_sm_ib =
    coords::coord_point<tripoint, coords::origin::reality_bubble, coords::sm, true>;
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

template<typename Point, coords::origin Origin, coords::scale Scale, bool LhsInBounds, bool RhsInBounds>
inline int square_dist( const coords::coord_point<Point, Origin, Scale, LhsInBounds> &loc1,
                        const coords::coord_point<Point, Origin, Scale, RhsInBounds> &loc2 )
{
    return square_dist( loc1.raw(), loc2.raw() );
}

template<typename Point, coords::origin Origin, coords::scale Scale, bool LhsInBounds, bool RhsInBounds>
inline int trig_dist( const coords::coord_point<Point, Origin, Scale, LhsInBounds> &loc1,
                      const coords::coord_point<Point, Origin, Scale, RhsInBounds> &loc2 )
{
    return trig_dist( loc1.raw(), loc2.raw() );
}

template<typename Point, coords::origin Origin, coords::scale Scale, bool LhsInBounds, bool RhsInBounds>
inline int rl_dist( const coords::coord_point<Point, Origin, Scale, LhsInBounds> &loc1,
                    const coords::coord_point<Point, Origin, Scale, RhsInBounds> &loc2 )
{
    return rl_dist( loc1.raw(), loc2.raw() );
}

template<typename Point, coords::origin Origin, coords::scale Scale, bool LhsInBounds, bool RhsInBounds>
inline int manhattan_dist( const coords::coord_point<Point, Origin, Scale, LhsInBounds> &loc1,
                           const coords::coord_point<Point, Origin, Scale, RhsInBounds> &loc2 )
{
    return manhattan_dist( loc1.raw(), loc2.raw() );
}

template<typename Point, coords::origin Origin, coords::scale Scale, bool LhsInBounds, bool RhsInBounds>
inline int octile_dist( const coords::coord_point<Point, Origin, Scale, LhsInBounds> &loc1,
                        const coords::coord_point<Point, Origin, Scale, RhsInBounds> &loc2, int multiplier = 1 )
{
    return octile_dist( loc1.raw(), loc2.raw(), multiplier );
}

template<typename Point, coords::origin Origin, coords::scale Scale, bool LhsInBounds, bool RhsInBounds>
direction direction_from( const coords::coord_point<Point, Origin, Scale, LhsInBounds> &loc1,
                          const coords::coord_point<Point, Origin, Scale, RhsInBounds> &loc2 )
{
    return direction_from( loc1.raw(), loc2.raw() );
}

template<typename Point, coords::origin Origin, coords::scale Scale, bool LhsInBounds, bool RhsInBounds>
std::vector < coords::coord_point < Point, Origin, Scale, LhsInBounds &&RhsInBounds >>
        line_to( const coords::coord_point<Point, Origin, Scale, LhsInBounds> &loc1,
                 const coords::coord_point<Point, Origin, Scale, RhsInBounds> &loc2 )
{
    std::vector<Point> raw_result = line_to( loc1.raw(), loc2.raw() );
    std::vector < coords::coord_point < Point, Origin, Scale, LhsInBounds &&RhsInBounds >> result;
    std::transform( raw_result.begin(), raw_result.end(), std::back_inserter( result ),
    []( const Point & p ) {
        return coords::coord_point < Point, Origin, Scale, LhsInBounds &&
               RhsInBounds >::make_unchecked( p );
    } );
    return result;
}

template<typename Point, coords::origin Origin, coords::scale Scale, bool LhsInBounds, bool RhsInBounds>
coords::coord_point < Point, Origin, Scale, LhsInBounds &&RhsInBounds >
midpoint( const coords::coord_point<Point, Origin, Scale, LhsInBounds> &loc1,
          const coords::coord_point<Point, Origin, Scale, RhsInBounds> &loc2 )
{
    return coords::coord_point < Point, Origin, Scale, LhsInBounds &&
           RhsInBounds >::make_unchecked( ( loc1.raw() + loc2.raw() ) / 2 );
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
template<typename Point, coords::origin Origin, coords::scale Scale, bool InBounds>
std::vector<coords::coord_point<Point, Origin, Scale>>
        closest_points_first( const coords::coord_point<Point, Origin, Scale, InBounds> &loc,
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

#pragma once
#ifndef CATA_SRC_COORDINATES_H
#define CATA_SRC_COORDINATES_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iosfwd>
#include <iterator>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "cata_inline.h"
#include "coords_fwd.h"  // IWYU pragma: export
#include "cuboid_rectangle.h"
#include "debug.h"
#include "line.h"  // IWYU pragma: keep
#include "map_scale_constants.h"
#include "point.h"

class JsonOut;
class JsonValue;

namespace coords
{
template <typename Point, origin Origin, scale Scale> class coord_point_ob;

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
 * For more details see doc/c++/POINTS_COORDINATES.md.
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

template<typename Point, typename Subpoint>
class coord_point_mut : public coord_point_base<Point>
{
    protected:
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

template<typename Point, origin Origin, scale Scale>
class coord_point_ob_rel
{
    public:
        static const coord_point_ob<Point, Origin, Scale> north;
        static const coord_point_ob<Point, Origin, Scale> north_east;
        static const coord_point_ob<Point, Origin, Scale> east;
        static const coord_point_ob<Point, Origin, Scale> south_east;
        static const coord_point_ob<Point, Origin, Scale> south;
        static const coord_point_ob<Point, Origin, Scale> south_west;
        static const coord_point_ob<Point, Origin, Scale> west;
        static const coord_point_ob<Point, Origin, Scale> north_west;
};

template<typename Point, origin Origin, scale Scale>
class coord_point_ob_3d
{
    public:
        static const coord_point_ob<Point, Origin, Scale> above;
        static const coord_point_ob<Point, Origin, Scale> below;
};

class coord_point_ob_not_rel {};
class coord_point_ob_not_3d {};

template<typename Point, origin Origin, scale Scale>
class coord_point_ob : public
    coord_point_mut<Point, coord_point_ob<point, Origin, Scale>>,
            public std::
            conditional_t<Origin == origin::relative, coord_point_ob_rel<Point, Origin, Scale>, coord_point_ob_not_rel>,
            public std::
            conditional_t < Origin == origin::relative &&Point::dimension == 3,
            coord_point_ob_3d<Point, Origin, Scale>, coord_point_ob_not_3d >
{
        using base = coord_point_mut<Point, coord_point_ob<point, Origin, Scale>>;

    public:
        using base::base;

        static constexpr int dimension = Point::dimension;
        // Coordinate representing the origin
        static const coord_point_ob zero;
        // Coordinate with minimum representable coordinates
        static const coord_point_ob min;
        // Coordinate with maximum representable coordinates
        static const coord_point_ob max;
        // Sentinel value for returning and detecting invalid coordinates.
        // Equal to @ref min for backward compatibility.
        static const coord_point_ob invalid;
        constexpr bool is_invalid() const {
            return *this == invalid;
        }

        static constexpr bool is_inbounds = false;
        using this_as_tripoint = coord_point_ob<tripoint, Origin, Scale>;
        using this_as_point = coord_point_ob<point, Origin, Scale>;
        using this_as_ob = coord_point_ob<Point, Origin, Scale>;
        using this_as_tripoint_ob = coord_point_ob<tripoint, Origin, Scale>;

        template <typename T>
        static constexpr coord_point_ob make_unchecked( T x, T y ) {
            return coord_point_ob( x, y );
        }
        template <typename T>
        static constexpr coord_point_ob make_unchecked( T x, T y, T z ) {
            return coord_point_ob( x, y, z );
        }
        static constexpr coord_point_ob make_unchecked( const base &other ) {
            return coord_point_ob( other );
        }
        static constexpr coord_point_ob make_unchecked( const Point &other ) {
            return coord_point_ob( other );
        }
        template <typename T>
        static constexpr coord_point_ob make_unchecked( const this_as_point &other, T z ) {
            return coord_point_ob( other, z );
        }
        template <typename T>
        static constexpr coord_point_ob make_unchecked( const point &other, T z ) {
            return coord_point_ob( other.x, other.y, z );
        }

        constexpr auto xy() const {
            return this_as_point( this->raw().xy() );
        }

        constexpr auto abs() const {
            return coord_point_ob( this->raw().abs() );
        }

        constexpr auto rotate( int turns, const point &dim = point::south_east ) const {
            return coord_point_ob( this->raw().rotate( turns, dim ) );
        }

        friend inline this_as_ob operator+( const coord_point_ob &l, const point &r ) {
            return this_as_ob( l.raw() + r );
        }

        friend inline this_as_tripoint_ob operator+( const coord_point_ob &l, const tripoint &r ) {
            return this_as_tripoint_ob( l.raw() + r );
        }

        friend inline this_as_ob operator+( const point &l, const coord_point_ob &r ) {
            return this_as_ob( l + r.raw() );
        }

        friend inline this_as_tripoint_ob operator+( const tripoint &l, const coord_point_ob &r ) {
            return this_as_tripoint_ob( l + r.raw() );
        }

        friend inline this_as_ob operator-( const coord_point_ob &l, const point &r ) {
            return this_as_ob( l.raw() - r );
        }

        friend inline this_as_tripoint_ob operator-( const coord_point_ob &l, const tripoint &r ) {
            return this_as_tripoint_ob( l.raw() - r );
        }
};

// These definitions can go in the class in clang and gcc, and are much shorter there,
// but MSVC doesn't allow that, so...
template<typename Point, origin Origin, scale Scale>
const coord_point_ob<Point, Origin, Scale>
coord_point_ob<Point, Origin, Scale>::min =
    coord_point_ob<Point, Origin, Scale>( Point::min );
template<typename Point, origin Origin, scale Scale>
const coord_point_ob<Point, Origin, Scale>
coord_point_ob<Point, Origin, Scale>::max =
    coord_point_ob<Point, Origin, Scale>( Point::max );
template<typename Point, origin Origin, scale Scale>
const coord_point_ob<Point, Origin, Scale>
coord_point_ob<Point, Origin, Scale>::invalid =
    coord_point_ob<Point, Origin, Scale>( Point::invalid );
template<typename Point, origin Origin, scale Scale>
const coord_point_ob<Point, Origin, Scale>
coord_point_ob<Point, Origin, Scale>::zero =
    coord_point_ob<Point, Origin, Scale>( Point::zero );
template<typename Point, origin Origin, scale Scale>
const coord_point_ob<Point, Origin, Scale>
coord_point_ob_rel<Point, Origin, Scale>::north
    =
        coord_point_ob<Point, Origin, Scale>( Point::north );
template<typename Point, origin Origin, scale Scale>
const coord_point_ob<Point, Origin, Scale>
coord_point_ob_rel<Point, Origin, Scale>::north_east
    =
        coord_point_ob<Point, Origin, Scale>( Point::north_east );
template<typename Point, origin Origin, scale Scale>
const coord_point_ob<Point, Origin, Scale>
coord_point_ob_rel<Point, Origin, Scale>::east
    =
        coord_point_ob<Point, Origin, Scale>( Point::east );
template<typename Point, origin Origin, scale Scale>
const coord_point_ob<Point, Origin, Scale>
coord_point_ob_rel<Point, Origin, Scale>::south_east
    =
        coord_point_ob<Point, Origin, Scale>( Point::south_east );
template<typename Point, origin Origin, scale Scale>
const coord_point_ob<Point, Origin, Scale>
coord_point_ob_rel<Point, Origin, Scale>::south
    =
        coord_point_ob<Point, Origin, Scale>( Point::south );
template<typename Point, origin Origin, scale Scale>
const coord_point_ob<Point, Origin, Scale>
coord_point_ob_rel<Point, Origin, Scale>::south_west
    =
        coord_point_ob<Point, Origin, Scale>( Point::south_west );
template<typename Point, origin Origin, scale Scale>
const coord_point_ob<Point, Origin, Scale>
coord_point_ob_rel<Point, Origin, Scale>::west
    =
        coord_point_ob<Point, Origin, Scale>( Point::west );
template<typename Point, origin Origin, scale Scale>
const coord_point_ob<Point, Origin, Scale>
coord_point_ob_rel<Point, Origin, Scale>::north_west
    =
        coord_point_ob<Point, Origin, Scale>( Point::north_west );
template<typename Point, origin Origin, scale Scale>
const coord_point_ob<Point, Origin, Scale>
coord_point_ob_3d<Point, Origin, Scale>::above
    =
        coord_point_ob<Point, Origin, Scale>( Point::above );
template<typename Point, origin Origin, scale Scale>
const coord_point_ob<Point, Origin, Scale>
coord_point_ob_3d<Point, Origin, Scale>::below
    =
        coord_point_ob<Point, Origin, Scale>( Point::below );

template<typename Point, origin Origin, scale Scale>
class coord_point_ib : public coord_point_ob<Point, Origin, Scale>
{
        using base = coord_point_ob<Point, Origin, Scale>;
        using base::base;

    public:
        static constexpr bool is_inbounds = true;
        using this_as_tripoint = coord_point_ib<tripoint, Origin, Scale>;
        using this_as_point = coord_point_ib<point, Origin, Scale>;
        using this_as_tripoint_ob = coord_point_ob<tripoint, Origin, Scale>;
        using this_as_ob = base;

        // Explicit functions to construct inbounds versions without doing any
        // bounds checking. Only use these with a very good reason, when you
        // are completely sure the result will be inbounds.
        template <typename T>
        static constexpr coord_point_ib make_unchecked( T x, T y ) {
            return coord_point_ib( x, y );
        }
        template <typename T>
        static constexpr coord_point_ib make_unchecked( T x, T y, T z ) {
            return coord_point_ib( x, y, z );
        }
        static constexpr coord_point_ib make_unchecked( const base &other ) {
            return coord_point_ib( other );
        }
        static constexpr coord_point_ib make_unchecked( const Point &other ) {
            return coord_point_ib( other );
        }
        template <typename T>
        static constexpr coord_point_ib make_unchecked( const this_as_point &other, T z ) {
            return coord_point_ib( other, z );
        }
        template <typename T>
        static constexpr coord_point_ib make_unchecked( const point &other, T z ) {
            return coord_point_ib( other.x, other.y, z );
        }

        // Allow implicit conversions from inbounds to out of bounds.
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator const this_as_ob &() const {
            return *this;
        }

        const this_as_ob &as_ob() const {
            return *this;
        }

        constexpr auto xy() const {
            return this_as_point::make_unchecked( this->raw().xy() );
        }
};

template<typename Point, origin Origin, scale Scale>
constexpr inline coord_point_ob<Point, Origin, Scale> &operator+=
( coord_point_ob<Point, Origin, Scale>
  &me, const coord_point_ob<Point, origin::relative, Scale> &r )
{
    me.raw() += r.raw();
    return me;
}

template<typename Point, origin Origin, scale Scale>
constexpr inline coord_point_ob<Point, Origin, Scale> &operator-=
( coord_point_ob<Point, Origin, Scale>
  &me, const coord_point_ob<Point, origin::relative, Scale> &r )
{
    me.raw() -= r.raw();
    return me;
}

template<typename Point, origin Origin, scale Scale>
constexpr inline coord_point_ob<Point, Origin, Scale> &operator+=
( coord_point_ob<Point, Origin, Scale>
  &me, const point &r )
{
    me.raw() += r;
    return me;
}

template<typename Point, origin Origin, scale Scale>
constexpr inline coord_point_ob<Point, Origin, Scale> &operator-=
( coord_point_ob<Point, Origin, Scale>
  &me, const point &r )
{
    me.raw() -= r;
    return me;
}

template<typename Point, origin Origin, scale Scale>
constexpr inline coord_point_ob<Point, Origin, Scale> &operator+=
( coord_point_ob<Point, Origin, Scale>
  &me, const tripoint &r )
{
    me.raw() += r;
    return me;
}

template<typename Point, origin Origin, scale Scale>
constexpr inline coord_point_ob<Point, Origin, Scale> &operator-=
( coord_point_ob<Point, Origin, Scale>
  &me, const tripoint &r )
{
    me.raw() -= r;
    return me;
}

template<typename Point, origin Origin, scale Scale>
constexpr inline bool operator==( const coord_point_ob<Point, Origin, Scale> &l,
                                  const coord_point_ob<Point, Origin, Scale> &r )
{
    return l.raw() == r.raw();
}

template<typename Point, origin Origin, scale Scale>
constexpr inline bool operator!=( const coord_point_ob<Point, Origin, Scale> &l,
                                  const coord_point_ob<Point, Origin, Scale> &r )
{
    return l.raw() != r.raw();
}

template<typename Point, origin Origin, scale Scale>
constexpr inline bool operator<( const coord_point_ob<Point, Origin, Scale> &l,
                                 const coord_point_ob<Point, Origin, Scale> &r )
{
    return l.raw() < r.raw();
}

template<typename PointL, typename PointR, origin OriginL, scale Scale>
constexpr inline auto operator+(
    const coord_point_ob<PointL, OriginL, Scale> &l,
    const coord_point_ob<PointR, origin::relative, Scale> &r )
{
    using PointResult = decltype( PointL() + PointR() );
    return coord_point_ob<PointResult, OriginL, Scale>( l.raw() + r.raw() );
}

template < typename PointL, typename PointR, origin OriginR, scale Scale,
           // enable_if to prevent ambiguity with above when both args are
           // relative
           typename = std::enable_if_t < OriginR != origin::relative >>
constexpr inline auto operator+(
    const coord_point_ob<PointL, origin::relative, Scale> &l,
    const coord_point_ob<PointR, OriginR, Scale> &r )
{
    using PointResult = decltype( PointL() + PointR() );
    return coord_point_ob<PointResult, OriginR, Scale>( l.raw() + r.raw() );
}

template<typename PointL, typename PointR, origin OriginL, scale Scale>
constexpr inline auto operator-(
    const coord_point_ob<PointL, OriginL, Scale> &l,
    const coord_point_ob<PointR, origin::relative, Scale> &r )
{
    using PointResult = decltype( PointL() + PointR() );
    return coord_point_ob<PointResult, OriginL, Scale>( l.raw() - r.raw() );
}

template < typename PointL, typename PointR, origin Origin, scale Scale,
           // enable_if to prevent ambiguity with above when both args are
           // relative
           typename = std::enable_if_t < Origin != origin::relative >>
constexpr inline auto operator-(
    const coord_point_ob<PointL, Origin, Scale> &l,
    const coord_point_ob<PointR, Origin, Scale> &r )
{
    using PointResult = decltype( PointL() + PointR() );
    return coord_point_ob<PointResult, origin::relative, Scale>( l.raw() - r.raw() );
}

template<typename Point, origin Origin, scale Scale>
constexpr inline auto operator-(
    const coord_point_ob<Point, Origin, Scale> &l )
{
    using PointResult = decltype( Point() );
    return coord_point_ob<PointResult, Origin, Scale>( - l.raw() );
}

// Only relative points can be multiplied by a constant
template<typename Point, scale Scale>
constexpr inline coord_point_ob<Point, origin::relative, Scale> operator*(
    int l, const coord_point_ob<Point, origin::relative, Scale> &r )
{
    return coord_point_ob<Point, origin::relative, Scale>( l * r.raw() );
}

template<typename Point, scale Scale>
constexpr inline coord_point_ob<Point, origin::relative, Scale> operator*(
    const coord_point_ob<Point, origin::relative, Scale> &r, int l )
{
    return coord_point_ob<Point, origin::relative, Scale>( r.raw() * l );
}

template<typename Point, origin Origin, scale Scale>
inline std::ostream &operator<<( std::ostream &os,
                                 const coord_point_ob<Point, Origin, Scale> &p )
{
    return os << p.raw();
}

template <typename Point, origin Origin, scale Scale>
constexpr inline coord_point < Point, Origin, Scale>
coord_min( const coord_point_ob<Point, Origin, Scale> &l,
           const coord_point_ob<Point, Origin, Scale> &r )
{
    return coord_point < Point, Origin, Scale >::make_unchecked( std::min( l.x(), r.x() ),
            std::min( l.y(), r.y() ), std::min( l.z(),
                    r.z() ) );
}

template <typename Point, origin Origin, scale Scale>
constexpr inline coord_point < Point, Origin, Scale >
coord_max( const coord_point_ob<Point, Origin, Scale> &l,
           const coord_point_ob<Point, Origin, Scale> &r )
{
    return coord_point < Point, Origin, Scale >::make_unchecked( std::max( l.x(), r.x() ),
            std::max( l.y(), r.y() ), std::max( l.z(),
                    r.z() ) );
}

// Min and max of inbounds points results in an inbounds point.

template <typename Point, origin Origin, scale Scale>
constexpr inline coord_point_ib < Point, Origin, Scale>
coord_min( const coord_point_ib<Point, Origin, Scale> &l,
           const coord_point_ib<Point, Origin, Scale> &r )
{
    return coord_point_ib < Point, Origin, Scale >::make_unchecked( std::min( l.x(), r.x() ),
            std::min( l.y(), r.y() ), std::min( l.z(),
                    r.z() ) );
}

template <typename Point, origin Origin, scale Scale>
constexpr inline coord_point_ib < Point, Origin, Scale >
coord_max( const coord_point_ib<Point, Origin, Scale> &l,
           const coord_point_ib<Point, Origin, Scale> &r )
{
    return coord_point_ib < Point, Origin, Scale >::make_unchecked( std::max( l.x(), r.x() ),
            std::max( l.y(), r.y() ), std::max( l.z(),
                    r.z() ) );
}

template<int ScaleUp, int ScaleDown, scale ResultScale>
struct project_to_impl;

template<int ScaleUp, scale ResultScale>
struct project_to_impl<ScaleUp, 0, ResultScale> {
    template<template<class, origin, scale> class coord, typename Point, origin Origin, scale SourceScale>
    coord<Point, Origin, ResultScale> CATA_FORCEINLINE operator()(
        const coord<Point, Origin, SourceScale> src ) {
        // Inbounds points are guaranteed to be inbounds after scaling up.
        //
        // e.g. point_bub_sm_ib scaled up to point_bub_ms_ib points to the top
        // left of the submap, which is still inbounds.
        return coord<Point, Origin, ResultScale>::make_unchecked( multiply_xy( src.raw(),
                ScaleUp ) );
    }
};

template<int ScaleDown, scale ResultScale>
struct project_to_impl<0, ScaleDown, ResultScale> {
    template<typename Point, origin Origin, scale SourceScale>
    coord_point_ob<Point, Origin, ResultScale> CATA_FORCEINLINE operator()(
        const coord_point_ob<Point, Origin, SourceScale> src ) {
        return coord_point_ob<Point, Origin, ResultScale>(
                   divide_xy_round_to_minus_infinity( src.raw(), ScaleDown ) );
    }

    template<typename Point, origin Origin, scale SourceScale>
    coord_point_ib<Point, Origin, ResultScale> CATA_FORCEINLINE operator()(
        const coord_point_ib<Point, Origin, SourceScale> src ) {
        // Inbounds points are guaranteed to be inbounds after scaling down.
        //
        // They are also guaranteed to be >= 0, so we can use a more effecient method of scaling.
        return coord_point_ib<Point, Origin, ResultScale>::make_unchecked(
                   divide_xy_round_to_minus_infinity_non_negative( src.raw(), ScaleDown ) );
    }
};

template<scale ResultScale, typename Point, origin Origin, scale SourceScale>
CATA_FORCEINLINE auto project_to( const coord_point_ob<Point, Origin, SourceScale> src )
{
    constexpr int scale_down = map_squares_per( ResultScale ) / map_squares_per( SourceScale );
    constexpr int scale_up = map_squares_per( SourceScale ) / map_squares_per( ResultScale );
    return project_to_impl<scale_up, scale_down, ResultScale>()( src );
}

template<scale ResultScale, typename Point, origin Origin, scale SourceScale>
CATA_FORCEINLINE auto project_to( const coord_point_ib<Point, Origin, SourceScale> src )
{
    constexpr int scale_down = map_squares_per( ResultScale ) / map_squares_per( SourceScale );
    constexpr int scale_up = map_squares_per( SourceScale ) / map_squares_per( ResultScale );
    return project_to_impl<scale_up, scale_down, ResultScale>()( src );
}

// Resolves the remainer type for project_remain. In most cases the result shares
// the same InBounds as the source, however some cases let us get inbounds results
// from out of bounds input.
//
// This needs to be special cased, as due to the existance of tinymap we cannot
// do this for origin::reality_bubble
template<typename Point, origin RemainderOrigin, scale SourceScale, bool InBounds>
struct remainder_inbounds {
    using type =
        std::conditional_t<InBounds, coord_point_ib<Point, RemainderOrigin, SourceScale>, coord_point_ob<Point, RemainderOrigin, SourceScale>>;
};

// *_sm_ms are always inbounds as remainder.
template<typename Point, bool InBounds>
struct remainder_inbounds<Point, origin::submap, scale::map_square, InBounds> {
    using type = coord_point_ib<Point, origin::submap, scale::map_square>;
};

template<typename Point, origin RemainderOrigin, scale SourceScale, bool InBounds>
using remainder_inbounds_t = typename
                             remainder_inbounds<Point, RemainderOrigin, SourceScale, InBounds>::type;

template<typename Point, origin QuotientOrigin, scale SourceScale, bool InBounds>
using quotient_type_t =
    std::conditional_t<InBounds, coord_point_ib<Point, QuotientOrigin, SourceScale>, coord_point_ob<Point, QuotientOrigin, SourceScale>>;

template<origin Origin, scale CoarseScale, scale FineScale, bool InBounds>
struct quotient_remainder_helper {
    constexpr static origin RemainderOrigin = origin_from_scale( CoarseScale );
    using quotient_type = quotient_type_t<point, Origin, CoarseScale, InBounds>;
    using quotient_type_tripoint = quotient_type_t<tripoint, Origin, CoarseScale, InBounds>;
    using quotient_type_ob = quotient_type_t<point, Origin, CoarseScale, false>;
    using quotient_type_tripoint_ob = quotient_type_t<tripoint, Origin, CoarseScale, false>;
    using remainder_type = remainder_inbounds_t<point, RemainderOrigin, FineScale, InBounds>;
    using remainder_type_tripoint =
        remainder_inbounds_t<tripoint, RemainderOrigin, FineScale, InBounds>;
    using remainder_type_ob = remainder_inbounds_t<point, RemainderOrigin, FineScale, false>;
    using remainder_type_tripoint_ob =
        remainder_inbounds_t<tripoint, RemainderOrigin, FineScale, false>;
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
    using quotient_type_ob = typename helper::quotient_type_ob;
    using quotient_type_tripoint_ob = typename helper::quotient_type_tripoint_ob;
    using remainder_type = typename helper::remainder_type;
    using remainder_type_tripoint = typename helper::remainder_type_tripoint;
    using remainder_type_ob = typename helper::remainder_type_ob;
    using remainder_type_tripoint_ob = typename helper::remainder_type_tripoint_ob;

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

    // Additional conversions that allow converting _ib results to _ob outputs.
    // The template trickery is to avoid ambiguous overloads

    template < typename quotient_tripoint_ob = quotient_type_tripoint_ob,
               std::enable_if_t < !std::is_same_v<quotient_type_tripoint, quotient_tripoint_ob >> * = nullptr >
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator std::tuple<quotient_type_tripoint_ob &, remainder_type &>() {
        return std::tie( static_cast<quotient_type_tripoint_ob &>( quotient_tripoint ), remainder );
    }
    template < typename quotient_ob = quotient_type_ob,
               std::enable_if_t < !std::is_same_v<quotient_type, quotient_ob >> * = nullptr >
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator std::tuple<quotient_ob &, remainder_type_tripoint &>() {
        return std::tie( static_cast<quotient_ob &>( quotient ), remainder_tripoint );
    }

    template < typename remainder_ob = remainder_type_ob,
               std::enable_if_t < !std::is_same_v<remainder_type, remainder_ob >> * = nullptr >
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator std::tuple<quotient_type_tripoint &, remainder_ob &>() {
        return std::tie( quotient_tripoint, static_cast<remainder_ob &>( remainder ) );
    }
    template < typename remainder_tripoint_ob = remainder_type_tripoint_ob,
               std::enable_if_t < !std::is_same_v<remainder_type_tripoint, remainder_tripoint_ob >> * = nullptr >
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator std::tuple<quotient_type &, remainder_tripoint_ob &>() {
        return std::tie( quotient, static_cast<remainder_tripoint_ob &>( remainder_tripoint ) );
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
template<scale ResultScale, origin Origin, scale SourceScale, template<class, origin, scale> class coord, bool InBounds = coord<point, Origin, SourceScale>::is_inbounds>
CATA_FORCEINLINE quotient_remainder_point<Origin, ResultScale, SourceScale, InBounds>
project_remain(
    const coord<point, Origin, SourceScale> src )
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

template<scale ResultScale, origin Origin, scale SourceScale, template<class, origin, scale> class coord, bool InBounds = coord<tripoint, Origin, SourceScale>::is_inbounds>
CATA_FORCEINLINE quotient_remainder_tripoint<Origin, ResultScale, SourceScale, InBounds>
project_remain(
    const coord<tripoint, Origin, SourceScale> src )
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
         origin FineOrigin, scale FineScale>
inline auto project_combine(
    const coord_point_ob<PointL, CoarseOrigin, CoarseScale> &coarse,
    const coord_point_ob<PointR, FineOrigin, FineScale> &fine )
{
    static_assert( origin_from_scale( CoarseScale ) == FineOrigin,
                   "given point types are not compatible for combination" );
    static_assert( PointL::dimension != 3 || PointR::dimension != 3,
                   "two tripoints should not be combined; it's unclear how to handle z" );
    using PointResult = decltype( PointL() + PointR() );
    const coord_point_ob<PointL, CoarseOrigin, FineScale> refined_coarse =
        project_to<FineScale>( coarse );
    return coord_point < PointResult, CoarseOrigin, FineScale >::make_unchecked(
               refined_coarse.raw() + fine.raw() );
}

template<typename PointL, typename PointR, origin CoarseOrigin, scale CoarseScale,
         origin FineOrigin, scale FineScale>
inline auto project_combine(
    const coord_point_ib<PointL, CoarseOrigin, CoarseScale> &coarse,
    const coord_point_ib<PointR, FineOrigin, FineScale> &fine )
{
    using PointResult = decltype( PointL() + PointR() );
    return coord_point_ib<PointResult, CoarseOrigin, FineScale>::make_unchecked( project_combine(
                coarse.as_ob(), fine.as_ob() ).raw() );
}

template<scale FineScale, origin Origin, scale CoarseScale>
inline auto project_bounds( const coord_point_ob<point, Origin, CoarseScale> &coarse )
{
    constexpr point one( 1, 1 ); // NOLINT(cata-use-named-point-constants)
    return half_open_rectangle<coord_point_ob<point, Origin, FineScale>>(
               project_to<FineScale>( coarse ), project_to<FineScale>( coarse + one ) );
}

template<scale FineScale, origin Origin, scale CoarseScale>
inline auto project_bounds( const coord_point_ob<tripoint, Origin, CoarseScale> &coarse )
{
    constexpr point one( 1, 1 ); // NOLINT(cata-use-named-point-constants)
    return half_open_cuboid<coord_point_ob<tripoint, Origin, FineScale>>(
               project_to<FineScale>( coarse ), project_to<FineScale>( coarse + one ) );
}

} // namespace coords

template<typename Point, coords::origin Origin, coords::scale Scale>
// NOLINTNEXTLINE(cert-dcl58-cpp)
struct std::hash<coords::coord_point_ob<Point, Origin, Scale>> {
    std::size_t operator()( const coords::coord_point_ob<Point, Origin, Scale> &p ) const {
        const hash<Point> h{};
        return h( p.raw() );
    }
};

using coords::project_to;
using coords::project_remain;
using coords::project_combine;
using coords::project_bounds;

// Rebase relative coordinates to the base you know they're actually relative to.
point_rel_ms rebase_rel( point_sm_ms );
point_rel_ms rebase_rel( point_omt_ms p );
point_rel_ms rebase_rel( point_bub_ms p );
point_rel_sm rebase_rel( point_bub_sm p );
point_sm_ms rebase_sm( point_rel_ms p );
point_omt_ms rebase_omt( point_rel_ms p );
point_bub_ms rebase_bub( point_rel_ms p );
point_bub_sm rebase_bub( point_rel_sm p );

tripoint_rel_ms rebase_rel( tripoint_sm_ms p );
tripoint_rel_ms rebase_rel( tripoint_omt_ms p );
tripoint_rel_ms rebase_rel( tripoint_bub_ms p );
tripoint_rel_sm rebase_rel( tripoint_bub_sm p );
tripoint_sm_ms rebase_sm( tripoint_rel_ms p );
tripoint_omt_ms rebase_omt( tripoint_rel_ms p );
tripoint_bub_ms rebase_bub( tripoint_rel_ms p );
tripoint_bub_sm rebase_bub( tripoint_rel_sm p );

// 'Glue' rebase operations for when a tinymap is using the underlying map operation and when a tinymap
// has to be cast to a map to access common functionality. Note that this doesn't actually change anything
// as the reference remains the same location regardless, and the map operation still knows how large the map is.
point_bub_ms rebase_bub( point_omt_ms p );
tripoint_bub_ms rebase_bub( tripoint_omt_ms p );
tripoint_omt_ms rebase_omt( tripoint_bub_ms p );

template<typename Point, coords::origin Origin, coords::scale Scale>
inline int square_dist( const coords::coord_point_ob<Point, Origin, Scale> &loc1,
                        const coords::coord_point_ob<Point, Origin, Scale> &loc2 )
{
    return square_dist( loc1.raw(), loc2.raw() );
}

template<typename Point, coords::origin Origin, coords::scale Scale>
inline int trig_dist( const coords::coord_point_ob<Point, Origin, Scale> &loc1,
                      const coords::coord_point_ob<Point, Origin, Scale> &loc2 )
{
    return trig_dist( loc1.raw(), loc2.raw() );
}

template<typename Point, coords::origin Origin, coords::scale Scale>
inline int rl_dist( const coords::coord_point_ob<Point, Origin, Scale> &loc1,
                    const coords::coord_point_ob<Point, Origin, Scale> &loc2 )
{
    return rl_dist( loc1.raw(), loc2.raw() );
}

template<typename Point, coords::origin Origin, coords::scale Scale>
inline int rl_dist_exact( const coords::coord_point_ob<Point, Origin, Scale> &loc1,
                          const coords::coord_point_ob<Point, Origin, Scale> &loc2 )
{
    return rl_dist_exact( loc1.raw(), loc2.raw() );
}

template<typename Point, coords::origin Origin, coords::scale Scale>
inline int manhattan_dist( const coords::coord_point_ob<Point, Origin, Scale> &loc1,
                           const coords::coord_point_ob<Point, Origin, Scale> &loc2 )
{
    return manhattan_dist( loc1.raw(), loc2.raw() );
}

template<typename Point, coords::origin Origin, coords::scale Scale>
inline int octile_dist( const coords::coord_point_ob<Point, Origin, Scale> &loc1,
                        const coords::coord_point_ob<Point, Origin, Scale> &loc2, int multiplier = 1 )
{
    return octile_dist( loc1.raw(), loc2.raw(), multiplier );
}

template<typename Point, coords::origin Origin, coords::scale Scale>
direction direction_from( const coords::coord_point_ob<Point, Origin, Scale> &loc1,
                          const coords::coord_point_ob<Point, Origin, Scale> &loc2 )
{
    return direction_from( loc1.raw(), loc2.raw() );
}

// line from loc1 to loc2, including loc2 but not loc1
template<typename Point, coords::origin Origin, coords::scale Scale, std::enable_if_t<std::is_same_v<Point, point>, int> = 0>
std::vector < coords::coord_point < Point, Origin, Scale >>
        line_to( const coords::coord_point_ob<Point, Origin, Scale> &loc1,
                 const coords::coord_point_ob<Point, Origin, Scale> &loc2,
                 const int t = 0 )
{
    std::vector<Point> raw_result = line_to( loc1.raw(), loc2.raw(), t );
    std::vector < coords::coord_point < Point, Origin, Scale >> result;
    std::transform( raw_result.begin(), raw_result.end(), std::back_inserter( result ),
    []( const Point & p ) {
        return coords::coord_point < Point, Origin, Scale >::make_unchecked( p );
    } );
    return result;
}

// line from loc1 to loc2, including loc2 but not loc1
template<typename Point, coords::origin Origin, coords::scale Scale, std::enable_if_t<std::is_same_v<Point, point>, int> = 0>
std::vector < coords::coord_point_ib < Point, Origin, Scale >>
        line_to( const coords::coord_point_ib<Point, Origin, Scale> &loc1,
                 const coords::coord_point_ib<Point, Origin, Scale> &loc2,
                 const int t = 0 )
{
    std::vector<Point> raw_result = line_to( loc1.raw(), loc2.raw(), t );
    std::vector < coords::coord_point_ib < Point, Origin, Scale >> result;
    std::transform( raw_result.begin(), raw_result.end(), std::back_inserter( result ),
    []( const Point & p ) {
        return coords::coord_point_ib < Point, Origin, Scale >::make_unchecked( p );
    } );
    return result;
}

// line from loc1 to loc2, including loc2 but not loc1
template<typename Tripoint, coords::origin Origin, coords::scale Scale,
         std::enable_if_t<std::is_same_v<Tripoint, tripoint>, int> = 0>
std::vector < coords::coord_point < Tripoint, Origin, Scale >>
        line_to( const coords::coord_point_ob<Tripoint, Origin, Scale> &loc1,
                 const coords::coord_point_ob<Tripoint, Origin, Scale> &loc2,
                 const int t = 0, const int t2 = 0 )
{
    std::vector<Tripoint> raw_result = line_to( loc1.raw(), loc2.raw(), t, t2 );
    std::vector < coords::coord_point < Tripoint, Origin, Scale>> result;
    std::transform( raw_result.begin(), raw_result.end(), std::back_inserter( result ),
    []( const Tripoint & p ) {
        return coords::coord_point < Tripoint, Origin, Scale >::make_unchecked( p );
    } );
    return result;
}

// line from loc1 to loc2, including loc2 but not loc1
template<typename Tripoint, coords::origin Origin, coords::scale Scale,
         std::enable_if_t<std::is_same_v<Tripoint, tripoint>, int> = 0>
std::vector < coords::coord_point_ib < Tripoint, Origin, Scale>>
        line_to( const coords::coord_point_ib<Tripoint, Origin, Scale> &loc1,
                 const coords::coord_point_ib<Tripoint, Origin, Scale> &loc2,
                 const int t = 0, const int t2 = 0 )
{
    std::vector<Tripoint> raw_result = line_to( loc1.raw(), loc2.raw(), t, t2 );
    std::vector < coords::coord_point_ib < Tripoint, Origin, Scale >> result;
    std::transform( raw_result.begin(), raw_result.end(), std::back_inserter( result ),
    []( const Tripoint & p ) {
        return coords::coord_point_ib < Tripoint, Origin, Scale >::make_unchecked( p );
    } );
    return result;
}


template<typename Point, coords::origin Origin, coords::scale Scale>
coords::coord_point < Point, Origin, Scale >
midpoint( const coords::coord_point_ob<Point, Origin, Scale> &loc1,
          const coords::coord_point_ob<Point, Origin, Scale> &loc2 )
{
    return coords::coord_point < Point, Origin, Scale >::make_unchecked( (
                loc1.raw() + loc2.raw() ) / 2 );
}

template<typename Point, coords::origin Origin, coords::scale Scale>
coords::coord_point_ib < Point, Origin, Scale >
midpoint( const coords::coord_point_ib<Point, Origin, Scale> &loc1,
          const coords::coord_point_ib<Point, Origin, Scale> &loc2 )
{
    return coords::coord_point_ib < Point, Origin, Scale >::make_unchecked( (
                loc1.raw() + loc2.raw() ) / 2 );
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
coords::coord_point<Point, Origin, Scale>
midpoint_round_to_nearest( std::vector<coords::coord_point_ob<Point, Origin, Scale>> &locs )
{
    tripoint mid;
    for( const auto &loc : locs ) {
        mid += loc.raw();
    }

    float num = locs.size();
    mid.x = std::round( mid.x / num );
    mid.y = std::round( mid.y / num );
    mid.z = std::round( mid.z / num );

    return coords::coord_point_ib < Point, Origin, Scale >::make_unchecked( mid );
}

template<typename Point, coords::origin Origin, coords::scale Scale>
std::vector<coords::coord_point_ob<Point, Origin, Scale>>
        closest_points_first( const coords::coord_point_ob<Point, Origin, Scale> &loc,
                              int min_dist, int max_dist )
{
    std::vector<Point> raw_result = closest_points_first( loc.raw(), min_dist, max_dist );
    std::vector<coords::coord_point_ob<Point, Origin, Scale>> result;
    result.reserve( raw_result.size() );
    std::transform( raw_result.begin(), raw_result.end(), std::back_inserter( result ),
    []( const Point & p ) {
        return coords::coord_point_ob<Point, Origin, Scale>( p );
    } );
    return result;
}
template<typename Point, coords::origin Origin, coords::scale Scale>
std::vector<coords::coord_point_ob<Point, Origin, Scale>>
        closest_points_first( const coords::coord_point_ob<Point, Origin, Scale> &loc,
                              int max_dist )
{
    return closest_points_first( loc, 0, max_dist );
}

template<typename Point, coords::origin Origin, coords::scale Scale>
std::vector<coords::coord_point_ib<Point, Origin, Scale>>
        closest_points_first( const coords::coord_point_ib<Point, Origin, Scale> &loc,
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
    void fromomap( const point &rel_om, const point &rel_om_pos );

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

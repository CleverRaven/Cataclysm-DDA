#include <algorithm>
#include <array>
#include <cstdlib>
#include <tuple>

#include "cuboid_rectangle.h"
#include "level_cache.h"
#include "point.h"
#include "reachability_cache.h"

//helper functions
static constexpr half_open_rectangle<point> bounding_rect( point_zero, {MAPSIZE_X, MAPSIZE_Y} );

// needs to be explicitly defined to avoid linker errors
// see https://stackoverflow.com/questions/8452952/c-linker-error-with-class-static-constexpr
static constexpr int MAX_D = reachability_cache_layer::MAX_D;

bool reachability_cache_layer::update( const point &p, reachability_cache_layer::ElType value )
{
    bool change = ( *this )[ p ] != value;
    ( *this )[ p ] = value;
    return change;
}

inline reachability_cache_layer::ElType &reachability_cache_layer::operator[]( const point &p )
{
    return cache[p.x][p.y];
}
inline const reachability_cache_layer::ElType &reachability_cache_layer::operator[](
    const point &p ) const
{
    return cache[p.x][p.y];
}

inline reachability_cache_layer::ElType
reachability_cache_layer::get_or( const point &p, reachability_cache_layer::ElType def ) const
{
    return bounding_rect.contains( p ) ? ( *this )[ p ] : def;
}

template<bool Horizontal, typename... Types>
void reachability_cache<Horizontal, Types...>::invalidate()
{
    dirty_any = true;
    dirty.set();
}

template<bool Horizontal, typename... Types>
void reachability_cache<Horizontal, Types...>::invalidate( const point &p )
{
    dirty_any = true;
    dirty[dirty_idx( p )] = true;
}

template<bool Horizontal, typename... Types>
void reachability_cache<Horizontal, Types...>::rebuild( Q q,
        const Types &... params )
{
    static_assert( MAPSIZE_Y == SEEY * MAPSIZE, "reachability cache uses outdated map dimensions" );
    static_assert( MAPSIZE_X == SEEX * MAPSIZE, "reachability cache uses outdated map dimensions" );

    // start is inclusive, end is exclusive (1 step outside of the range)
    point dir;
    point start;
    point end;

    if( q == Q::SW || q == Q::NW ) {
        std::tie( dir.x, start.x, end.x ) = std::make_tuple( 1, 0, MAPSIZE_X );
    } else {
        std::tie( dir.x, start.x, end.x ) = std::make_tuple( -1, MAPSIZE_X - 1, -1 );
    }
    if( q == Q::NE || q == Q::NW ) {
        std::tie( dir.y, start.y, end.y ) = std::make_tuple( 1, 0, MAPSIZE_Y );
    } else {
        std::tie( dir.y, start.y, end.y ) = std::make_tuple( -1, MAPSIZE_Y - 1, -1 );
    }

    point sm_dir = {dir.x * SEEX, dir.y * SEEY};
    reachability_cache_layer &layer = ( *this )[q];
    // submaps processing can mark adjacent submaps as "dirty", but that shouldn't affect other quadrants
    std::bitset<MAPSIZE *MAPSIZE> dirty_local;

    // Traverse the submaps in order
    // smx and smy denote the "starting" corner or the submap (in map coords)
    for( int smx = start.x; smx != end.x; smx += sm_dir.x ) {
        int dirty_shift = ( smx / SEEX ) * MAPSIZE + ( start.y / SEEY );
        for( int smy = start.y; smy != end.y; smy += sm_dir.y ) {
            // if this submap is not marked as changed (externally or by neighbor submaps processing),
            // skip it
            if( !dirty[dirty_shift] && !dirty_local[dirty_shift] ) {
                dirty_shift += dir.y;
                continue;
            }

            const point sm_p( smx, smy );
            point cur_sm_end = sm_p + sm_dir;
            bool last_change = false;
            bool next_x_dirty = false;
            bool next_y_dirty = false;
            int sm_last_x = cur_sm_end.x - dir.x;
            for( int x = smx; x != cur_sm_end.x; x += dir.x ) {
                for( int y = start.y; y != cur_sm_end.y; y += dir.y ) {
                    last_change = Spec::dynamic_fun( layer, {x, y}, dir, params ... );
                    next_x_dirty |= ( x == sm_last_x ) & last_change;
                }
                next_y_dirty |= last_change;
            }

            // after the cycle, last_change holds the status of the "last corner" change,
            // it's the only thing that can affect dirty state of the diagonally adjacent submap.
            // Checking if diagonal neighbor is within bounds:
            if( last_change && smx + sm_dir.x != end.x  && smy + sm_dir.y != end.y ) {
                // can save some multiplications by reusing dirty_shift, but this way it's more readable
                dirty_local[dirty_idx( sm_p + sm_dir )] = true;
            }
            // "next" submap in y direction
            if( next_y_dirty && smy + sm_dir.y != end.y ) {
                dirty_local[dirty_idx( sm_p + point( 0, sm_dir.y ) )] = true;
            }
            // "next" submap in x direction
            if( next_x_dirty && smx + sm_dir.x != end.x ) {
                dirty_local[dirty_idx( sm_p + point( sm_dir.x, 0 ) )] = true;
            }

            dirty_shift += dir.y;
        }
    }

}

template<bool Horizontal, typename... Types>
void reachability_cache<Horizontal, Types...>::rebuild( const Types &... params )
{
    for( int q = 0; q < enum_traits<Q>::size; q++ ) {
        rebuild( static_cast<Q>( q ), params... );
    }
}

template<bool Horizontal, typename... Types>
int reachability_cache<Horizontal, Types...>::get_value( Q quad, const point &p ) const
{
    return ( *this )[ quad ][ p ];
}

template<bool Horizontal, typename... Types>
inline reachability_cache_layer &reachability_cache<Horizontal, Types...>::operator[](
    const Q &quad )
{
    return layers[static_cast<int>( quad )];
}

template<bool Horizontal, typename... Params>
inline const reachability_cache_layer &
reachability_cache<Horizontal, Params...>::operator[]( const reachability_cache::Q &quad ) const
{
    return layers[static_cast<int>( quad )];
}

template<bool Horizontal, typename... Types>
bool reachability_cache<Horizontal, Types...>::has_potential_los(
    const point &from, const point &to, const Types &... params )
{
    if( dirty_any ) {
        if( Spec::source_cache_dirty( params ... ) ) {
            // this is specifically to fix the issue with the debug overlay
            // calling this method before transparency cache was rebuild after map shift
            return true;
        }
        rebuild( params ... );
        dirty.reset();
        dirty_any = false;
    }

    point dp = ( to - from );
    int d = std::abs( dp.x ) + std::abs( dp.y );

    bool south = to.y > from.y;
    bool west = to.x < from.x;

    Q quad = enum_traits<Q>::quadrant( south, west );
    return Spec::test( d, ( *this )[quad][from] );
}

template<bool Horizontal, typename... Params>
inline int reachability_cache<Horizontal, Params...>::dirty_idx( const point &p )
{
    return ( p.x / SEEX ) * MAPSIZE + p.y / SEEY;
}

static bool transp( const level_cache &lc, const point &p )
{
    return lc.transparent_cache_wo_fields[p.x][p.y];
}

static int max3( int arg,  int arg2, int arg3 )
{
    return std::max( std::max( arg, arg2 ), arg3 );
}

static int min4( int arg,  int arg2, int arg3, int arg4 )
{
    return std::min( std::min( arg, arg2 ), std::min( arg3, arg4 ) );
}

// DP function for the "horizontal" cache
// el = 0;  if not transparent, or else:
// el = max( horizontal_neighbor + 1, vertical_neighbor + 1, diagonal_neighbor + 2)
bool reachability_cache_specialization<true, level_cache>::dynamic_fun(
    reachability_cache_layer &layer, const point &p, const point &dir, const level_cache &this_lc )
{
    using Layer = reachability_cache_layer;

    // if point is transparent, returns given prev_value, otherwise returns zero
    const auto transp_or_zero = [&]( const point & p, int prev_value ) {
        return prev_value != 0 && !transp( this_lc, p ) ? 0 : prev_value;
    };

    const Layer::ElType v =
        std::min(
            MAX_D,
            max3(
                transp_or_zero( p - dir, layer.get_or( p - dir, 0 ) ) + 2,
                transp_or_zero( p - point( dir.x, 0 ), layer.get_or( p - point( dir.x, 0 ), 0 ) ) + 1,
                transp_or_zero( p - point( 0, dir.y ), layer.get_or( p - point( 0, dir.y ), 0 ) ) + 1
            ) );

    return layer.update( p, v );
}

// DP function for the "vertical" cache
// el = 0;  if this tile doesn't have floor/roof
// el = MAX_D; if tile is not transparent, else:
// el = min( horizontal_neighbor + 1, vertical_neighbor + 1, diagonal_neighbor + 2)
bool reachability_cache_specialization<false, level_cache, level_cache>::dynamic_fun(
    reachability_cache_layer &layer,
    const point &p, const point &dir,
    const level_cache &this_lc,
    const level_cache &floor_lc )
{
    using Layer = reachability_cache_layer;

    if( !floor_lc.floor_cache[p.x][p.y] && transp( this_lc, p ) ) {
        return layer.update( p, 0 );
    }

    const auto val_if_transp = [&]( const point & p, int prev_v ) {
        return prev_v >= MAX_D || !transp( this_lc, p ) ? MAX_D : prev_v;
    };

    const Layer::ElType v =
        min4(
            MAX_D,
            val_if_transp( p - dir, layer.get_or( p - dir, MAX_D ) ) + 2,
            val_if_transp( p - point( dir.x, 0 ), layer.get_or( p - point( dir.x, 0 ), MAX_D ) ) + 1,
            val_if_transp( p - point( 0, dir.y ), layer.get_or( p - point( 0, dir.y ), MAX_D ) ) + 1
        );

    return layer.update( p, v );
}

// horizontal cache test
bool reachability_cache_specialization<true, level_cache>::test( int d, int cache_v )
{
    // for a transparent tile cache value is "1",
    // if tile has direct transparent neighbor, it's value will be "2"
    // d = |dx| + |dy| for such neighbor will be "1"
    // so if d equals or exceeds cache value, it's a "no"
    return d <= cache_v;
}

bool reachability_cache_specialization<false, level_cache, level_cache>::source_cache_dirty(
    const level_cache &this_lc, const level_cache &floor_lc )
{
    return floor_lc.floor_cache_dirty || this_lc.transparency_cache_dirty.any();
}

bool reachability_cache_specialization<true, level_cache>::source_cache_dirty(
    const level_cache &this_lc )
{
    return this_lc.transparency_cache_dirty.any();
}

// vertical cache test
bool reachability_cache_specialization<false,  level_cache, level_cache>::test( int d,
        int cache_v )
{
    // cache value directly below the absent floor is 0
    // direct neighbor will have value of 1, distance is also 1
    // so only if d is strictly smaller than cache value (which is distance to open floor), it's a "no"
    return d < MAX_D && d >= cache_v;
}

// avoid linker errors
template class reachability_cache<true, level_cache>;
template class reachability_cache<false, level_cache, level_cache>;

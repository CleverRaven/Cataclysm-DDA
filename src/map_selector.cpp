#include "map_selector.h"

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "map.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "rng.h"

class game;

// NOLINTNEXTLINE(cata-static-declarations)
extern std::unique_ptr<game> g;

map_selector::map_selector( const tripoint_bub_ms &pos, int radius, bool accessible )
{
    for( const tripoint_bub_ms &e : closest_points_first( pos, radius ) ) {
        if( !accessible || get_map().clear_path( pos, e, radius, 1, 100 ) ) {
            data.emplace_back( e );
        }
    }
}

tripoint_range<tripoint_bub_ms> points_in_range_bub( const map &m )
{
    return tripoint_range<tripoint_bub_ms>(
               tripoint_bub_ms( 0, 0, -OVERMAP_DEPTH ),
               tripoint_bub_ms( SEEX * m.getmapsize() - 1, SEEY * m.getmapsize() - 1, OVERMAP_HEIGHT ) );
}

tripoint_range<tripoint_bub_ms> points_in_level_range( const map &m, const int z )
{
    return tripoint_range<tripoint_bub_ms>(
               tripoint_bub_ms( 0, 0, z ),
               tripoint_bub_ms( SEEX * m.getmapsize() - 1, SEEY * m.getmapsize() - 1, z ) );
}

std::optional<tripoint_bub_ms> random_point( const map &m,
        const std::function<bool( const tripoint_bub_ms & )> &predicate )
{
    return random_point( points_in_range_bub( m ), predicate );
}

std::optional<tripoint_bub_ms> random_point_on_level( const map &m, const int z,
        const std::function<bool( const tripoint_bub_ms & )> &predicate )
{
    return random_point( points_in_level_range( m, z ), predicate );
}

std::optional<tripoint_bub_ms> random_point( const tripoint_range<tripoint_bub_ms> &range,
        const std::function<bool( const tripoint_bub_ms & )> &predicate )
{
    // Optimist approach: just assume there are plenty of suitable places and a randomly
    // chosen point will have a good chance to hit one of them.
    // If there are only few suitable places, we have to find them all, otherwise this loop may never finish.
    for( int tries = 0; tries < 10; ++tries ) {
        const tripoint_bub_ms p( rng( range.min().x(), range.max().x() ), rng( range.min().y(),
                                 range.max().y() ),
                                 rng( range.min().z(), range.max().z() ) );
        if( predicate( p ) ) {
            return p;
        }
    }
    std::vector<tripoint_bub_ms> suitable;
    for( const tripoint_bub_ms &p : range ) {
        if( predicate( p ) ) {
            suitable.push_back( p );
        }
    }
    if( suitable.empty() ) {
        return {};
    }
    return random_entry( suitable );
}

map_cursor::map_cursor( const tripoint_bub_ms &pos ) : pos_abs_( g ? get_map().get_abs(
                pos ) : tripoint_abs_ms::zero ), pos_bub_( g ? tripoint_bub_ms::zero : pos ) { }

map_cursor::map_cursor( map *here, const tripoint_bub_ms &pos ) : pos_abs_( g ? here->get_abs(
                pos ) : tripoint_abs_ms::zero ), pos_bub_( g ? tripoint_bub_ms::zero : pos )
{
}

map_cursor::map_cursor( const tripoint_abs_ms &pos ) : pos_abs_( pos ),
    pos_bub_( tripoint_bub_ms::zero ) { }

tripoint_bub_ms map_cursor::pos_bub() const
{
    return g ? get_map().get_bub( pos_abs_ ) : pos_bub_;
}

tripoint_bub_ms map_cursor::pos_bub( const map &here ) const
{
    return g ? here.get_bub( pos_abs_ ) : pos_bub_;
}

tripoint_abs_ms map_cursor::pos_abs() const
{
    return g ? pos_abs_ : tripoint_abs_ms::invalid;
}

#include "map_selector.h"

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "game_constants.h"
#include "map.h"
#include "map_iterator.h"
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

tripoint_range<tripoint> points_in_range( const map &m )
{
    return tripoint_range<tripoint>(
               tripoint( 0, 0, -OVERMAP_DEPTH ),
               tripoint( SEEX * m.getmapsize() - 1, SEEY * m.getmapsize() - 1, OVERMAP_HEIGHT ) );
}

tripoint_range<tripoint_bub_ms> points_in_range_bub( const map &m )
{
    return tripoint_range<tripoint_bub_ms>(
               tripoint_bub_ms( 0, 0, -OVERMAP_DEPTH ),
               tripoint_bub_ms( SEEX * m.getmapsize() - 1, SEEY * m.getmapsize() - 1, OVERMAP_HEIGHT ) );
}

std::optional<tripoint> random_point( const map &m,
                                      const std::function<bool( const tripoint & )> &predicate )
{
    return random_point( points_in_range( m ), predicate );
}

std::optional<tripoint_bub_ms> random_point( const map &m,
        const std::function<bool( const tripoint_bub_ms & )> &predicate )
{
    return random_point( points_in_range_bub( m ), predicate );
}

std::optional<tripoint> random_point( const tripoint_range<tripoint> &range,
                                      const std::function<bool( const tripoint & )> &predicate )
{
    // Optimist approach: just assume there are plenty of suitable places and a randomly
    // chosen point will have a good chance to hit one of them.
    // If there are only few suitable places, we have to find them all, otherwise this loop may never finish.
    for( int tries = 0; tries < 10; ++tries ) {
        const tripoint p( rng( range.min().x, range.max().x ), rng( range.min().y, range.max().y ),
                          rng( range.min().z, range.max().z ) );
        if( predicate( p ) ) {
            return p;
        }
    }
    std::vector<tripoint> suitable;
    for( const tripoint &p : range ) {
        if( predicate( p ) ) {
            suitable.push_back( p );
        }
    }
    if( suitable.empty() ) {
        return {};
    }
    return random_entry( suitable );
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

map_cursor::map_cursor( const tripoint_bub_ms &pos ) : pos_abs_( g ? get_map().getglobal(
                pos ) : tripoint_abs_ms( tripoint_zero ) ), pos_bub_( g ? tripoint_bub_ms( tripoint_zero ) : pos ) { }

tripoint_bub_ms map_cursor::pos() const
{
    return g ? get_map().bub_from_abs( pos_abs_ ) : pos_bub_;
}

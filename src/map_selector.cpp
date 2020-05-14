#include "map_selector.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

#include "game.h"
#include "game_constants.h"
#include "map.h"
#include "map_iterator.h"
#include "optional.h"
#include "rng.h"

map_selector::map_selector( const tripoint &pos, int radius, bool accessible )
{
    for( const tripoint &e : closest_tripoints_first( pos, radius ) ) {
        if( !accessible || g->m.clear_path( pos, e, radius, 1, 100 ) ) {
            data.emplace_back( e );
        }
    }
}

tripoint_range points_in_range( const map &m )
{
    const int z = m.get_abs_sub().z;
    const bool hasz = m.has_zlevels();
    return tripoint_range( tripoint( 0, 0, hasz ? -OVERMAP_DEPTH : z ),
                           tripoint( SEEX * m.getmapsize() - 1, SEEY * m.getmapsize() - 1, hasz ? OVERMAP_HEIGHT : z ) );
}

cata::optional<tripoint> random_point( const map &m,
                                       const std::function<bool( const tripoint & )> &predicate )
{
    return random_point( points_in_range( m ), predicate );
}

cata::optional<tripoint> random_point( const tripoint_range &range,
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
    for( const auto &p : range ) {
        if( predicate( p ) ) {
            suitable.push_back( p );
        }
    }
    if( suitable.empty() ) {
        return {};
    }
    return random_entry( suitable );
}

map_cursor::map_cursor( const tripoint &pos ) : pos_( g ? g->m.getabs( pos ) : pos ) { }

map_cursor::operator tripoint() const
{
    return g ? g->m.getlocal( pos_ ) : pos_;
}

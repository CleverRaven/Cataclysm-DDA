#include "vehicle_selector.h"

#include <algorithm>
#include <memory>

#include "game.h"
#include "map.h"
#include "optional.h"
#include "point.h"
#include "vpart_position.h"

vehicle_selector::vehicle_selector( const tripoint &pos, int radius, bool accessible,
                                    bool visibility_only )
{
    for( const tripoint &e : closest_tripoints_first( pos, radius ) ) {
        if( !accessible ||
            ( visibility_only ? g->m.sees( pos, e, radius ) : g->m.clear_path( pos, e, radius, 1, 100 ) ) ) {
            if( const optional_vpart_position vp = g->m.veh_at( e ) ) {
                data.emplace_back( vp->vehicle(), vp->part_index() );
            }
        }
    }
}

vehicle_selector::vehicle_selector( const tripoint &pos, int radius, bool accessible,
                                    const vehicle &ignore )
{
    for( const tripoint &e : closest_tripoints_first( pos, radius ) ) {
        if( !accessible || g->m.clear_path( pos, e, radius, 1, 100 ) ) {
            if( const optional_vpart_position vp = g->m.veh_at( e ) ) {
                if( &vp->vehicle() != &ignore ) {
                    data.emplace_back( vp->vehicle(), vp->part_index() );
                }
            }
        }
    }
}

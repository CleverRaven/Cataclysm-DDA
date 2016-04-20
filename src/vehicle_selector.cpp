#include "vehicle_selector.h"

#include "game.h"
#include "vehicle.h"
#include "map.h"

vehicle_selector::vehicle_selector( const tripoint &pos, int radius, bool accessible )
{
    for( const auto &e : closest_tripoints_first( radius, pos ) ) {
        if( !accessible || g->m.clear_path( pos, e, radius, 1, 100 ) ) {
            int part = -1;
            vehicle *veh = g->m.veh_at( e, part );
            if( veh && part >= 0 ) {
                data.emplace_back( *veh, part );
            }
        }
    }
}

vehicle_selector::vehicle_selector( const tripoint &pos, int radius, bool accessible,
                                    const vehicle &ignore )
{
    for( const auto &e : closest_tripoints_first( radius, pos ) ) {
        if( !accessible || g->m.clear_path( pos, e, radius, 1, 100 ) ) {
            int part = -1;
            vehicle *veh = g->m.veh_at( e, part );
            if( veh && veh != &ignore && part >= 0 ) {
                data.emplace_back( *veh, part );
            }
        }
    }
}

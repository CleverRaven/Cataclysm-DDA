#include "vehicle_selector.h"

#include "game.h"
#include "vehicle.h"
#include "map.h"

vehicle_selector::vehicle_selector( const tripoint& pos, int radius ) {
    for( const auto& e : closest_tripoints_first( radius, pos ) ) {
        int part = -1;
        vehicle *veh = g->m.veh_at( e, part );
        if( veh && part >= 0 ) {
            part = veh->part_with_feature( part, "CARGO" );
            if( part != -1 ) {
                data.emplace_back( *veh, part );
            }
        }
    }
}

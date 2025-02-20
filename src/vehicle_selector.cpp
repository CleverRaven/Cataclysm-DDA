#include "vehicle_selector.h"

#include <optional>

#include "coordinates.h"
#include "map.h"
#include "vpart_position.h"

vehicle_selector::vehicle_selector( map &here, const tripoint_bub_ms &pos, int radius,
                                    bool accessible,
                                    bool visibility_only )
{
    for( const tripoint_bub_ms &e : closest_points_first( pos, radius ) ) {
        if( !accessible ||
            ( visibility_only ? here.sees( pos, e, radius ) : here.clear_path( pos, e, radius, 1, 100 ) ) ) {
            if( const optional_vpart_position vp = here.veh_at( e ) ) {
                data.emplace_back( vp->vehicle(), vp->part_index() );
            }
        }
    }
}

vehicle_selector::vehicle_selector( map &here, const tripoint_bub_ms &pos, int radius,
                                    bool accessible,
                                    const vehicle &ignore )
{
    for( const tripoint_bub_ms &e : closest_points_first( pos, radius ) ) {
        if( !accessible || here.clear_path( pos, e, radius, 1, 100 ) ) {
            if( const optional_vpart_position vp = here.veh_at( e ) ) {
                data.emplace_back( vp->vehicle(), vp->part_index(), &vp->vehicle() == &ignore );
            }
        }
    }
}

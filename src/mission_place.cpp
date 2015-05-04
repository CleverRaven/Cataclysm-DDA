#include "mission.h"
#include "overmapbuffer.h"
#include "overmap.h"

// Input position is in global overmap terrain coordinates!

bool mission_place::near_town( const tripoint &pos_omt )
{
    const auto pos_sm = overmapbuffer::omt_to_sm_copy( pos_omt );
    const auto cref = overmap_buffer.closest_city( pos_sm );
    if( !cref ) {
        return false; // no nearby city at all.
    }
    // distance was calculated in submap coordinates
    return cref.distance / 2 - cref.city->s <= 40;
}

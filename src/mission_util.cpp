#include <memory>
#include <string>
#include <vector>

#include "mission.h"
#include "game.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "enums.h"
#include "player.h"
#include "rng.h"

/**
 * Reveal the cloest overmap terrain of a type and return the its location
 */
tripoint mission_util::reveal_om_ter( const std::string &omter, int reveal_rad, bool must_see,
                                      int target_z )
{
    // Missions are normally on z-level 0, but allow an optional argument.
    tripoint loc = g->u.global_omt_location();
    loc.z = target_z;
    const tripoint place = overmap_buffer.find_closest( loc, omter, 0, must_see );
    if( place != overmap::invalid_tripoint && reveal_rad >= 0 ) {
        overmap_buffer.reveal( place, reveal_rad );
    }
    return place;
}

/**
 * Set target of mission to closest overmap terrain of that type,
 * reveal the area around it (uses reveal with reveal_rad),
 * and returns the mission target.
*/
tripoint mission_util::target_om_ter( const std::string &omter, int reveal_rad, mission *miss,
                                      bool must_see, int target_z )
{
    const tripoint place = reveal_om_ter( omter, reveal_rad, must_see, target_z );
    miss->set_target( place );
    return place;
}

tripoint mission_util::target_om_ter_random( const std::string &omter, int reveal_rad,
        mission *miss,
        bool must_see, int range, tripoint loc )
{
    if( loc == overmap::invalid_tripoint ) {
        loc = g->u.global_omt_location();
    }

    auto places = overmap_buffer.find_all( loc, omter, range, must_see );
    if( places.empty() ) {
        return g->u.global_omt_location();
    }
    const auto loc_om = overmap_buffer.get_existing_om_global( loc );
    std::vector<tripoint> places_om;
    for( auto &i : places ) {
        if( loc_om == overmap_buffer.get_existing_om_global( i ) ) {
            places_om.push_back( i );
        }
    }

    const tripoint place = random_entry( places_om );
    if( reveal_rad >= 0 ) {
        overmap_buffer.reveal( place, reveal_rad );
    }
    miss->set_target( place );
    return place;
}

#include "monster.h"

#include "enums.h"
#include "calendar.h"
#include "senses.h"

tripoint monster::overmap_move( const tripoint &location, const senses &sensor ) {
    // TODO: calculate real overmap sight range,
    // or pass in a value indicating acuity and let the sensor figure it out.
    tripoint target = sensor.nearest_visible_enemy( location, faction, 10 );

    if( target == location ) {
        // TODO: inject scent detection threshold.
        target = sensor.strongest_scent_track( location, 10 );
    }

    if( target == location ) {
        target = recent_stimulus.source;
    }

    // Using target and map info from sensor, determine the next step to take.
    for( const tripoint &candidate : squares_closer_to( location, target ) ) {
        if( sensor.can_move_to( candidate, 0 ) ) {
            return candidate;
        }
    }
    return location;
}

int monster::next_overmap_move() const {
    return next_overmap_move_turn;
}

static int effective_strength( const monster_group_stimulus &stimulus, const tripoint &location ) {
    // TODO: different scaling for effective strength based on stimulus type?
    return stimulus.strength - ( stimulus.occurence - calendar::turn ) -
        rl_dist( location, stimulus.source );
}

// Possible improvement, store a direction to the target instead of a target point,
// for enhanced drifting around when navigating toward it.
void monster::apply_stimulus( const tripoint &location, const monster_group_stimulus &stimulus ) {
    if( effective_strength( stimulus, location ) > effective_strength( recent_stimulus, location ) ) {
        recent_stimulus = stimulus;
    }
}

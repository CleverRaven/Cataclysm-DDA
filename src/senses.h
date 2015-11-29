#ifndef SENSES_H
#define SENSES_H

#include <vector>

enum home_area_type : int;
using mfaction_id = int_id<monfaction>;
enum movement_mode : int;
struct tripoint;

/**
 * Object to inject into monster AI to isolate AI code from map or overmap accesses.
 */
class senses {
public:
    // Zombie types only need a target location, they don't care if they can path there.
    virtual tripoint nearest_visible_enemy(
        const tripoint &location, const mfaction_id &faction, int sightrange ) const = 0;

    // Monsters only ever get adjacent scent strength.
    // Returns current location if scent threshold isn't exceeded.
    virtual tripoint strongest_scent_track( const tripoint &location, int scent_threshold ) const = 0;

    // For smarter monsters, pathing around obstacles.
    virtual std::vector<tripoint> path_to_nearest_visible_enemy(
        const tripoint &location, const mfaction_id &faction, int sightrange, movement_mode moves ) const = 0;

    // Nearby area the monster finds attractive.
    virtual tripoint nearby_home_area( const tripoint &location, int sightrange, home_area_type ) const = 0;

    virtual bool can_move_to( const tripoint &location, int movement_mode ) const = 0;
};

#endif

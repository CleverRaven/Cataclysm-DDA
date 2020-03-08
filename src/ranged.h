#ifndef RANGED_H
#define RANGED_H

#include <vector>
#include "type_id.h"

class item;
class player;
class spell;
struct itype;
struct tripoint;

enum target_mode : int {
    TARGET_MODE_FIRE,
    TARGET_MODE_THROW,
    TARGET_MODE_TURRET,
    TARGET_MODE_TURRET_MANUAL,
    TARGET_MODE_REACH,
    TARGET_MODE_THROW_BLIND,
    TARGET_MODE_SPELL
};

/** Stores data for aiming the player's weapon across turns */
struct targeting_data {
    item *relevant;
    int range;
    int power_cost;
    bool held;
    const itype *ammo;
};

class target_handler
{
        // TODO: alias return type of target_ui
    public:
        /**
         *  Prompts for target and returns trajectory to it.
         *  @param pc The player doing the targeting
         *  @param mode targeting mode, which affects UI display among other things.
         *  @param relevant active item, if any (for instance, a weapon to be aimed).
         *  @param range the maximum distance to which we're allowed to draw a target.
         *  @param ammo effective ammo data (derived from @param relevant if unspecified).
         */
        std::vector<tripoint> target_ui( player &pc, target_mode mode,
                                         item *relevant, int range,
                                         const itype *ammo = nullptr );
        // magic version of target_ui
        std::vector<tripoint> target_ui( spell_id sp, bool no_fail = false,
                                         bool no_mana = false );
        std::vector<tripoint> target_ui( spell &casting, bool no_fail = false,
                                         bool no_mana = false );
};

int range_with_even_chance_of_good_hit( int dispersion );

#endif // RANGED_H

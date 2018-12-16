#ifndef RANGED_H
#define RANGED_H

#include <functional>
#include <vector>

class item;
class player;
struct itype;
struct tripoint;

/**
 * Targeting UI callback is passed the item being targeted (if any)
 * and should return pointer to effective ammo data (if any)
 */
using target_callback = std::function<const itype *( item *obj )>;
using firing_callback = std::function<void( const int )>;

enum target_mode : int {
    TARGET_MODE_FIRE,
    TARGET_MODE_THROW,
    TARGET_MODE_TURRET,
    TARGET_MODE_TURRET_MANUAL,
    TARGET_MODE_REACH,
    TARGET_MODE_THROW_BLIND
};

// @todo: move callbacks to a new struct and define some constructors for ease of use
struct targeting_data {
    target_mode mode;
    item *relevant;
    int range;
    int power_cost;
    bool held;
    const itype *ammo;
    target_callback on_mode_change;
    target_callback on_ammo_change;
    firing_callback pre_fire;
    firing_callback post_fire;
};

class target_handler
{
        // @todo: alias return type of target_ui
    public:
        /**
         *  Prompts for target and returns trajectory to it.
         *  @param pc The player doing the targeting
         *  @param args structure containing arguments passed to the overloaded form.
         */
        std::vector<tripoint> target_ui( player &pc, const targeting_data &args );
        /**
         *  Prompts for target and returns trajectory to it.
         *  @param pc The player doing the targeting
         *  @param mode targeting mode, which affects UI display among other things.
         *  @param relevant active item, if any (for instance, a weapon to be aimed).
         *  @param range the maximum distance to which we're allowed to draw a target.
         *  @param ammo effective ammo data (derived from @param relevant if unspecified).
         *  @param on_mode_change callback when user attempts changing firing mode.
         *  @param on_ammo_change callback when user attempts changing ammo.
         */
        std::vector<tripoint> target_ui( player &pc, target_mode mode,
                                         item *relevant, int range,
                                         const itype *ammo = nullptr,
                                         const target_callback &on_mode_change = target_callback(),
                                         const target_callback &on_ammo_change = target_callback() );
};

int range_with_even_chance_of_good_hit( int dispersion );

#endif // RANGED_H

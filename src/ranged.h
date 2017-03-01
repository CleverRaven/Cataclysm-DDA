#ifndef RANGED_H
#define RANGED_H

#include <functional>

class item;
class player;
struct itype;

/**
 * Targeting UI callback is passed the item being targeted (if any)
 * and should return pointer to effective ammo data (if any)
 */
using target_callback = std::function<const itype *(item *obj)>;
using firing_callback = std::function<void( const int )>;

enum target_mode {
    TARGET_MODE_FIRE,
    TARGET_MODE_THROW,
    TARGET_MODE_TURRET,
    TARGET_MODE_TURRET_MANUAL,
    TARGET_MODE_REACH
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

#endif // RANGED_H

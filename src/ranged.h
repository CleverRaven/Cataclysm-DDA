#ifndef RANGED_H
#define RANGED_H

#include <functional>

class item;
class itype;

/**
 * Targeting UI callback is passed the item being targeted (if any)
 * and should return pointer to effective ammo data (if any)
 */
using target_callback = std::function<const itype *(item *obj)>;

enum target_mode {
    TARGET_MODE_FIRE,
    TARGET_MODE_THROW,
    TARGET_MODE_TURRET,
    TARGET_MODE_TURRET_MANUAL,
    TARGET_MODE_REACH
};

struct targeting_data {
    target_mode mode;
    item *relevant;
    int range;
    int power_cost;
    bool held;
    const itype *ammo;
    target_callback on_mode_change;
    target_callback on_ammo_change;
};

#endif // RANGED_H

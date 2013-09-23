#ifndef _VEH_TYPE_H_
#define _VEH_TYPE_H_

#include "color.h"
#include "itype.h"

/* Flag info:
 * INTERNAL - Can be mounted inside other parts
 * ANCHOR_POINT - Allows secure seatbelt attachment
 * OVER - Can be mounted over other parts
 * VARIABLE_SIZE - Has 'bigness' for power, wheel radius, etc
 * Other flags are self-explanatory in their names. */
struct vpart_info
{
    std::string id;         // unique identifier for this part
    std::string name;       // part name, user-visible
    long sym;               // symbol of part as if it's looking north
    nc_color color;         // color
    char sym_broken;        // symbol of broken part as if it's looking north
    nc_color color_broken;  // color of broken part
    int dmg_mod;            // damage modifier, percent
    int durability;         // durability
    union
    {
        int par1;
        int power;      // engine (top spd), solar panel (% of 1 fuel per turn, can be > 100)
        int size;       // fuel tank, trunk
        int wheel_width;// wheel width in inches. car could be 9, bicycle could be 2.
        int bonus;      // seatbelt (str), muffler (%), horn (vol)
    };
    std::string fuel_type;  // engine, fuel tank
    itype_id item;      // corresponding item
    int difficulty;     // installation difficulty (mechanics requirement)
    std::set<std::string> flags;    // flags

    bool has_flag(const std::string flag) {
        return flags.count(flag) != 0;
    }
};

extern std::map<std::string, vpart_info> vehicle_part_types;

#endif

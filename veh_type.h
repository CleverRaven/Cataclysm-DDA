#ifndef _VEH_TYPE_H_
#define _VEH_TYPE_H_

#include "color.h"
#include "itype.h"

/**
 * Represents an entry in the breaks_into list.
 */
struct break_entry {
    std::string item_id;
    int min;
    int max;
};

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
    int power;              // engine (top spd), solar panel/powered component (% of 1 fuel per turn, can be > 100)
    union
    {
        int par1;
        int size;       // fuel tank, trunk
        int wheel_width;// wheel width in inches. car could be 9, bicycle could be 2.
        int bonus;      // seatbelt (str), muffler (%), horn (vol)
    };
    std::string fuel_type;  // engine, fuel tank
    itype_id item;      // corresponding item
    int difficulty;     // installation difficulty (mechanics requirement)
    std::string location;   //Where in the vehicle this part goes
    std::set<std::string> flags;    // flags
    std::vector<break_entry> breaks_into;

    int z_order;        // z-ordering, inferred from location, cached here

    bool has_flag(const std::string flag) const {
        return flags.count(flag) != 0;
    }
};

extern std::map<std::string, vpart_info> vehicle_part_types;
extern const std::string legacy_vpart_id[74];
#endif

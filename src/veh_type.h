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

#ifndef mfb
#define mfb(n) static_cast <unsigned long> (1 << (n))
#endif
// bitmask backing store of -certian- vpart_info.flags, ones that
// won't be going away, are involved in core functionality, and are checked frequently
enum vpart_bitflags { 
    VPFLAG_NONE,
    VPFLAG_ARMOR,
    VPFLAG_TRANSPARENT,
    VPFLAG_EVENTURN,
    VPFLAG_ODDTURN,
    VPFLAG_CONE_LIGHT,
    VPFLAG_CIRCLE_LIGHT,
    VPFLAG_BOARDABLE,
    VPFLAG_AISLE,
    VPFLAG_CONTROLS,
    VPFLAG_OBSTACLE,
    VPFLAG_OPAQUE,
    VPFLAG_OPENABLE,
    VPFLAG_SEATBELT,
    VPFLAG_WHEEL
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
    int loadid;             // # of loaded order, non-saved runtime optimization
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
    unsigned long bitflags; // flags checked so often that things slow down due to string cmp

    int z_order;        // z-ordering, inferred from location, cached here

    bool has_flag(const std::string flag) const {
        return flags.count(flag) != 0;
    }
    bool has_flag(const vpart_bitflags flag) {
        return (bitflags & mfb(flag));
    }
};

extern std::map<std::string, vpart_info> vehicle_part_types;
extern const std::string legacy_vpart_id[74];
extern std::vector<vpart_info> vehicle_part_int_types;
extern std::map<std::string, vpart_bitflags> vpart_bitflag_map;
extern void init_vpart_bitflag_map();
#endif

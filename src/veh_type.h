#ifndef VEH_TYPE_H
#define VEH_TYPE_H

#include "string_id.h"
#include "int_id.h"
#include "enums.h"

#include <vector>
#include <bitset>
#include <string>
#include <memory>

struct vpart_info;
using vpart_str_id = string_id<vpart_info>;
using vpart_id = int_id<vpart_info>;
struct vehicle_prototype;
using vproto_id = string_id<vehicle_prototype>;
class vehicle;
class JsonObject;
struct vehicle_item_spawn;
typedef int nc_color;

// bitmask backing store of -certian- vpart_info.flags, ones that
// won't be going away, are involved in core functionality, and are checked frequently
enum vpart_bitflags : int {
    VPFLAG_ARMOR,
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
    VPFLAG_WHEEL,
    VPFLAG_MOUNTABLE,
    VPFLAG_FLOATS,
    VPFLAG_DOME_LIGHT,
    VPFLAG_AISLE_LIGHT,
    VPFLAG_ATOMIC_LIGHT,
    VPFLAG_ALTERNATOR,
    VPFLAG_ENGINE,
    VPFLAG_FRIDGE,
    VPFLAG_FUEL_TANK,
    VPFLAG_LIGHT,
    VPFLAG_WINDOW,
    VPFLAG_CURTAIN,
    VPFLAG_CARGO,
    VPFLAG_INTERNAL,
    VPFLAG_SOLAR_PANEL,
    VPFLAG_VARIABLE_SIZE,
    VPFLAG_TRACK,
    VPFLAG_RECHARGE,
    VPFLAG_EXTENDS_VISION,

    NUM_VPFLAGS
};
/* Flag info:
 * INTERNAL - Can be mounted inside other parts
 * ANCHOR_POINT - Allows secure seatbelt attachment
 * OVER - Can be mounted over other parts
 * VARIABLE_SIZE - Has 'bigness' for power, wheel radius, etc
 * MOUNTABLE - Usable as a point to fire a mountable weapon from.
 * Other flags are self-explanatory in their names. */
struct vpart_info {
    using itype_id = std::string;
    vpart_str_id id;         // unique identifier for this part
    vpart_id loadid;             // # of loaded order, non-saved runtime optimization
    std::string name;       // part name, user-visible
    long sym;               // symbol of part as if it's looking north
    nc_color color;         // color
    char sym_broken;        // symbol of broken part as if it's looking north
    nc_color color_broken;  // color of broken part
    int dmg_mod;            // damage modifier, percent
    int durability;         // durability
    int power;              // engine (top spd), solar panel/powered component (% of 1 fuel per turn, can be > 100)
    int epower;             // electrical power in watts (positive values for generation, negative for consumption)
    int folded_volume;      // volume of a foldable part when folded
    int range;              // turret target finder range
    union {
        int par1;
        int size;       // fuel tank, trunk
        int wheel_width;// wheel width in inches. car could be 9, bicycle could be 2.
        int bonus;      // seatbelt (str), muffler (%), horn (vol)
    };
    itype_id fuel_type;  // engine, fuel tank
    itype_id item;      // corresponding item
    int difficulty;     // installation difficulty (mechanics requirement)
    std::string location;   //Where in the vehicle this part goes
    std::string breaks_into_group;
private:
    std::set<std::string> flags;    // flags
    std::bitset<NUM_VPFLAGS> bitflags; // flags checked so often that things slow down due to string cmp
public:

    int z_order;        // z-ordering, inferred from location, cached here
    int list_order;     // Display order in vehicle interact display

    bool has_flag(const std::string &flag) const
    {
        return flags.count(flag) != 0;
    }
    bool has_flag(const vpart_bitflags flag) const
    {
        return bitflags.test( flag );
    }
    void set_flag( const std::string &flag );

    static void load( JsonObject &jo );
    static void check();
    static void reset();

    static const std::vector<const vpart_info*> &get_all();
};

struct vehicle_item_spawn
{
    point pos;
    int chance;
    std::vector<std::string> item_ids;
    std::vector<std::string> item_groups;
};

/**
 * Prototype of a vehicle. The blueprint member is filled in during the finalizing, before that it
 * is a nullptr. Creating a new vehicle copies the blueprint vehicle.
 */
struct vehicle_prototype
{
    std::string name;
    std::vector<std::pair<point, vpart_str_id> > parts;
    std::vector<vehicle_item_spawn> item_spawns;

    std::unique_ptr<vehicle> blueprint;

    static void load( JsonObject &jo );
    static void reset();
    static void finalize();

    static std::vector<vproto_id> get_all();
};

extern const vpart_str_id legacy_vpart_id[74];

#endif

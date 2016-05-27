#ifndef VEH_TYPE_H
#define VEH_TYPE_H

#include "string_id.h"
#include "int_id.h"
#include "enums.h"
#include "color.h"
#include "damage.h"

#include <vector>
#include <bitset>
#include <string>
#include <memory>

using itype_id = std::string;

class vpart_info;
using vpart_str_id = string_id<vpart_info>;
using vpart_id = int_id<vpart_info>;
struct vehicle_prototype;
using vproto_id = string_id<vehicle_prototype>;
class vehicle;
class JsonObject;
struct vehicle_item_spawn;
struct quality;
using quality_id = string_id<quality>;
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
class vpart_info
{
    public:
        /** Unique identifier for this part */
        vpart_str_id id;

        /** integer identifier derived from load order (non-saved runtime optimization) */
        vpart_id loadid;

        /** Translated name of a part */
        std::string name() const;

        /** base item for this part */
        itype_id item;

        /** What slot of the vehicle tile does this part occupy? */
        std::string location;

        /** Color of part for different states */
        nc_color color = c_ltgray;
        nc_color color_broken = c_ltgray;

        /**
         * Symbol of part which will be translated as follows:
         * y, u, n, b to NW, NE, SE, SW lines correspondingly
         * h, j, c to horizontal, vertical, cross correspondingly
         */
        long sym = 0;
        char sym_broken = '#';

        /** Maximum damage part can sustain before being destroyed */
        int durability = 0;

        /** Damage modifier (percentage) used when damaging other entities upon collision */
        int dmg_mod = 100;

        // Electrical power (watts). Is positive for generation, negative for consumption
        int epower = 0;

        /*
         * For engines this is maximum output
         * For alternators is engine power consumed (negative value)
         * For solar panel/powered components (% of 1 fuel per turn, can be > 100)
         */
        int power = 0;

        /** Fuel type of engine or tank */
        itype_id fuel_type = "null";

        /** Volume of a foldable part when folded */
        int folded_volume = 0;

        /** Maximum turret range */
        int range = 12;

        /** Fuel tank or cargo location volume */
        int size = 0;

        /** Mechanics skill required to install item */
        int difficulty = 0;

        /** @ref item_group this part breaks into when destroyed */
        std::string breaks_into_group = "EMPTY_GROUP";

        /** Tool qualities this vehicle part can provide when installed */
        std::map<quality_id, int> qualities;

        union {
            int par1;
            int wheel_width;// wheel width in inches. car could be 9, bicycle could be 2.
            int bonus;      // seatbelt (str), muffler (%), horn (vol)
        };

        /** Flat decrease of damage of a given type. */
        std::array<float, NUM_DT> damage_reduction;

    private:
        /** Name from vehicle part definition which if set overrides the base item name */
        mutable std::string name_;

        std::set<std::string> flags;    // flags
        std::bitset<NUM_VPFLAGS> bitflags; // flags checked so often that things slow down due to string cmp
    public:

        int z_order;        // z-ordering, inferred from location, cached here
        int list_order;     // Display order in vehicle interact display

        bool has_flag( const std::string &flag ) const {
            return flags.count( flag ) != 0;
        }
        bool has_flag( const vpart_bitflags flag ) const {
            return bitflags.test( flag );
        }
        void set_flag( const std::string &flag );

        static void load( JsonObject &jo );
        static void finalize();
        static void check();
        static void reset();

        static const std::vector<const vpart_info *> &get_all();
};

struct vehicle_item_spawn {
    point pos;
    int chance;
    /** Chance [0-100%] for items to spawn with ammo (plus default magazine if necesssary) */
    int with_ammo = 0;
    /** Chance [0-100%] for items to spawn with their default magazine (if any) */
    int with_magazine = 0;
    std::vector<itype_id> item_ids;
    std::vector<std::string> item_groups;
};

/**
 * Prototype of a vehicle. The blueprint member is filled in during the finalizing, before that it
 * is a nullptr. Creating a new vehicle copies the blueprint vehicle.
 */
struct vehicle_prototype {
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

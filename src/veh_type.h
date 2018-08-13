#pragma once
#ifndef VEH_TYPE_H
#define VEH_TYPE_H

#include "string_id.h"
#include "enums.h"
#include "color.h"
#include "damage.h"
#include "calendar.h"
#include "units.h"

#include <vector>
#include <bitset>
#include <string>
#include <memory>
#include <map>
#include <utility>
#include <array>

using itype_id = std::string;

class vpart_info;
using vpart_id = string_id<vpart_info>;
struct vehicle_prototype;
using vproto_id = string_id<vehicle_prototype>;
class vehicle;
class JsonObject;
struct vehicle_item_spawn;
struct quality;
using quality_id = string_id<quality>;
class Character;

struct requirement_data;
using requirement_id = string_id<requirement_data>;

class Skill;
using skill_id = string_id<Skill>;

// bitmask backing store of -certain- vpart_info.flags, ones that
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
    VPFLAG_LIGHT,
    VPFLAG_WINDOW,
    VPFLAG_CURTAIN,
    VPFLAG_CARGO,
    VPFLAG_INTERNAL,
    VPFLAG_SOLAR_PANEL,
    VPFLAG_RECHARGE,
    VPFLAG_EXTENDS_VISION,
    VPFLAG_ENABLED_DRAINS_EPOWER,
    VPFLAG_WASHING_MACHINE,

    NUM_VPFLAGS
};
/* Flag info:
 * INTERNAL - Can be mounted inside other parts
 * ANCHOR_POINT - Allows secure seatbelt attachment
 * OVER - Can be mounted over other parts
 * MOUNTABLE - Usable as a point to fire a mountable weapon from.
 * Other flags are self-explanatory in their names. */
class vpart_info
{
    private:
        /** Unique identifier for this part */
        vpart_id id;

    public:
        /** Translated name of a part */
        std::string name() const;

        vpart_id get_id() const {
            return id;
        }

        /** base item for this part */
        itype_id item;

        /** What slot of the vehicle tile does this part occupy? */
        std::string location;

        /** Color of part for different states */
        nc_color color = c_light_gray;
        nc_color color_broken = c_light_gray;

        /**
         * Symbol of part which will be translated as follows:
         * y, u, n, b to NW, NE, SE, SW lines correspondingly
         * h, j, c to horizontal, vertical, cross correspondingly
         */
        long sym = 0;
        char sym_broken = '#';

        /** hint to tilesets for what tile to use if this part doesn't have one */
        std::string looks_like;

        /** Maximum damage part can sustain before being destroyed */
        int durability = 0;

        /** A text description of the part as a vehicle part */
        std::string description;

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

        /** Default ammo (for turrets) */
        itype_id default_ammo = "null";

        /** Volume of a foldable part when folded */
        units::volume folded_volume = 0;

        /** Cargo location volume */
        units::volume size = 0;

        /** Mechanics skill required to install item */
        int difficulty = 0;

        /** Legacy parts don't specify installation requirements */
        bool legacy = true;

        /** Format the description for display */
        int format_description( std::ostringstream &msg, std::string format_color, int width ) const;


        /** Installation requirements for this component */
        requirement_data install_requirements() const;

        /** Required skills to install this component */
        std::map<skill_id, int> install_skills;

        /** Installation time (in moves) for component (@see install_time), default 1 hour */
        int install_moves = to_moves<int>( 1_hours );

        /** Installation time (in moves) for this component accounting for player skills */
        int install_time( const Character &ch ) const;

        /** Requirements for removal of this component */
        requirement_data removal_requirements() const;

        /** Required skills to remove this component */
        std::map<skill_id, int> removal_skills;

        /** Removal time (in moves) for component (@see removal_time), default is half @ref install_moves */
        int removal_moves = -1;

        /** Removal time (in moves) for this component accounting for player skills */
        int removal_time( const Character &ch ) const;

        /** Requirements for repair of this component (per level of damage) */
        requirement_data repair_requirements() const;

        /** Returns whether or not the part is repairable  */
        bool is_repairable() const;

        /** Required skills to repair this component */
        std::map<skill_id, int> repair_skills;

        /** Repair time (in moves) to fully repair a component (@see repair_time) */
        int repair_moves = to_moves<int>( 1_hours );

        /** Repair time (in moves) to fully repair this component, accounting for player skills */
        int repair_time( const Character &ch ) const;

        /** @ref item_group this part breaks into when destroyed */
        std::string breaks_into_group = "EMPTY_GROUP";

        /** Tool qualities this vehicle part can provide when installed */
        std::map<quality_id, int> qualities;

        /** seatbelt (str), muffler (%), horn (vol), light (intensity) */
        int bonus = 0;

        /** Flat decrease of damage of a given type. */
        std::array<float, NUM_DT> damage_reduction;

    private:
        /** Name from vehicle part definition which if set overrides the base item name */
        mutable std::string name_;

        std::set<std::string> flags;    // flags
        std::bitset<NUM_VPFLAGS> bitflags; // flags checked so often that things slow down due to string cmp

        /** Second field is the multiplier */
        std::vector<std::pair<requirement_id, int>> install_reqs;
        std::vector<std::pair<requirement_id, int>> removal_reqs;
        std::vector<std::pair<requirement_id, int>> repair_reqs;

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

        static void load( JsonObject &jo, const std::string &src );
        static void finalize();
        static void check();
        static void reset();

        static const std::map<vpart_id, vpart_info> &all();
};

struct vehicle_item_spawn {
    point pos;
    int chance;
    /** Chance [0-100%] for items to spawn with ammo (plus default magazine if necessary) */
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
    struct part_def {
        point pos;
        vpart_id part;
        int with_ammo = 0;
        std::set<itype_id> ammo_types;
        std::pair<int, int> ammo_qty = { -1, -1 };
        itype_id fuel = "null";
    };

    std::string name;
    std::vector<part_def> parts;
    std::vector<vehicle_item_spawn> item_spawns;

    std::unique_ptr<vehicle> blueprint;

    static void load( JsonObject &jo );
    static void reset();
    static void finalize();

    static std::vector<vproto_id> get_all();
};

#endif

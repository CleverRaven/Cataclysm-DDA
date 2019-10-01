#pragma once
#ifndef VEH_TYPE_H
#define VEH_TYPE_H

#include <array>
#include <bitset>
#include <map>
#include <memory>
#include <string>
#include <set>
#include <utility>
#include <vector>
#include <sstream>

#include "calendar.h"
#include "color.h"
#include "damage.h"
#include "optional.h"
#include "string_id.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "requirements.h"
#include "point.h"
#include "translations.h"

using itype_id = std::string;

class JsonObject;
class Character;

// bitmask backing store of -certain- vpart_info.flags, ones that
// won't be going away, are involved in core functionality, and are checked frequently
enum vpart_bitflags : int {
    VPFLAG_ARMOR,
    VPFLAG_EVENTURN,
    VPFLAG_ODDTURN,
    VPFLAG_CONE_LIGHT,
    VPFLAG_WIDE_CONE_LIGHT,
    VPFLAG_HALF_CIRCLE_LIGHT,
    VPFLAG_CIRCLE_LIGHT,
    VPFLAG_BOARDABLE,
    VPFLAG_AISLE,
    VPFLAG_CONTROLS,
    VPFLAG_OBSTACLE,
    VPFLAG_OPAQUE,
    VPFLAG_OPENABLE,
    VPFLAG_SEATBELT,
    VPFLAG_SPACE_HEATER,
    VPFLAG_COOLER,
    VPFLAG_WHEEL,
    VPFLAG_MOUNTABLE,
    VPFLAG_FLOATS,
    VPFLAG_DOME_LIGHT,
    VPFLAG_AISLE_LIGHT,
    VPFLAG_ATOMIC_LIGHT,
    VPFLAG_ALTERNATOR,
    VPFLAG_ENGINE,
    VPFLAG_FRIDGE,
    VPFLAG_FREEZER,
    VPFLAG_LIGHT,
    VPFLAG_WINDOW,
    VPFLAG_CURTAIN,
    VPFLAG_CARGO,
    VPFLAG_INTERNAL,
    VPFLAG_SOLAR_PANEL,
    VPFLAG_WATER_WHEEL,
    VPFLAG_WIND_TURBINE,
    VPFLAG_RECHARGE,
    VPFLAG_EXTENDS_VISION,
    VPFLAG_ENABLED_DRAINS_EPOWER,
    VPFLAG_AUTOCLAVE,
    VPFLAG_WASHING_MACHINE,
    VPFLAG_DISHWASHER,
    VPFLAG_FLUIDTANK,
    VPFLAG_REACTOR,
    VPFLAG_RAIL,

    NUM_VPFLAGS
};
/* Flag info:
 * INTERNAL - Can be mounted inside other parts
 * ANCHOR_POINT - Allows secure seatbelt attachment
 * OVER - Can be mounted over other parts
 * MOUNTABLE - Usable as a point to fire a mountable weapon from.
 * E_COLD_START - Cold weather makes the engine take longer to start
 * E_STARTS_INSTANTLY - The engine takes no time to start, like foot pedals
 * E_ALTERNATOR - The engine can mount and power an alternator
 * E_COMBUSTION - The engine burns fuel to provide power and can burn or explode
 * E_HIGHER_SKILL - Multiple engines with this flag are harder to install
 * Other flags are self-explanatory in their names. */

struct vpslot_engine {
    float backfire_threshold = 0;
    int backfire_freq = 1;
    int muscle_power_factor = 0;
    float damaged_power_factor = 0;
    int noise_factor = 0;
    int m2c = 1;
    std::vector<std::string> exclusions;
    std::vector<itype_id> fuel_opts;
};

struct veh_ter_mod {
    int movecost;   /* movecost for moving through this terrain (overrides current terrain movecost)
                     * if movecost <= 0 ignore this parameter */
    int penalty;    // penalty while not on this terrain (adds to movecost)
};

struct vpslot_wheel {
    float rolling_resistance = 1;
    int contact_area = 1;
    std::vector<std::pair<std::string, veh_ter_mod>> terrain_mod;
    float or_rating;
};

struct vpslot_workbench {
    // Base multiplier applied for crafting here
    float multiplier;
    // Mass/volume allowed before a crafting speed penalty is applied
    units::mass allowed_mass;
    units::volume allowed_volume;
};

struct transform_terrain_data {
    std::set<std::string> pre_flags;
    std::string post_terrain;
    std::string post_furniture;
    std::string post_field;
    int post_field_intensity;
    time_duration post_field_age;
};

class vpart_info
{
    private:
        /** Unique identifier for this part */
        vpart_id id;

        cata::optional<vpslot_engine> engine_info;
        cata::optional<vpslot_wheel> wheel_info;
        cata::optional<vpslot_workbench> workbench_info;

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
        int sym = 0;
        char sym_broken = '#';

        /** hint to tilesets for what tile to use if this part doesn't have one */
        std::string looks_like;

        /** Maximum damage part can sustain before being destroyed */
        int durability = 0;

        /** A text description of the part as a vehicle part */
        translation description;

        /** Damage modifier (percentage) used when damaging other entities upon collision */
        int dmg_mod = 100;

        /**
         * Electrical power, flat rate (watts); positive for generation, negative for consumption
         * For motor consumption scaled with powertrain demand see @ref energy_consumption instead
         */
        int epower = 0;

        /**
         * Energy consumed by engines and motors (watts) when delivering max @ref power
         * Includes waste. Gets scaled based on powertrain demand.
         */
        int energy_consumption = 0;

        /**
         * For engines and motors this is maximum output (watts)
         * For alternators is engine power consumed (negative value)
         */
        int power = 0;

        /** Emissions of part */
        std::set<emit_id> emissions;

        /** Fuel type of engine or tank */
        itype_id fuel_type = "null";

        /** Default ammo (for turrets) */
        itype_id default_ammo = "null";

        /** Volume of a foldable part when folded */
        units::volume folded_volume = 0_ml;

        /** Cargo location volume */
        units::volume size = 0_ml;

        /** Mechanics skill required to install item */
        int difficulty = 0;

        /** Legacy parts don't specify installation requirements */
        bool legacy = true;

        /** Format the description for display */
        int format_description( std::ostringstream &msg, const nc_color &format_color, int width ) const;

        /** Installation requirements for this component */
        requirement_data install_requirements() const;

        /** Required skills to install this component */
        std::map<skill_id, int> install_skills;

        /** Installation time (in moves) for component (@see install_time), default 1 hour */
        int install_moves = to_moves<int>( 1_hours );

        /** Installation time (in moves) for this component accounting for player skills */
        int install_time( const player &p ) const;

        /** Requirements for removal of this component */
        requirement_data removal_requirements() const;

        /** Required skills to remove this component */
        std::map<skill_id, int> removal_skills;

        /** Removal time (in moves) for component (@see removal_time), default is half @ref install_moves */
        int removal_moves = -1;

        /** Removal time (in moves) for this component accounting for player skills */
        int removal_time( const player &p ) const;

        /** Requirements for repair of this component (per level of damage) */
        requirement_data repair_requirements() const;

        /** Returns whether or not the part is repairable  */
        bool is_repairable() const;

        /** Required skills to repair this component */
        std::map<skill_id, int> repair_skills;

        /** Repair time (in moves) to fully repair a component (@see repair_time) */
        int repair_moves = to_moves<int>( 1_hours );

        /** Repair time (in moves) to fully repair this component, accounting for player skills */
        int repair_time( const player &p ) const;

        /** @ref item_group this part breaks into when destroyed */
        std::string breaks_into_group = "EMPTY_GROUP";

        /** Tool qualities this vehicle part can provide when installed */
        std::map<quality_id, int> qualities;

        /** seatbelt (str), muffler (%), horn (vol), light (intensity) */
        int bonus = 0;

        /** cargo weight modifier (percentage) */
        int cargo_weight_modifier = 100;

        /** Flat decrease of damage of a given type. */
        std::array<float, NUM_DT> damage_reduction = {};

        /* Contains data for terrain transformer parts */
        transform_terrain_data transform_terrain;

        /**
         * @name Engine specific functions
         *
         */
        std::vector<std::string> engine_excludes() const;
        int engine_m2c() const;
        float engine_backfire_threshold() const;
        int engine_backfire_freq() const;
        int engine_muscle_power_factor() const;
        float engine_damaged_power_factor() const;
        int engine_noise_factor() const;
        std::vector<itype_id> engine_fuel_opts() const;
        /**
         * @name Wheel specific functions
         *
         */
        float wheel_rolling_resistance() const;
        int wheel_area() const;
        std::vector<std::pair<std::string, veh_ter_mod>> wheel_terrain_mod() const;
        float wheel_or_rating() const;

        /**
         * Getter for optional workbench info
         */
        const cata::optional<vpslot_workbench> &get_workbench_info() const;

    private:
        /** Name from vehicle part definition which if set overrides the base item name */
        translation name_;

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

        static void load_engine( cata::optional<vpslot_engine> &eptr, JsonObject &jo,
                                 const itype_id &fuel_type );
        static void load_wheel( cata::optional<vpslot_wheel> &whptr, JsonObject &jo );
        static void load_workbench( cata::optional<vpslot_workbench> &wbptr, JsonObject &jo );
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

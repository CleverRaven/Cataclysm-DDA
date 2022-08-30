#pragma once
#ifndef CATA_SRC_VEH_TYPE_H
#define CATA_SRC_VEH_TYPE_H

#include <algorithm>
#include <array>
#include <bitset>
#include <iosfwd>
#include <map>
#include <memory>
#include <new>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "color.h"
#include "compatibility.h"
#include "damage.h"
#include "optional.h"
#include "point.h"
#include "requirements.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"

class JsonObject;
class Character;
class vehicle;

// bitmask backing store of -certain- vpart_info.flags, ones that
// won't be going away, are involved in core functionality, and are checked frequently
enum vpart_bitflags : int {
    VPFLAG_ARMOR,
    VPFLAG_APPLIANCE,
    VPFLAG_ARCADE,
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
    VPFLAG_SIMPLE_PART,
    VPFLAG_SPACE_HEATER,
    VPFLAG_HEATED_TANK,
    VPFLAG_COOLER,
    VPFLAG_WHEEL,
    VPFLAG_ROTOR,
    VPFLAG_ROTOR_SIMPLE,
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
    VPFLAG_TURRET_CONTROLS,
    VPFLAG_ROOF,

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
    float backfire_threshold = 0.0f;
    int backfire_freq = 1;
    int muscle_power_factor = 0;
    float damaged_power_factor = 0.0f;
    int noise_factor = 0;
    int m2c = 1;
    std::vector<std::string> exclusions;
    std::vector<itype_id> fuel_opts;
};

struct veh_ter_mod {
    /* movecost for moving through this terrain (overrides current terrain movecost)
                     * if movecost <= 0 ignore this parameter */
    int movecost;
    // penalty while not on this terrain (adds to movecost)
    int penalty;
};

struct vpslot_wheel {
    float rolling_resistance = 1.0f;
    int contact_area = 1;
    std::vector<std::pair<std::string, veh_ter_mod>> terrain_mod;
    float or_rating = 0.0f;
};

struct vpslot_rotor {
    int rotor_diameter = 1;
};

struct vpslot_workbench {
    // Base multiplier applied for crafting here
    float multiplier = 1.0f;
    // Mass/volume allowed before a crafting speed penalty is applied
    units::mass allowed_mass = 0_gram;
    units::volume allowed_volume = 0_ml;
};

struct transform_terrain_data {
    std::set<std::string> pre_flags;
    std::string post_terrain;
    std::string post_furniture;
    std::string post_field;
    int post_field_intensity = 0;
    time_duration post_field_age = 0_turns;
};

const std::vector<std::pair<std::string, translation>> vpart_variants = {
    { "cover_left", to_translation( "vpart_variants", "Cover Left" ) },
    { "cover_right", to_translation( "vpart_variants", "Cover Right" ) },
    { "hatch_wheel_left", to_translation( "vpart_variants", "Hatchback Wheel Left" ) },
    { "hatch_wheel_right", to_translation( "vpart_variants", "Hatchback Wheel Right" ) },
    { "wheel_left", to_translation( "vpart_variants", "Wheel Left" ) },
    { "wheel_right", to_translation( "vpart_variants", "Wheel Right" ) },
    { "cross_unconnected", to_translation( "vpart_variants", "Unconnected Cross" ) },
    { "cross", to_translation( "vpart_variants", "Cross" ) },
    { "horizontal_front_edge", to_translation( "vpart_variants", "Front Edge Horizontal" ) },
    { "horizontal_front", to_translation( "vpart_variants", "Front Horizontal" ) },
    { "horizontal_rear_edge", to_translation( "vpart_variants", "Rear Edge Horizontal" ) },
    { "horizontal_rear", to_translation( "vpart_variants", "Rear Horizontal" ) },
    { "horizontal_2_front", to_translation( "vpart_variants", "Front Thick Horizontal" ) },
    { "horizontal_2_rear", to_translation( "vpart_variants", "Rear Thick Horizontal" ) },
    { "ne_edge", to_translation( "vpart_variants", "Front Right Corner" ) },
    { "nw_edge", to_translation( "vpart_variants", "Front Left Corner" ) },
    { "se_edge", to_translation( "vpart_variants", "Rear Right Corner" ) },
    { "sw_edge", to_translation( "vpart_variants", "Rear Left Corner" ) },
    { "vertical_right", to_translation( "vpart_variants", "Right Vertical" ) },
    { "vertical_left", to_translation( "vpart_variants", "Left Vertical" ) },
    { "vertical_2_right", to_translation( "vpart_variants", "Right Thick Vertical" ) },
    { "vertical_2_left", to_translation( "vpart_variants", "Left Thick Vertical" ) },
    { "vertical_T_right", to_translation( "vpart_variants", "Right T Joint" ) },
    { "vertical_T_left", to_translation( "vpart_variants", "Left T Joint" ) },
    { "front_right", to_translation( "vpart_variants", "Front Right" ) },
    { "front_left", to_translation( "vpart_variants", "Front Left" ) },
    { "rear_right", to_translation( "vpart_variants", "Rear Right" ) },
    { "rear_left", to_translation( "vpart_variants", "Rear Left" ) },
    // these have to be last to avoid false positives
    { "cover", to_translation( "vpart_variants", "Cover" ) },
    { "vertical", to_translation( "vpart_variants", "Vertical" ) },
    { "horizontal", to_translation( "vpart_variants", "Horizontal" ) },
    { "vertical_2", to_translation( "vpart_variants", "Thick Vertical" ) },
    { "horizontal_2", to_translation( "vpart_variants", "Thick Horizontal" ) },
    { "ne", to_translation( "vpart_variants", "Front Right" ) },
    { "nw", to_translation( "vpart_variants", "Front Left" ) },
    { "se", to_translation( "vpart_variants", "Rear Right" ) },
    { "sw", to_translation( "vpart_variants", "Rear Left" ) },
    { "front", to_translation( "vpart_variants", "Front" ) },
    { "rear", to_translation( "vpart_variants", "Rear" ) },
    { "left", to_translation( "vpart_variants", "Left" ) },
    { "right", to_translation( "vpart_variants", "Right" ) }
};

const std::map<std::string, int> vpart_variants_standard = {
    { "cover", '^' }, { "cross", 'c' },
    { "horizontal", 'h' }, { "horizontal_2", '=' }, { "vertical", 'j' }, { "vertical_2", 'H' },
    { "ne", 'u' }, { "nw", 'y' }, { "se", 'n' }, { "sw", 'b' }
};
std::pair<std::string, std::string> get_vpart_str_variant( const std::string &vpid );
std::pair<vpart_id, std::string> get_vpart_id_variant( const vpart_id &vpid );
std::pair<vpart_id, std::string> get_vpart_id_variant( const std::string &vpid );

class vpart_category
{
    public:
        static const std::vector<vpart_category> &all();

        static void load( const JsonObject &jo );
        static void finalize();
        static void reset();

        std::string get_id() const {
            return id_;
        }

        std::string name() const {
            return name_.translated();
        }

        std::string short_name() const {
            return short_name_.translated();
        }

        bool operator < ( const vpart_category &cat ) const {
            return priority_ < cat.priority_;
        }

    private:
        std::string id_;
        translation name_;
        translation short_name_;
        int priority_ = 0; // order of tab in the UI
};

class vpart_info
{
    public:
        static void load_engine( cata::optional<vpslot_engine> &eptr, const JsonObject &jo,
                                 const itype_id &fuel_type );
        static void load_wheel( cata::optional<vpslot_wheel> &whptr, const JsonObject &jo );
        static void load_workbench( cata::optional<vpslot_workbench> &wbptr, const JsonObject &jo );
        static void load_rotor( cata::optional<vpslot_rotor> &roptr, const JsonObject &jo );
        static void load( const JsonObject &jo, const std::string &src );
        static void finalize();
        static void check();
        static void reset();

        static const std::map<vpart_id, vpart_info> &all();

        /** Translated name of a part */
        std::string name() const;

        vpart_id get_id() const {
            return id;
        }

        const std::set<std::string> &get_flags() const {
            return flags;
        }
        bool has_flag( const std::string &flag ) const {
            return flags.count( flag ) != 0;
        }
        bool has_flag( const vpart_bitflags flag ) const {
            return bitflags.test( flag );
        }
        void set_flag( const std::string &flag );

        bool has_tool( const itype_id &tool ) const {
            return std::find_if( pseudo_tools.cbegin(),
            pseudo_tools.cend(), [&tool]( std::pair<itype_id, int>p ) {
                return p.first == tool;
            } ) != pseudo_tools.cend();
        }

        /** Gets all categories of this part */
        const std::set<std::string> &get_categories() const;

        /** Gets whether part is in a category for display */
        bool has_category( const std::string &category ) const;

        /** Format the description for display */
        int format_description( std::string &msg, const nc_color &format_color, int width ) const;

        /** Installation requirements for this component */
        requirement_data install_requirements() const;

        /** Installation time (in moves) for this component accounting for player skills */
        int install_time( const Character &you ) const;

        /** Requirements for removal of this component */
        requirement_data removal_requirements() const;

        /** Removal time (in moves) for this component accounting for player skills */
        int removal_time( const Character &you ) const;

        /** Requirements for repair of this component (per level of damage) */
        requirement_data repair_requirements() const;

        /** Returns whether or not the part is repairable  */
        bool is_repairable() const;

        /** Repair time (in moves) to fully repair this component, accounting for player skills */
        int repair_time( const Character &you ) const;

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
        /** @name rotor specific functions
        */
        int rotor_diameter() const;
        /**
         * Getter for optional workbench info
         */
        const cata::optional<vpslot_workbench> &get_workbench_info() const;

        std::set<std::pair<itype_id, int>> get_pseudo_tools() const;
    private:
        std::set<std::string> flags;
        // category list for installation ui breakdown
        std::set<std::string> categories;
        // flags checked so often that things slow down due to string cmp
        std::bitset<NUM_VPFLAGS> bitflags;

        /** Second field is the multiplier */
        std::vector<std::pair<requirement_id, int>> install_reqs;
        std::vector<std::pair<requirement_id, int>> removal_reqs;
        std::vector<std::pair<requirement_id, int>> repair_reqs;

        /** Pseudo tools this provides when installed, second is the hotkey */
        std::set<std::pair<itype_id, int>> pseudo_tools;

        cata::optional<vpslot_engine> engine_info;
        cata::optional<vpslot_wheel> wheel_info;
        cata::optional<vpslot_rotor> rotor_info;
        cata::optional<vpslot_workbench> workbench_info;

        /** Unique identifier for this part */
        vpart_id id;

        /** Name from vehicle part definition which if set overrides the base item name */
        translation name_;

    public:
        /* map of standard variant names to symbols */
        std::map<std::string, int> symbols;

        /** Required skills to install, repair, or remove this component */
        std::map<skill_id, int> install_skills;
        std::map<skill_id, int> repair_skills;
        std::map<skill_id, int> removal_skills;

        /** @ref item_group this part breaks into when destroyed */
        item_group_id breaks_into_group = item_group_id( "EMPTY_GROUP" );

        /** Flat decrease of damage of a given type. */
        std::array<float, static_cast<int>( damage_type::NUM )> damage_reduction = {};

        /** Tool qualities this vehicle part can provide when installed */
        std::map<quality_id, int> qualities;

        /** Emissions of part */
        std::set<emit_id> emissions;

        /**
          * Exhaust emissions of part

          * If the vehicle has an exhaust part, it is emitted there;
          * otherwise, it is emitted in place
          */
        std::set<emit_id> exhaust;

        /** Color of part for different states */
        nc_color color = c_light_gray;
        nc_color color_broken = c_light_gray;

        /* Contains data for terrain transformer parts */
        transform_terrain_data transform_terrain;

        /** Fuel type of engine or tank */
        itype_id fuel_type = itype_id::NULL_ID();

        /** Default ammo (for turrets) */
        itype_id default_ammo = itype_id::NULL_ID();

        /** Volume of a foldable part when folded */
        units::volume folded_volume = 0_ml;

        /** Cargo location volume */
        units::volume size = 0_ml;

        /** hint to tilesets for what tile to use if this part doesn't have one */
        std::string looks_like;

        /** A text description of the part as a vehicle part */
        translation description;

        /** base item for this part */
        itype_id base_item;

        /** What slot of the vehicle tile does this part occupy? */
        std::string location;

        /**
         * Symbol of part which will be translated as follows:
         * y, u, n, b to NW, NE, SE, SW lines correspondingly
         * h, j, c to horizontal, vertical, cross correspondingly
         */
        int sym = 0;
        int sym_broken = '#';

        /** Maximum damage part can sustain before being destroyed */
        int durability = 0;

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

        /** Installation time (in moves) for component (@see install_time), default 1 hour */
        int install_moves = to_moves<int>( 1_hours );
        /** Repair time (in moves) to fully repair a component (@see repair_time) */
        int repair_moves = to_moves<int>( 1_hours );
        /** Removal time (in moves) for component (@see removal_time),
         *  default is half @ref install_moves */
        int removal_moves = -1;

        /** seatbelt (str), muffler (%), horn (vol), light (intensity), recharging (power) */
        int bonus = 0;

        /** cargo weight modifier (percentage) */
        int cargo_weight_modifier = 100;

        /*Comfort data for sleeping in vehicles*/
        int comfort = 0;
        int floor_bedding_warmth = 0;
        int bonus_fire_warmth_feet = 300;

        // z-ordering, inferred from location, cached here
        int z_order = 0;
        // Display order in vehicle interact display
        int list_order = 0;

        /** Legacy parts don't specify installation requirements */
        bool legacy = true;
};

struct vehicle_item_spawn {
    point pos;
    int chance = 0;
    /** Chance [0-100%] for items to spawn with ammo (plus default magazine if necessary) */
    int with_ammo = 0;
    /** Chance [0-100%] for items to spawn with their default magazine (if any) */
    int with_magazine = 0;
    std::vector<itype_id> item_ids;
    // item_ids, but for items with variants specified
    std::vector<std::pair<itype_id, std::string>> variant_ids;
    std::vector<item_group_id> item_groups;
};

/**
 * Prototype of a vehicle. The blueprint member is filled in during the finalizing, before that it
 * is a nullptr. Creating a new vehicle copies the blueprint vehicle.
 */
struct vehicle_prototype {
    struct part_def {
        point pos;
        vpart_id part;
        std::string variant;
        int with_ammo = 0;
        std::set<itype_id> ammo_types;
        std::pair<int, int> ammo_qty = { -1, -1 };
        itype_id fuel = itype_id::NULL_ID();
    };

    struct zone_def {
        zone_type_id zone_type;
        std::string name;
        std::string filter;
        point pt;
    };

    vehicle_prototype();
    vehicle_prototype( vehicle_prototype && ) noexcept;
    ~vehicle_prototype();

    vehicle_prototype &operator=( vehicle_prototype && ) noexcept( string_is_noexcept );

    translation name;
    std::vector<part_def> parts;
    std::vector<vehicle_item_spawn> item_spawns;
    std::vector<zone_def> zone_defs;

    std::unique_ptr<vehicle> blueprint;

    static void load( const JsonObject &jo );
    static void reset();
    static void finalize();

    static std::vector<vproto_id> get_all();
};

/**
* Holder for all vpart migrations, when loading parts with vpart_id present
* in the map keys they'll be replaced by vpart_id in the pair's value
*/
class vpart_migration
{
    public:
        /** Handler for loading "vehicle_part_migration" type of json object */
        static void load( const JsonObject &jo );

        /** Clears migration list */
        static void reset();

        /** Map of deprecated vpart_id to their replacement vpart_id */
        static const std::map<vpart_id, vpart_id> &get_migrations();
};

#endif // CATA_SRC_VEH_TYPE_H

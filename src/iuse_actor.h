#pragma once
#ifndef CATA_SRC_IUSE_ACTOR_H
#define CATA_SRC_IUSE_ACTOR_H

#include <climits>
#include <iosfwd>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "color.h"
#include "coordinates.h"
#include "damage.h"
#include "enums.h"
#include "explosion.h"
#include "iuse.h"
#include "ret_val.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"

class Character;
class JsonObject;
class item;
class item_location;
class npc_template;
struct furn_t;
struct iteminfo;
struct tripoint;

/**
 * Transform an item into a specific type.
 * Optionally activate it.
 * Optionally split it in container and content (like opening a jar).
 *
 * It optionally checks for
 * 1. original item has a minimal amount of charges.
 * 2. player has a minimal amount of "fire" charges and consumes them,
 * 3. if fire is used, checks that the player is not underwater.
 */
class iuse_transform : public iuse_actor
{
    public:
        /** displayed if player sees transformation with %s replaced by item name */
        translation msg_transform;

        /** type of the resulting item */
        itype_id target;

        /** if set transform item to container and place new item (of type @ref target) inside */
        itype_id container;

        /** if set choose this variant when transforming */
        std::string variant_type = "<any>";

        /** whether the transformed container is sealed */
        bool sealed = true;

        /** if zero or positive set remaining ammo of @ref target to this (after transformation) */
        int ammo_qty = -1;

        /** if this has values, set remaining ammo of @ref target to one of them chosen at random (after transformation) */
        std::vector<int> random_ammo_qty;

        /** if positive set transformed item active and start countdown for countdown_action*/
        time_duration target_timer = 0_seconds;

        /** if both this and ammo_qty are specified then set @ref target to this specific ammo */
        itype_id ammo_type;

        /** used to set the active property of the transformed @ref target */
        bool active = false;

        /**does the item requires to be worn to be activable*/
        bool need_worn = false;

        /**does the item requires to be wielded to be activable*/
        bool need_wielding = false;

        /** does the item require being empty to be activable */
        bool need_empty = false;

        /** subtracted from @ref Creature::moves when transformation is successful */
        int moves = 0;

        /** minimum number of fire charges required (if any) for transformation */
        int need_fire = 0;

        /** displayed if item is in player possession with %s replaced by item name */
        translation need_fire_msg;

        /** minimum charges (if any) required for transformation */
        int need_charges = 0;

        /** displayed if item is in player possession with %s replaced by item name */
        translation need_charges_msg;

        /** Tool qualities needed, e.g. "fine bolt turning 1". **/
        std::map<quality_id, int> qualities_needed;

        translation menu_text;

        explicit iuse_transform( const std::string &type = "transform" ) : iuse_actor( type ) {}

        ~iuse_transform() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        ret_val<void> can_use( const Character &, const item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        std::string get_name() const override;
        void finalize( const itype_id &my_item_type ) override;
        void info( const item &, std::vector<iteminfo> & ) const override;
    private:
        void do_transform( Character *p, item &it, const std::string &variant_type ) const;
};

class unpack_actor : public iuse_actor
{
    public:
        /** The itemgroup from which we unpack items from */
        item_group_id unpack_group;

        /** Whether or not the items from the group should spawn fitting */
        bool items_fit = false;

        /**
         *  If the item is filthy, at what volume (held) threshold should the
         *   items unpacked be made filthy
         */
        units::volume filthy_vol_threshold = 0_ml;

        explicit unpack_actor( const std::string &type = "unpack" ) : iuse_actor( type ) {}

        ~unpack_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *p, item &it, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> &dump ) const override;
};

class message_iuse : public iuse_actor
{
    public:
        explicit message_iuse( const std::string &type = "message" ) : iuse_actor( type ) {}

        /** if specified overrides default action name */
        translation name;

        /** message if player sees activation with %s replaced by item name */
        translation message;

        ~message_iuse() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        std::string get_name() const override;
};

class sound_iuse : public iuse_actor
{
    public:
        explicit sound_iuse( const std::string &type = "sound" ) : iuse_actor( type ) {}

        /** if specified overrides default action name */
        translation name;
        /** message if player hears activation with %s replaced by item name */

        translation sound_message;

        int sound_volume = 0;
        std::string sound_id = "misc";
        std::string sound_variant = "default";

        ~sound_iuse() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        std::string get_name() const override;
};

/**
 * This is a @ref iuse_actor for active items that explode when
 * their charges reaches 0.
 * It can be called each turn, it can make a sound each turn.
 */
class explosion_iuse : public iuse_actor
{
    public:
        // Structure describing the explosion + shrapnel
        // Ignored if its power field is < 0
        explosion_data explosion;

        // Those 2 values are forwarded to game::draw_explosion,
        // Nothing is drawn if radius < 0 (game::explosion might still draw something)
        int draw_explosion_radius = -1;
        nc_color draw_explosion_color = c_white;
        /** Call game::flashbang? */
        bool do_flashbang = false;
        bool flashbang_player_immune = false;
        /** Create fields of this type around the center of the explosion */
        int fields_radius = -1;
        field_type_id fields_type;
        int fields_min_intensity = 1;
        int fields_max_intensity = 0;
        /** Calls game::emp_blast if >= 0 */
        int emp_blast_radius = -1;
        /** Calls game::scrambler_blast if >= 0 */
        int scrambler_blast_radius = -1;

        explicit explosion_iuse( const std::string &type = "explosion" ) : iuse_actor( type ) {}

        ~explosion_iuse() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};

/** Used in consume_drug_iuse for storing effect data. */
struct effect_data {
    efftype_id id;
    time_duration duration;
    bodypart_id bp;
    bool permanent;

    effect_data( const efftype_id &nid, const time_duration &dur, bodypart_id nbp, bool perm ) :
        id( nid ), duration( dur ), bp( nbp ), permanent( perm ) {}
};

/**
 * This iuse encapsulates the effects of taking a drug.
 */
class consume_drug_iuse : public iuse_actor
{
    public:
        /** Message to display when drug is consumed. **/
        translation activation_message;
        /** Fields to produce when you take the drug, mostly intended for various kinds of smoke. **/
        std::map<std::string, int> fields_produced;
        /** Tool charges needed to take the drug, e.g. fire. **/
        std::map<itype_id, int> charges_needed;
        /** Tools needed, but not consumed, e.g. "smoking apparatus". **/
        std::map<itype_id, int> tools_needed;
        /** An effect or effects (conditions) to give the player for the stated duration. **/
        std::vector<effect_data> effects;
        /** A list of stats and adjustments to them. **/
        std::map<std::string, int> stat_adjustments;

        /** Modify player vitamin_levels by random amount between min (first) and max (second) */
        std::map<vitamin_id, std::pair<int, int>> vitamins;

        /**List of damage over time applied by this drug, negative damage heals*/
        std::vector<damage_over_time_data> damage_over_time;

        /** How many move points this action takes. */
        int moves = 100;

        explicit consume_drug_iuse( const std::string &type = "consume_drug" ) : iuse_actor( type ) {}

        ~consume_drug_iuse() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;

        /** Item produced after using drugs. */
        std::string used_up_item;
};

/**
 * This is a @ref iuse_transform that uses the age of the item instead of a counter.
 * The age is calculated from the current turn and the birthday of the item.
 * The player has to activate the item manually, only when the specific
 * age has been reached, it will transform.
 */
class delayed_transform_iuse : public iuse_transform
{
    public:
        /**
         * The minimal age of the item (in turns) to allow the transformation.
         */
        int transform_age = 0;
        /**
         * Message to display when the user activates the item before the
         * age has been reached.
         */
        translation not_ready_msg;

        /** How much longer (in turns) until the transformation can be done, can be negative. */
        int time_to_do( const item &it ) const;

        explicit delayed_transform_iuse( const std::string &type = "delayed_transform" ) : iuse_transform(
                type ) {}

        ~delayed_transform_iuse() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};

/**
 * This iuse contains the logic to transform a robot item into an actual monster on the map.
 */
class place_monster_iuse : public iuse_actor
{
    public:
        /** The monster type id of the monster to create. */
        mtype_id mtypeid;
        /** If true, place the monster at a random square around the player,
         * otherwise allow the player to select the target square. */
        bool place_randomly = false;
        /** How many move points this action takes. */
        int moves = 100;
        /** Difficulty of programming the monster (to be friendly). */
        int difficulty = 0;
        /** Shown when programming the monster succeeded and it's friendly. Can be empty. */
        translation friendly_msg;
        /** Shown when programming the monster failed and it's hostile. Can be empty. */
        translation hostile_msg;
        /** Skills used to make the monster not hostile when activated. **/
        std::set<skill_id> skills;
        /** The monster will be spawned in as a pet. False by default. Can be empty. */
        bool is_pet = false;
        /** minimum charges (if any) required for activating monster */
        int need_charges = 0;

        place_monster_iuse() : iuse_actor( "place_monster" ) { }
        ~place_monster_iuse() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};

/**
 * This iuse contains the logic to change one's scent.
 */
class change_scent_iuse : public iuse_actor
{
    public:
        /** The scent type id of the new scent. */
        scenttype_id scenttypeid;
        /** How many move points this action takes. */
        int moves = 100;
        /**How many charge are consumed on use*/
        int charges_to_use = 1;
        /**Scent value modifier*/
        int scent_mod = 0;
        /**How long does the scent stays*/
        time_duration duration;
        /**Is the scent mask waterproof*/
        bool waterproof = false;
        /**Side effect of using the item*/
        std::vector<effect_data> effects;

        change_scent_iuse() : iuse_actor( "change_scent" ) { }
        ~change_scent_iuse() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};

/**
 * This iuse contains the logic to summon an npc on the map.
 */
class place_npc_iuse : public iuse_actor
{
    public:
        string_id<npc_template> npc_class_id;
        bool place_randomly = false;
        int radius = 1;
        int moves = 100;
        translation summon_msg;

        place_npc_iuse() : iuse_actor( "place_npc" ) { }
        ~place_npc_iuse() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};

/**
 * Implements deployable furniture from items
 */
class deploy_furn_actor : public iuse_actor
{
    public:
        /**
         * furniture type id the item should create
         */
        string_id<furn_t> furn_type;

        deploy_furn_actor() : iuse_actor( "deploy_furn" ) {}

        ~deploy_furn_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};

/**
 * Deployable appliances from items
 */
class deploy_appliance_actor : public iuse_actor
{
    public:
        itype_id appliance_base;

        deploy_appliance_actor() : iuse_actor( "deploy_appliance" ) {}
        ~deploy_appliance_actor() override = default;

        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};

/**
 * Reveals specific things on the overmap.
 */
class reveal_map_actor : public iuse_actor
{
    public:
        /**
         * The radius of the overmap area that gets revealed.
         * This is in overmap terrain coordinates.
         * A radius of 1 means all terrains directly around center are revealed.
         * The center is location of nearest city defined in `reveal_map_center_omt` variable of
         * activated item (or current player global omt location if variable is not set).
         */
        int radius = 0;
        /**
         * Overmap terrain types that get revealed.
         */
        std::vector<std::pair<std::string, ot_match_type>> omt_types;
        /**
         * The message displayed after revealing.
         */
        translation message;

        void reveal_targets(
            const tripoint_abs_omt &center, const std::pair<std::string, ot_match_type> &target,
            int reveal_distance ) const;

        explicit reveal_map_actor( const std::string &type = "reveal_map" ) : iuse_actor( type ) {}

        ~reveal_map_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};

/**
 * Starts a fire instantly
 */
class firestarter_actor : public iuse_actor
{
    public:
        /**
         * Moves used at start of the action when starting fires with good fuel.
         */
        int moves_cost_fast = 100;

        /**
         * Total moves when starting fires with mediocre fuel.
         */
        int moves_cost_slow = 1000;

        /**
         * Does it need sunlight to be used.
         */
        bool need_sunlight = false;

        static bool prep_firestarter_use( const Character &p, tripoint_bub_ms &pos );
        /** Player here isn't const because pyromaniacs gain a mood boost from it */
        static void resolve_firestarter_use( Character *p, const tripoint_bub_ms &pos );
        /** Modifier on speed - higher is better, 0 means it won't work. */
        float light_mod( const tripoint &pos ) const;
        /** Checks quality of fuel on the tile and interpolates move cost based on that. */
        int moves_cost_by_fuel( const tripoint_bub_ms &pos ) const;

        explicit firestarter_actor( const std::string &type = "firestarter" ) : iuse_actor( type ) {}

        ~firestarter_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        ret_val<void> can_use( const Character &, const item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};

/**
 * Cuts stuff up into components
 */
class salvage_actor : public iuse_actor
{
    public:
        /** Moves used per unit of volume of cut item */
        int moves_per_part = 25;

        std::optional<int> try_to_cut_up( Character &p, item &cutter, item_location &cut ) const;
        int time_to_cut_up( const item &it ) const;
        bool valid_to_cut_up( const Character *p, const item &it ) const;

        explicit salvage_actor( const std::string &type = "salvage" ) : iuse_actor( type ) {}

        ~salvage_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
    private:
        void cut_up( Character &p, item_location &cut ) const;
};

/**
 * Writes on stuff (ground or items)
 */
class inscribe_actor : public iuse_actor
{
    public:
        // Can it write on items/terrain
        bool on_items = true;
        bool on_terrain = false;

        // Does it require target material to be from the whitelist?
        bool material_restricted = true;

        // Materials it can write on
        std::set<material_id> material_whitelist = {
            material_id( "wood" ),
            material_id( "clay" ),
            material_id( "porcelain" ),
            material_id( "plastic" ),
            material_id( "glass" ),
            material_id( "chitin" ),
            material_id( "iron" ),
            material_id( "steel" ),
            material_id( "silver" ),
            material_id( "bone" )
        };

        // How will the inscription be described
        translation verb = to_translation( "Carve" );
        translation gerund = to_translation( "Carved" );

        bool item_inscription( item &tool, item &cut ) const;

        explicit inscribe_actor( const std::string &type = "inscribe" ) : iuse_actor( type ) {}

        ~inscribe_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};

/**
 * Try to turn on a burning melee weapon
 * Not iuse_transform, because they don't have that much in common
 */
class fireweapon_off_actor : public iuse_actor
{
    public:
        itype_id target_id;
        translation success_message = to_translation( "hsss" );
        translation failure_message = to_translation( "hsss" ); // Due to bad roll
        int noise = 0; // If > 0 success message is a success sound instead
        int moves = 0;
        // Lower is better: rng(0, 10) - item.damage_level() > this variable
        int success_chance = INT_MIN;

        fireweapon_off_actor() : iuse_actor( "fireweapon_off" ) {}

        ~fireweapon_off_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        ret_val<void> can_use( const Character &, const item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};

/**
 * Active burning melee weapon
 */
class fireweapon_on_actor : public iuse_actor
{
    public:
        translation noise_message = to_translation( "hsss" ); // If noise is 0, message content instead
        translation charges_extinguish_message;
        translation water_extinguish_message;
        translation auto_extinguish_message;
        int noise_chance = 1; // one_in(this variable)

        explicit fireweapon_on_actor( const std::string &type = "fireweapon_on" ) : iuse_actor( type ) {}

        ~fireweapon_on_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};

/**
 * Makes noise of a given volume
 */
class manualnoise_actor : public iuse_actor
{
    public:
        translation use_message;
        translation noise_message = to_translation( "hsss" );
        std::string noise_id;
        std::string noise_variant;
        int noise = 0; // Should work even with no volume, even if it seems impossible
        int moves = 0;

        explicit manualnoise_actor( const std::string &type = "manualnoise" ) : iuse_actor( type ) {}

        ~manualnoise_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        ret_val<void> can_use( const Character &, const item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};

class play_instrument_iuse : public iuse_actor
{
    public:
        explicit play_instrument_iuse( const std::string &type = "play_instrument" ) : iuse_actor( type ) {}

        ~play_instrument_iuse() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        ret_val<void> can_use( const Character &, const item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};

/**
 * Plays music
 */
class musical_instrument_actor : public iuse_actor
{
    public:
        /**
         * Speed penalty when playing the instrument
         */
        int speed_penalty = 0;
        /**
         * Volume of the music played
         */
        int volume = 0;
        /**
         * Base morale bonus/penalty
         */
        int fun = 0;
        /**
         * Morale bonus scaling (off current perception)
         */
        int fun_bonus = 0;
        /**
        * List of sound descriptions for players
        */
        std::vector<translation> player_descriptions;
        /**
        * List of sound descriptions for NPCs
        */
        std::vector<translation> npc_descriptions;
        /**
         * Display description once per this duration (@ref calendar::once_every).
         */
        time_duration description_frequency = 0_turns;

        explicit musical_instrument_actor( const std::string &type = "musical_instrument" ) : iuse_actor(
                type ) {}

        ~musical_instrument_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        ret_val<void> can_use( const Character &, const item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};

/**
 * Learn a spell
 */
class learn_spell_actor : public iuse_actor
{
    public:
        // list of spell ids that can be learned from this item
        std::vector<std::string> spells;

        explicit learn_spell_actor( const std::string &type = "learn_spell" ) : iuse_actor( type ) {}

        ~learn_spell_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *p, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};

/**
 * Cast a spell. The item's spell level is fixed, and the casting action uses up a charge from the item.
 */
class cast_spell_actor : public iuse_actor
{
    public:
        // this item's spell fail % is 0
        bool no_fail = false;
        // the spell this item casts when used.
        spell_id item_spell;
        int spell_level = 0;
        /**does the item requires to be worn to be activable*/
        bool need_worn = false;
        /**does the item requires to be wielded to be activable*/
        bool need_wielding = false;
        /**is the item mundane in nature*/
        bool mundane = false;

        explicit cast_spell_actor( const std::string &type = "cast_spell" ) : iuse_actor( type ) {}

        ~cast_spell_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *p, item &it, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
        std::string get_name() const override;
};

/**
 * Holster a weapon
 */
class holster_actor : public iuse_actor
{
    public:
        /** Prompt to use when selecting an item */
        translation holster_prompt;
        /** Message to show when holstering an item */
        translation holster_msg;

        /** Check if obj could be stored in the holster */
        bool can_holster( const item &holster, const item &obj ) const;

        /** Store an object in the holster */
        bool store( Character &you, item &holster, item &obj ) const;

        explicit holster_actor( const std::string &type = "holster" ) : iuse_actor( type ) {}

        ~holster_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};

class ammobelt_actor : public iuse_actor
{
    public:
        itype_id belt; /** what type of belt is created with this linkage? */

        ammobelt_actor() : iuse_actor( "ammobelt" ) {}

        ~ammobelt_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};

/**
 * Repair an item
 */
class repair_item_actor : public iuse_actor
{
    public:
        /** Materials we are allowed to repair */
        std::set<material_id> materials;
        /** Skill used */
        skill_id used_skill;
        /** Maximum skill level that can be gained by using this actor. */
        int trains_skill_to = 0;
        /**
          * Volume of materials required (and used up) as percentage of repaired item's volume.
          * Set to 0 to always use just 1 component.
          */
        float cost_scaling = 1.0f;
        /** Extra value added to skill roll */
        int tool_quality = 0;
        /** Move cost for every attempt */
        int move_cost = 0;

        enum attempt_hint : int {
            AS_SUCCESS = 0,     // Success, but can retry
            AS_RETRY,           // Failed, but can retry
            AS_FAILURE,         // Failed hard, don't retry
            AS_DESTROYED,       // Failed and destroyed item
            AS_CANT,            // Couldn't attempt
            AS_CANT_USE_TOOL,   // Cannot use tool
            AS_CANT_YET         // Skill too low
        };

        enum repair_type : int {
            RT_NOTHING = 0,
            RT_REPAIR = 1,          // Just repairing damage
            RT_REFIT = 2,           // Refitting
            RT_DOWNSIZING = 3,      // When small, reduce clothing to small size
            RT_UPSIZING = 4,        // When normal, increase clothing to normal size
            // value of 5 obsoleted, previously used for reinforcing
            RT_PRACTICE = 6,        // Wanted to reinforce, but can't
            NUM_REPAIR_TYPES = 7
        };

        /** Attempts to repair target item with selected tool */
        attempt_hint repair( Character &pl, item &tool, item_location &fix, bool refit_only = false ) const;
        /// Checks if repairs on target item are possible. Excludes checks on tool.
        /// Doesn't just estimate - should not return true if repairs are not possible or false if they are.
        /// @param pl character performing the fix
        /// @param fix item to repair
        /// @param print_msg prints message to player if check fails
        /// @param check_consumed_available if true check materials consumed by repair are available for `pl`
        /// @return true if check succeeds
        bool can_repair_target( Character &pl, const item &fix, bool print_msg,
                                bool check_consumed_available ) const;
        /** Checks if we are allowed to use the tool. */
        bool can_use_tool( const Character &p, const item &tool, bool print_msg ) const;

        /** Returns a list of items that can be used to repair this item. */
        std::set<itype_id> get_valid_repair_materials( const item &fix ) const;
        /// Returns if components are available. Consumes them if `just_check` is false.
        /// @param pl character performing the fix
        /// @param fix item to repair
        /// @param print_msg prints message to player if check fails
        /// @param just_check only checks, doesn't consume
        /// @param check_consumed_available if false skips checking if materials consumed by repair are available for `pl`
        /// @return true if check succeeds
        bool handle_components( Character &pl, const item &fix, bool print_msg, bool just_check,
                                bool check_consumed_available ) const;
        /** Returns the chance to repair and to damage an item. */
        std::pair<float, float> repair_chance(
            const Character &pl, const item &fix, repair_type action_type ) const;
        /** What are we most likely trying to do with this item? */
        repair_type default_action( const item &fix, int current_skill_level ) const;
        /**
         * Calculates the difficulty to repair an item based on its materials.
         * Normally the difficulty to repair is that of the most difficult material.
         * If the item has a repairs_like, use the materials of that item instead.
         */
        int repair_recipe_difficulty( const item &fix ) const;
        /** Describes members of `repair_type` enum */
        static std::string action_description( repair_type );

        explicit repair_item_actor( const std::string &type = "repair_item" ) : iuse_actor( type ) {}

        ~repair_item_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;

        std::string get_name() const override;
        std::string get_description() const override;
};

class heal_actor : public iuse_actor
{
    public:
        /** How much hp to restore when healing limbs? */
        float limb_power = 0.0f;
        /** How much hp to restore when healing head? */
        float head_power = 0.0f;
        /** How much hp to restore when healing torso? */
        float torso_power = 0.0f;
        /** How many intensity levels will be applied when healing limbs? */
        float bandages_power = 0.0f;
        /** Extra intensity levels gained per skill level when healing limbs. */
        float bandages_scaling = 0.0f;
        /** How many intensity levels will be applied when healing limbs? */
        float disinfectant_power = 0.0f;
        /** Extra intensity levels gained per skill level when healing limbs. */
        float disinfectant_scaling = 0.0f;
        /** How many levels of bleeding effect intensity can it remove (dressing efficiency). */
        int bleed = 0;
        /** Chance to remove bite effect. */
        float bite = 0.0f;
        /** Chance to remove infected effect. */
        float infect = 0.0f;
        /** Cost in moves to use the item. */
        int move_cost = 100;
        /** Extra hp gained per skill level when healing limbs. */
        float limb_scaling = 0.0f;
        /** Extra hp gained per skill level when healing head. */
        float head_scaling = 0.0f;
        /** Extra hp gained per skill level when healing torso. */
        float torso_scaling = 0.0f;
        /** Effects to apply to patient on finished healing. */
        std::vector<effect_data> effects;
        /**
         * Item produced on finished healing. For example, bloody rag.
         * If the used item is a tool it, it will be turned into the used up item.
         * If it is not a tool a new item with this id will be created.
         */
        itype_id used_up_item_id;
        int used_up_item_quantity = 1;
        int used_up_item_charges = 1;
        std::set<flag_id> used_up_item_flags;

        /** How much hp would `healer` heal using this actor on `healed` body part. */
        int get_heal_value( const Character &healer, bodypart_id healed ) const;
        /** How many intensity levels will be applied using this actor by `healer`. */
        int get_bandaged_level( const Character &healer ) const;
        /** How many intensity levels will be applied using this actor by `healer`. */
        int get_disinfected_level( const Character &healer ) const;
        /** How many intensity levels of bleeding will be reduced using this actor by `healer`. */
        int get_stopbleed_level( const Character &healer ) const;
        /** Does the actual healing. Returns charges used. */
        int finish_using( Character &healer, Character &patient, item &it, bodypart_id healed ) const;

        bodypart_id use_healing_item( Character &healer, Character &patient, item &it, bool force ) const;

        explicit heal_actor( const std::string &type = "heal" ) : iuse_actor( type ) {}

        ~heal_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};

class place_trap_actor : public iuse_actor
{
    public:
        struct data {
            data();
            trap_str_id trap;
            /** The message shown when the trap has been set. */
            translation done_message;
            /** Amount of practice of the "trap" skill. */
            int practice = 0;
            /** Move points that are used when placing the trap. */
            int moves = 100;
            void load( const JsonObject &obj );
        };
        /** Whether one can place the trap when underwater. */
        bool allow_underwater = false;
        /** Whether one can place the trap directly under the character itself. */
        bool allow_under_player = false;
        /** Whether the trap needs solid neighbor squares (e.g. for trap wire). */
        bool needs_solid_neighbor = false;
        /**
         * Contains a terrain id of the terrain that must exist in a neighbor square to allow
         * placing this trap. If empty, it is ignored. This is for example for snare traps.
         */
        ter_str_id needs_neighbor_terrain;
        /** Data that applies to unburied traps and to traps that *can * not be buried. */
        data unburied_data;
        /**
         * Contains the question asked when the player can bury the trap. Something like "Bury the trap?"
         */
        translation bury_question;
        /** Data that applies to buried traps. */
        data buried_data;
        /**
         * The trap that makes up the outer layer of a multi-tile trap. This is not supported for buried traps!
         */
        trap_str_id outer_layer_trap;
        bool is_allowed( Character &p, const tripoint &pos, const std::string &name ) const;

        explicit place_trap_actor( const std::string &type = "place_trap" );
        ~place_trap_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};

class emit_actor : public iuse_actor
{
    public:
        std::set<emit_id> emits;
        /** If true multiplies the emits by number of charges on the item. */
        bool scale_qty = false;

        explicit emit_actor( const std::string &type = "emit_actor" ) : iuse_actor( type ) {}
        ~emit_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void finalize( const itype_id &my_item_type ) override;
};

class saw_barrel_actor : public iuse_actor
{
    public:
        explicit saw_barrel_actor( const std::string &type = "saw_barrel" ) : iuse_actor( type ) {}

        void load( const JsonObject &jo ) override;
        std::optional<int> use( Character *p, item &it, const tripoint &pnt ) const override;
        std::unique_ptr<iuse_actor> clone() const override;

        ret_val<void> can_use_on( const Character &you, const item &it, const item &target ) const;
};

class saw_stock_actor : public iuse_actor
{
    public:
        explicit saw_stock_actor( const std::string &type = "saw_stock" ) : iuse_actor( type ) {}

        void load( const JsonObject &jo ) override;
        std::optional<int> use( Character *p, item &it, const tripoint &pnt ) const override;
        std::unique_ptr<iuse_actor> clone() const override;

        ret_val<void> can_use_on( const Character &you, const item &it, const item &target ) const;
};

// this adds a pocket to a molle item
class molle_attach_actor : public iuse_actor
{
    public:
        explicit molle_attach_actor( const std::string &type = "attach_molle" ) : iuse_actor( type ) {}

        int size;
        int moves;

        void load( const JsonObject &jo ) override;
        std::optional<int> use( Character *p, item &it, const tripoint &pnt ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};

// this removes a pocket from a molle item
class molle_detach_actor : public iuse_actor
{
    public:
        explicit molle_detach_actor( const std::string &type = "detach_molle" ) : iuse_actor( type ) {}

        int moves;

        void load( const JsonObject &jo ) override;
        std::optional<int> use( Character *p, item &it, const tripoint &pnt ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};

class install_bionic_actor : public iuse_actor
{
    public:
        explicit install_bionic_actor( const std::string &type = "install_bionic" ) : iuse_actor( type ) {}

        void load( const JsonObject & ) override {}
        std::optional<int> use( Character *p, item &it, const tripoint &pnt ) const override;
        ret_val<void> can_use( const Character &, const item &it, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void finalize( const itype_id &my_item_type ) override;
};

class detach_gunmods_actor : public iuse_actor
{
    public:
        explicit detach_gunmods_actor( const std::string &type = "detach_gunmods" ) : iuse_actor( type ) {}

        void load( const JsonObject & ) override {}
        std::optional<int> use( Character *p, item &it, const tripoint &pnt ) const override;
        ret_val<void> can_use( const Character &, const item &it, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void finalize( const itype_id &my_item_type ) override;
};

class modify_gunmods_actor : public iuse_actor
{
    public:
        explicit modify_gunmods_actor( const std::string &type = "modify_gunmods" ) : iuse_actor( type ) {}

        void load( const JsonObject & ) override {}
        std::optional<int> use( Character *p, item &it, const tripoint &pnt ) const override;
        ret_val<void> can_use( const Character &, const item &it, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void finalize( const itype_id &my_item_type ) override;
};

class link_up_actor : public iuse_actor
{
    public:
        /** Maximum length of the cable. At -1, will use the item type's max_charges. */
        int cable_length = -1;
        /** Charge rate in watts */
        units::power charge_rate = 0_W;
        /** (this) out of 1.0 chance to successfully add 1 charge every charge interval */
        float efficiency = 0.85f;
        /** (Optional) The move cost to attach the cable. */
        int move_cost = 5;
        /** (Optional) Text displayed in the activation screen, defaults to "Plug in / Manage cables". */
        translation menu_text;

        std::set<link_state> targets = { link_state::no_link, link_state::vehicle_port };
        std::set<std::string> can_extend = {};

        std::optional<int> link_to_veh_app( Character *p, item &it, bool to_ports ) const;
        std::optional<int> link_tow_cable( Character *p, item &it, bool to_towing ) const;
        std::optional<int> link_extend_cable( Character *p, item &it, const tripoint &pnt ) const;
        std::optional<int> remove_extensions( Character *p, item &it ) const;

        link_up_actor() : iuse_actor( "link_up" ) {}

        ~link_up_actor() override = default;
        void load( const JsonObject &jo ) override;
        std::optional<int> use( Character *p, item &it, const tripoint &pnt ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
        std::string get_name() const override;
};

class deploy_tent_actor : public iuse_actor
{
    public:
        string_id<furn_t> wall;
        string_id<furn_t> floor;
        std::optional<string_id<furn_t>> floor_center;
        string_id<furn_t> door_opened;
        string_id<furn_t> door_closed;
        int radius = 1;
        std::optional<itype_id> broken_type;

        deploy_tent_actor() : iuse_actor( "deploy_tent" ) {}

        ~deploy_tent_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;

        bool check_intact( const tripoint &center ) const;
};

/**
* Weigh yourself on a bathroom scale. or something.
*/
class weigh_self_actor : public iuse_actor
{
    public:
        // max weight this device can handle before showing "error"
        units::mass max_weight;

        explicit weigh_self_actor( const std::string &type = "weigh_self" ) : iuse_actor( type ) {}

        ~weigh_self_actor() override = default;
        void load( const JsonObject &jo ) override;
        std::optional<int> use( Character *p, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};

/**
 * Modify clothing
 */
class sew_advanced_actor : public iuse_actor
{
    public:
        /** Materials */
        std::set<material_id> materials;
        /** Clothing mods */
        std::vector<clothing_mod_id> clothing_mods;
        /** Skill used */
        skill_id used_skill;

        explicit sew_advanced_actor( const std::string &type = "sew_advanced" ) : iuse_actor( type ) {}

        ~sew_advanced_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *, item &, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
};

/**
 * Activate an array of effect_on_conditions
 */
class effect_on_conditons_actor : public iuse_actor
{
    public:
        std::vector<effect_on_condition_id> eocs;
        translation description;
        translation menu_text;
        explicit effect_on_conditons_actor( const std::string &type = "effect_on_conditions" ) : iuse_actor(
                type ) {}

        ~effect_on_conditons_actor() override = default;
        void load( const JsonObject &obj ) override;
        std::optional<int> use( Character *p, item &it, const tripoint & ) const override;
        std::unique_ptr<iuse_actor> clone() const override;
        std::string get_name() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};
#endif // CATA_SRC_IUSE_ACTOR_H

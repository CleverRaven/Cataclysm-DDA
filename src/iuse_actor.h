#ifndef IUSE_ACTOR_H
#define IUSE_ACTOR_H

#include "iuse.h"
#include "game_constants.h"
#include "color.h"
#include "bodypart.h"
#include "string_id.h"
#include "explosion.h"
#include "vitamin.h"
#include <limits.h>

struct vehicle_prototype;
using vproto_id = string_id<vehicle_prototype>;
enum field_id : int;
enum hp_part : int;
struct mtype;
using mtype_id = string_id<mtype>;
class JsonObject;
class Skill;
using skill_id = string_id<Skill>;
class effect_type;
using efftype_id = string_id<effect_type>;
using ammotype = std::string;
using itype_id = std::string;
class material_type;
using material_id = string_id<material_type>;

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
        std::string msg_transform;

        /** type of the resulting item */
        std::string target;

        /** if set transform item to container and place new item (of type @ref target) inside */
        std::string container;

        /** if zero or positive set remaining ammo of @ref target to this (after transformation) */
        long ammo_qty = -1;

        /** if both this and ammo_qty are specified then set @ref target to this specific ammo */
        std::string ammo_type;

        /** used to set the active property of the transformed @ref target */
        bool active = false;

        /** subtracted from @ref Creature::moves when transformation is successful */
        int moves = 0;

        /** minimum number of fire charges required (if any) for transformation */
        long need_fire = 0;

        /** displayed if item is in player possession with %s replaced by item name */
        std::string need_fire_msg;

        /** minimum charges (if any) required for transformation */
        long need_charges = 0;

        /** displayed if item is in player possession with %s replaced by item name */
        std::string need_charges_msg;

        std::string menu_option_text;

        virtual ~iuse_transform();
        virtual void load( JsonObject &jo );
        virtual long use(player *, item *, bool, const tripoint& ) const override;
        virtual iuse_actor *clone() const override;
        std::string get_name() const override;
};

/**
 * This is a @ref iuse_transform for active items.
 * It can be called each turn.
 * It does the transformation - either when requested by the user,
 * or when the charges of the item reaches 0.
 * It can display different messages therefor.
 */
class auto_iuse_transform : public iuse_transform
{
    public:
        /**
         * If non-empty: check each turn if the player is underwater
         * and activate the transformation in that case.
         */
        std::string when_underwater;
        /**
         * If non-empty: don't let the user activate the transformation.
         * Instead wait for the item to trigger the transformation
         * (no charges, underwater).
         */
        std::string non_interactive_msg;

        auto_iuse_transform()
            : iuse_transform()
        {
        }
        virtual ~auto_iuse_transform();
        virtual void load( JsonObject &jo ) override;
        virtual long use(player *, item *, bool, const tripoint& ) const override;
        virtual iuse_actor *clone() const override;
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

        /** Maximum percentage of count that should be dropped within area of effect */
        int shrapnel_recovery = 0;
        /** What type of shrapnel to drop */
        itype_id shrapnel_drop = "null";

        // Those 2 values are forwarded to game::draw_explosion,
        // Nothing is drawn if radius < 0 (game::explosion might still draw something)
        int draw_explosion_radius;
        nc_color draw_explosion_color;
        /** Call game::flashbang? */
        bool do_flashbang;
        bool flashbang_player_immune;
        /** Create fields of this type around the center of the explosion */
        int fields_radius;
        field_id fields_type;
        int fields_min_density;
        int fields_max_density;
        /** Calls game::emp_blast if >= 0 */
        int emp_blast_radius;
        /** Calls game::scrambler_blast if >= 0 */
        int scrambler_blast_radius;
        /** Volume of sound each turn, -1 means no sound at all */
        int sound_volume;
        std::string sound_msg;
        /** Message shown when the player tries to deactivate the item,
         * which is not allowed. */
        std::string no_deactivate_msg;

        explosion_iuse()
            : iuse_actor()
            , draw_explosion_radius(-1)
            , draw_explosion_color(c_white)
            , do_flashbang(false)
            , flashbang_player_immune(false) // false is the default in game.h
            , fields_radius(-1)
            , fields_type()
            , fields_min_density(1)
            , fields_max_density(3)
            , emp_blast_radius(-1)
            , scrambler_blast_radius(-1)
            , sound_volume(-1)
        {
        }
        virtual ~explosion_iuse();
        virtual void load( JsonObject &jo );
        virtual long use(player *, item *, bool, const tripoint& ) const override;
        virtual iuse_actor *clone() const override;
};

/**
 * This iuse creates a new vehicle on the map.
 */
class unfold_vehicle_iuse : public iuse_actor
{
    public:
        /** Id of the vehicle prototype (@see map::add_vehicle what it expects) that will be
         * created when unfolding the item. */
        vproto_id vehicle_id;
        /** Message shown after successfully unfolding the item. */
        std::string unfold_msg;
        /** Creature::moves it takes to unfold. */
        int moves;
        std::map<std::string, int> tools_needed;
        unfold_vehicle_iuse()
            : iuse_actor()
            , moves(0)
        {
        }
        virtual ~unfold_vehicle_iuse();
        virtual void load( JsonObject &jo );
        virtual long use(player *, item *, bool, const tripoint& ) const override;
        virtual iuse_actor *clone() const override;
};

/** Used in consume_drug_iuse for storing effect data. */
struct effect_data
{
    efftype_id id;
    int duration;
    body_part bp;
    bool permanent;

    effect_data(const efftype_id &nid, int dur, body_part nbp, bool perm) :
                    id(nid), duration(dur), bp(nbp), permanent(perm) {};
};

/**
 * This iuse encapsulates the effects of taking a drug.
 */
class consume_drug_iuse : public iuse_actor
{
    public:
        /** Message to display when drug is consumed. **/
        std::string activation_message;
        /** Fields to produce when you take the drug, mostly intended for various kinds of smoke. **/
        std::map<std::string, int> fields_produced;
        /** Tool charges needed to take the drug, e.g. fire. **/
        std::map<std::string, int> charges_needed;
        /** Tools needed, but not consumed, e.g. "smoking apparatus". **/
        std::map<std::string, int> tools_needed;
        /** An effect or effects (conditions) to give the player for the stated duration. **/
        std::vector<effect_data> effects;
        /** A list of stats and adjustments to them. **/
        std::map<std::string, int> stat_adjustments;

        /** Modify player @vitamin_levels by random amount between min (first) and max (second) */
        std::map<vitamin_id, std::pair<int,int>> vitamins;

        /** How many move points this action takes. */
        int moves;

        consume_drug_iuse() : iuse_actor(), moves(100) { }
        virtual ~consume_drug_iuse();
        virtual void load( JsonObject &jo );
        virtual long use(player *, item *, bool, const tripoint& ) const override;
        virtual iuse_actor *clone() const override;
        virtual void info( const item &, std::vector<iteminfo> & ) const override;
};

/**
 * This is a @ref iuse_transform for similar to @ref auto_iuse_transform,
 * but it uses the age of the item instead of a counter.
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
        int transform_age;
        /**
         * Message to display when the user activates the item before the
         * age has been reached.
         */
        std::string not_ready_msg;

        /** How much longer (in turns) until the transformation can be done, can be negative. */
        int time_to_do( const item &it ) const;

        delayed_transform_iuse() : iuse_transform(), transform_age(0) { }
        virtual ~delayed_transform_iuse();
        virtual void load( JsonObject &jo ) override;
        virtual long use( player *, item *, bool, const tripoint& ) const override;
        virtual iuse_actor *clone() const override;
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
        bool place_randomly;
        /** How many move points this action takes. */
        int moves;
        /** Difficulty of programming the monster (to be friendly). */
        int difficulty;
        /** Shown when programming the monster succeeded and it's friendly. Can be empty. */
        std::string friendly_msg;
        /** Shown when programming the monster failed and it's hostile. Can be empty. */
        std::string hostile_msg;
        /** Skills used to make the monster not hostile when activated. **/
        skill_id skill1 = NULL_ID;
        skill_id skill2 = NULL_ID;

        place_monster_iuse() : iuse_actor(), place_randomly( false ), moves( 100 ), difficulty( 0 ) { }
        virtual ~place_monster_iuse();
        virtual void load( JsonObject &jo );
        virtual long use(player *, item *, bool, const tripoint& ) const override;
        virtual iuse_actor *clone() const override;
};

/**
 * Items that can be worn and can be activated to consume energy from UPS.
 * Note that the energy consumption is done in @ref player::process_active_items, it is
 * *not* done by this class!
 */
class ups_based_armor_actor : public iuse_actor
{
    public:
        /** Shown when activated. */
        std::string activate_msg;
        /** Shown when deactivated. */
        std::string deactive_msg;
        /** Shown when it runs out of power. */
        std::string out_of_power_msg;

        ups_based_armor_actor() : iuse_actor() { }
        virtual ~ups_based_armor_actor();
        virtual void load( JsonObject &jo );
        virtual long use(player *, item *, bool, const tripoint& ) const override;
        virtual iuse_actor *clone() const override;
};

/**
 * This implements lock picking.
 */
class pick_lock_actor : public iuse_actor
{
    public:
        /**
         * How good the used tool is at picking a lock.
         */
        int pick_quality;

        pick_lock_actor() : iuse_actor(), pick_quality( 0 ) { }
        virtual ~pick_lock_actor();
        virtual void load( JsonObject &jo );
        virtual long use(player *, item *, bool, const tripoint& ) const override;
        virtual iuse_actor *clone() const override;
};

/**
 * Reveals specific things on the overmap.
 */
class reveal_map_actor : public iuse_actor
{
    public:
        /**
         * The radius of the overmap area that gets revealed.
         * This is in overmap terrain coordinates. A radius of 1 means all terrains directly around
         * the character are revealed.
         */
        int radius;
        /**
         * Overmap terrain types that get revealed.
         */
        std::vector<std::string> omt_types;
        /**
         * The message displayed after revealing.
         */
        std::string message;

        void reveal_targets( tripoint const & center, const std::string &target, int reveal_distance ) const;

        reveal_map_actor() : iuse_actor(), radius( 0 ) { }
        virtual ~reveal_map_actor();
        virtual void load( JsonObject &jo );
        virtual long use(player *, item *, bool, const tripoint& ) const override;
        virtual iuse_actor *clone() const override;
};

/**
 * Starts a fire instantly
 */
class firestarter_actor : public iuse_actor
{
    public:
        /**
         * Moves used at start of the action.
         */
        int moves_cost;

        static bool prep_firestarter_use( const player *p, const item *it, tripoint &pos );
        static void resolve_firestarter_use( const player *p, const item *, const tripoint &pos );

        firestarter_actor() : iuse_actor(), moves_cost( 0 ) { }
        virtual ~firestarter_actor() { }
        virtual void load( JsonObject &jo );
        virtual long use( player*, item*, bool, const tripoint& ) const override;
        virtual bool can_use( const player*, const item*, bool, const tripoint& ) const override;
        virtual iuse_actor *clone() const override;
};

/**
 * Starts an extended action to start a fire
 */
class extended_firestarter_actor : public firestarter_actor
{
    public:
        /**
         * Does it need sunlight to be used.
         */
        bool need_sunlight;

        int calculate_time_for_lens_fire( const player *, float light_level ) const;

        extended_firestarter_actor() : firestarter_actor(), need_sunlight( false ) { }
        virtual ~extended_firestarter_actor() { }
        virtual void load( JsonObject &jo ) override;
        virtual long use( player*, item*, bool, const tripoint& ) const override;
        virtual bool can_use( const player*, const item*, bool, const tripoint& ) const override;
        virtual iuse_actor *clone() const override;
};

/**
 * Cuts stuff up into components
 */
class salvage_actor : public iuse_actor
{
    public:
        /**
         * Moves used per unit of volume of cut item.
         */
        int moves_per_part;
        /**
         * Materials it can cut.
         */
        std::vector<material_id> material_whitelist;

        bool try_to_cut_up( player *p, item *it ) const;
        int cut_up( player *p, item *it, item *cut ) const;
        bool valid_to_cut_up( const item *it ) const;

        salvage_actor() : iuse_actor(), moves_per_part( 25 ) { }
        virtual ~salvage_actor() { }
        virtual void load( JsonObject &jo );
        virtual long use( player*, item*, bool, const tripoint& ) const override;
        virtual iuse_actor *clone() const override;
};

/**
 * Writes on stuff (ground or items)
 */
class inscribe_actor : public iuse_actor
{
    public:
        // Can it write on items/terrain
        bool on_items;
        bool on_terrain;

        // Does it require target material to be from the whitelist?
        bool material_restricted;

        // Materials it can write on
        std::vector<material_id> material_whitelist;

        // How will the inscription be described
        std::string verb; // "Write", "Carve"
        std::string gerund; // "Written", "Carved"

        bool item_inscription( item *cut ) const;

        inscribe_actor() : iuse_actor(), on_items( true ), on_terrain( false ), material_restricted( true ) { }
        virtual ~inscribe_actor() { }
        virtual void load( JsonObject &jo );
        virtual long use( player*, item*, bool, const tripoint& ) const override;
        virtual iuse_actor *clone() const override;
};

/**
 * Cauterizes a wounded/masochistic survivor
 */
class cauterize_actor : public iuse_actor
{
    public:
        // Use flame. If false, uses item charges instead.
        bool flame;

        static bool cauterize_effect( player *p, item *it, bool force );

        cauterize_actor() : iuse_actor(), flame( true ) { }
        virtual ~cauterize_actor() { }
        virtual void load( JsonObject &jo );
        virtual long use( player*, item*, bool, const tripoint& ) const override;
        virtual bool can_use( const player*, const item*, bool, const tripoint& ) const override;
        virtual iuse_actor *clone() const override;
};

/**
 * Makes a zombie corpse into a zombie slave
 */
class enzlave_actor : public iuse_actor
{
    public:
        enzlave_actor() : iuse_actor() { }
        virtual ~enzlave_actor() { }
        virtual void load( JsonObject &jo );
        virtual long use( player*, item*, bool, const tripoint& ) const override;
        virtual bool can_use( const player*, const item*, bool, const tripoint& ) const override;
        virtual iuse_actor *clone() const override;
};

/**
 * Try to turn on a burning melee weapon
 * Not iuse_transform, because they don't have that much in common
 */
class fireweapon_off_actor : public iuse_actor
{
    public:
        std::string target_id;
        std::string success_message;
        std::string lacks_fuel_message;
        std::string failure_message; // Due to bad roll
        int noise; // If > 0 success message is a success sound instead
        int moves;
        int success_chance; // Lower is better: rng(0, 10) - item.damage > this variable

        fireweapon_off_actor() : iuse_actor(), noise(0), moves(0), success_chance(INT_MIN) { }
        virtual ~fireweapon_off_actor() { }
        virtual void load( JsonObject &jo );
        virtual long use( player*, item*, bool, const tripoint& ) const override;
        virtual bool can_use( const player*, const item*, bool, const tripoint& ) const override;
        virtual iuse_actor *clone() const override;
};

/**
 * Active burning melee weapon
 */
class fireweapon_on_actor : public iuse_actor
{
    public:
        std::string noise_message; // If noise is 0, message content instead
        std::string voluntary_extinguish_message;
        std::string charges_extinguish_message;
        std::string water_extinguish_message;
        std::string auto_extinguish_message;
        int noise; // If 0, it produces a message instead of noise
        int noise_chance; // one_in(this variable)
        int auto_extinguish_chance; // one_in(this) per turn to fail

        fireweapon_on_actor() : iuse_actor(), noise(0), noise_chance(1) { }
        virtual ~fireweapon_on_actor() { }
        virtual void load( JsonObject &jo );
        virtual long use( player*, item*, bool, const tripoint& ) const override;
        virtual iuse_actor *clone() const override;
};

/**
 * Makes noise of a given volume
 */
class manualnoise_actor : public iuse_actor
{
    public:
        std::string no_charges_message;
        std::string use_message;
        std::string noise_message;
        int noise; // Should work even with no volume, even if it seems impossible
        int moves;

        manualnoise_actor() : iuse_actor(), noise(0), moves(0) { }
        virtual ~manualnoise_actor() { }
        virtual void load( JsonObject &jo );
        virtual long use( player*, item*, bool, const tripoint& ) const override;
        virtual bool can_use( const player*, const item*, bool, const tripoint& ) const override;
        virtual iuse_actor *clone() const override;
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
        int speed_penalty;
        /**
         * Volume of the music played
         */
        int volume;
        /**
         * Base morale bonus/penalty
         */
        int fun;
        /**
         * Morale bonus scaling (off current perception)
         */
        int fun_bonus;
        /**
         * List of sound descriptions
         */
        std::vector< std::string > descriptions;
        /**
         * Display description once per this many turns
         */
        int description_frequency;

        musical_instrument_actor() = default;
        virtual ~musical_instrument_actor() = default;
        virtual void load( JsonObject &jo );
        virtual long use( player*, item*, bool, const tripoint& ) const override;
        virtual bool can_use( const player*, const item*, bool, const tripoint& ) const override;
        virtual iuse_actor *clone() const override;
};

/**
 * Holster a weapon
 */
class holster_actor : public iuse_actor
{
    public:
        /** Prompt to use when selecting an item */
        std::string holster_prompt;
        /** Message to show when holstering an item */
        std::string holster_msg;
        /** Maximum volume of each item that can be holstered */
        int max_volume;
        /** Minimum volume of each item that can be holstered or 1/3 max_volume if unspecified */
        int min_volume;
        /** Maximum weight of each item. If unspecified no weight limit is imposed */
        int max_weight;
        /** Total number of items that holster can contain **/
        int multi;
        /** Base move cost per unit volume when wielding the contained item */
        int draw_cost;
        /** Guns using any of these skills can be holstered */
        std::vector<skill_id> skills;
        /** Items with any of these flags set can be holstered */
        std::vector<std::string> flags;

        /** Check if obj could be stored in the holster */
        bool can_holster( const item& obj ) const;

        /** Store an object in the holster */
        bool store( player &p, item& holster, item& obj ) const;

        holster_actor() : iuse_actor(), max_weight( -1 ), multi( 1 ), draw_cost( VOLUME_MOVE_COST ) { }
        virtual ~holster_actor() { }
        virtual void load( JsonObject &jo );
        virtual long use( player *, item *, bool, const tripoint & ) const override;
        virtual iuse_actor *clone() const override;
};

/**
 * Store ammo and later reload using it
 */
class bandolier_actor : public iuse_actor
{
    public:
        /** Total number of rounds that can be stored **/
        int capacity = 1;
        /** What types of ammo can be stored? */
        std::set<ammotype> ammo;

        /** Check if obj could be stored in the bandolier */
        bool can_store( const item& bandolier, const item& obj ) const;

        /** Store ammo in the bandolier */
        bool store( player &p, item& bandolier, item& obj ) const;

        virtual ~bandolier_actor() { }
        virtual void load( JsonObject &jo );
        virtual long use( player *, item *, bool, const tripoint & ) const override;
        virtual iuse_actor *clone() const override;
        virtual void info( const item &, std::vector<iteminfo> & ) const override;
};

class ammobelt_actor : public iuse_actor
{
    public:
        itype_id belt; /** what type of belt is created with this linkage? */

        virtual ~ammobelt_actor() { }
        virtual void load( JsonObject &jo );
        virtual long use( player *, item *, bool, const tripoint & ) const override;
        virtual iuse_actor *clone() const override;
        virtual void info( const item &, std::vector<iteminfo> & ) const override;
};

/**
 * Repair an item
 */
class repair_item_actor : public iuse_actor
{
    public:
        /** Materials we are allowed to repair */
        std::vector<material_id> materials;
        /** Skill used */
        skill_id used_skill;
        /**
          * Volume of materials required (and used up) as percentage of repaired item's volume.
          * Set to 0 to always use just 1 component.
          */
        float cost_scaling;
        /** Extra value added to skill roll */
        int tool_quality;
        /** Move cost for every attempt */
        int move_cost;

        enum attempt_hint : int {
            AS_SUCCESS = 0,     // Success, but can retry
            AS_RETRY,           // Failed, but can retry
            AS_FAILURE,         // Failed hard, don't retry
            AS_DESTROYED,       // Failed and destroyed item
            AS_CANT,            // Couldn't attempt
            AS_CANT_YET         // Skill too low
        };

        enum repair_type : int {
            RT_NOTHING = 0,
            RT_REPAIR,          // Just repairing damage
            RT_REFIT,           // Adding (fits) tag
            RT_REINFORCE,       // Getting damage below 0
            RT_PRACTICE,        // Wanted to reinforce, but can't
            NUM_REPAIR_TYPES
        };

        /** Attempts to repair target item with selected tool */
        attempt_hint repair( player &pl, item &tool, item &target ) const;
        /** Checks if repairs are possible.
          * Doesn't just estimate - should not return true if repairs are not possible or false if they are. */
        bool can_repair( player &pl, const item &tool, const item &target, bool print_msg ) const;
        /** Returns if components are available. Consumes them if `just_check` is false. */
        bool handle_components( player &pl, const item &fix, bool print_msg, bool just_check ) const;
        /** Returns the chance to repair and to damage an item. */
        std::pair<float, float> repair_chance(
            const player &pl, const item &fix, repair_type action_type ) const;
        /** What are we most likely trying to do with this item? */
        repair_type default_action( const item &fix ) const;
        /**
         * Calculates the difficulty to repair an item
         * based on recipes to craft it and player's knowledge of them.
         * If `training` is true, player's lacking knowledge and skills are not used to increase difficulty.
         */
        int repair_recipe_difficulty( const player &pl, const item &fix, bool training = false ) const;
        /** Describes members of `repair_type` enum */
        static const std::string &action_description( repair_type );

        repair_item_actor() : iuse_actor() { }
        virtual ~repair_item_actor() { }
        virtual void load( JsonObject &jo );
        virtual long use( player *, item *, bool, const tripoint & ) const override;
        virtual iuse_actor *clone() const override;
};

class heal_actor : public iuse_actor
{
    public:
        /** How much hp to restore when healing limbs? */
        float limb_power;
        /** How much hp to restore when healing head? */
        float head_power;
        /** How much hp to restore when healing torso? */
        float torso_power;
        /** Chance to remove bleed effect. */
        float bleed;
        /** Chance to remove bite effect. */
        float bite;
        /** Chance to remove infected effect. */
        float infect;
        /** Cost in moves to use the item. */
        int move_cost;
        /** Is using this item a long action. */
        bool long_action;
        /** Extra hp gained per skill level when healing limbs. */
        float limb_scaling;
        /** Extra hp gained per skill level when healing head. */
        float head_scaling;
        /** Extra hp gained per skill level when healing torso. */
        float torso_scaling;
        /** Effects to apply to patient on finished healing. */
        std::vector<effect_data> effects;
        /**
         * Item produced on finished healing. For example, bloody rag.
         * If the used item is a tool it, it will be turned into the used up item.
         * If it is not a tool a new item with this id will be created.
         */
        std::string used_up_item;

        /** How much hp would `healer` heal using this actor on `healed` body part. */
        int get_heal_value( const player &healer, hp_part healed ) const;

        /** Does the actual healing. Used by both long and short actions. Returns charges used. */
        long finish_using( player &healer, player &patient, item &it, hp_part part ) const;

        hp_part use_healing_item( player &healer, player &patient, item &it, bool force ) const;

        heal_actor()
            : iuse_actor()
            , limb_power( 0 )
            , head_power( 0 )
            , torso_power( 0 )
            , bleed( 0.0f )
            , bite( 0.0f )
            , infect( 0.0f )
            , move_cost( 100 )
            , long_action( false )
            , limb_scaling( 0.0f )
            , head_scaling( 0.0f )
            , torso_scaling( 0.0f )
            { }
        virtual ~heal_actor() { }
        virtual void load( JsonObject &jo );
        virtual long use( player *, item *, bool, const tripoint & ) const override;
        virtual iuse_actor *clone() const override;
        virtual void info( const item &, std::vector<iteminfo> & ) const override;
};

#endif

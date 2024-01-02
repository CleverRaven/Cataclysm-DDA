#pragma once
#ifndef CATA_SRC_MTYPE_H
#define CATA_SRC_MTYPE_H

#include <iosfwd>
#include <array>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "behavior.h"
#include "calendar.h"
#include "color.h"
#include "damage.h"
#include "enum_bitset.h"
#include "enums.h"
#include "magic.h"
#include "mattack_common.h"
#include "pathfinding.h"
#include "shearing.h"
#include "speed_description.h"
#include "translations.h"
#include "type_id.h"
#include "units.h" // IWYU pragma: keep
#include "weakpoint.h"

class Creature;
class monster;
struct dealt_projectile_attack;
template <typename E> struct enum_traits;

enum class creature_size : int;

using mon_action_death  = void ( * )( monster & );
using mon_action_attack = bool ( * )( monster * );
using mon_action_defend = void ( * )( monster &, Creature *, dealt_projectile_attack const * );
using bodytype_id = std::string;
class JsonArray;
class JsonObject;

// These are triggers which may affect the monster's anger or morale.
// They are handled in monster::check_triggers(), in monster.cpp
enum class mon_trigger : int {
    STALK,              // Increases when following the player
    HOSTILE_WEAK,       // Hurt hostile player/npc/monster seen
    HOSTILE_CLOSE,      // Hostile creature within a few tiles
    HOSTILE_SEEN,       // Hostile creature in visual range
    HURT,               // We are hurt
    FIRE,               // Fire nearby
    FRIEND_DIED,        // A monster of the same type died
    FRIEND_ATTACKED,    // A monster of the same type attacked
    SOUND,              // Heard a sound
    PLAYER_NEAR_BABY,   // Player/npc is near a baby monster of this type
    MATING_SEASON,      // It's the monster's mating season (defined by baby_flags)
    BRIGHT_LIGHT,       // Illumination in the monster's tile is 75% of full daylight or higher

    LAST               // This item must always remain last.
};

template<>
struct enum_traits<mon_trigger> {
    static constexpr mon_trigger last = mon_trigger::LAST;
};

struct mon_flag {
    mon_flag_str_id id = mon_flag_str_id::NULL_ID();
    bool was_loaded = false;

    void load( const JsonObject &jo, std::string_view src );
    static void load_mon_flags( const JsonObject &jo, const std::string &src );
    static void reset();
    static const std::vector<mon_flag> &get_all();
};

/** Used to store monster effects placed on attack */
struct mon_effect_data {
    // The type of the effect.
    efftype_id id;
    // The percent chance of causing the effect.
    float chance;
    // Whether the effect is permanent.
    bool permanent;
    bool affect_hit_bp;
    bodypart_str_id bp;
    // The range of the durations (in turns) of the effect.
    std::pair<int, int> duration;
    // The range of the intensities of the effect.
    std::pair<int, int> intensity;
    // The message to print, if the player causes the effect.
    std::string message;

    mon_effect_data();
    void load( const JsonObject &jo );
};

/** Pet food data */
struct pet_food_data {
    std::set<std::string> food;
    std::string pet;
    std::string feed;

    bool was_loaded = false;
    void load( const JsonObject &jo );
    void deserialize( const JsonObject &data );
};

enum class mdeath_type {
    NORMAL,
    SPLATTER,
    BROKEN,
    NO_CORPSE,
    LAST
};

template<>
struct enum_traits<mdeath_type> {
    static constexpr mdeath_type last = mdeath_type::LAST;
};

struct monster_death_effect {
    bool was_loaded = false;
    bool has_effect = false;
    fake_spell sp;
    translation death_message;
    mdeath_type corpse_type = mdeath_type::NORMAL;

    void load( const JsonObject &jo );
    void deserialize( const JsonObject &data );
};

struct mount_item_data {
    /**
     * If this monster is a rideable mount that spawns with a tied item (leash), this is the tied item id
     */
    itype_id tied;
    /**
     * If this monster is a rideable mount that spawns with a tack item, this is the tack item id
     */
    itype_id tack;
    /**
     * If this monster is a rideable mount that spawns with armor, this is the armor item id
     */
    itype_id armor;
    /**
     * If this monster is a rideable mount that spawns with storage bags, this is the storage item id
     */
    itype_id storage;
};

struct mtype {
    private:
        friend class MonsterGenerator;

        enum_bitset<mon_trigger> anger;
        enum_bitset<mon_trigger> fear;
        enum_bitset<mon_trigger> placate;
    public:
        units::mass weight;
        // This monster's special "defensive" move that may trigger when the monster is attacked.
        // Note that this can be anything, and is not necessarily beneficial to the monster
        mon_action_defend sp_defense;
    private:
        ascii_art_id picture_id;
        std::unordered_set<mon_flag_id> flags;
        std::set<mon_flag_str_id> pre_flags_; // used only for initial loading
    public:
        mtype_id id;
        mfaction_str_id default_faction;
        harvest_id harvest;
        harvest_id dissect;
        speed_description_id speed_desc;
        // Monster upgrade variables
        mtype_id upgrade_into;
        mongroup_id upgrade_group;
        mtype_id burn_into;

        mtype_id zombify_into; // mtype_id this monster zombifies into
        mtype_id fungalize_into; // mtype_id this monster fungalize into

        mtype_id baby_monster;
        itype_id baby_egg;
        // Monster biosignature variables
        itype_id biosig_item;

        /**
         * If this is not empty, the monster can be converted into an item
         * of this type (if it's friendly).
         */
        itype_id revert_to_itype;
        /**
         * If this monster is a rideable mech with built-in weapons, this is the weapons id
         */
        itype_id mech_weapon;
        /**
         * If this monster is a rideable mech it needs a power source battery type
         */
        itype_id mech_battery;

        /** Stores effect data for effects placed on attack */
        std::vector<mon_effect_data> atk_effs;

        /** Mod origin */
        std::vector<std::pair<mtype_id, mod_id>> src;
        weakpoint_families families;
    private:
        std::vector<weakpoints_id> weakpoints_deferred;
    public:

        // The type of material this monster can absorb. Leave unspecified for all materials.
        std::vector<material_id> absorb_material;
        damage_instance melee_damage; // Basic melee attack damage
        std::vector<std::string> special_attacks_names; // names of attacks, in json load order
        std::vector<std::string> chat_topics; // What it has to say.
        std::vector<std::string> baby_flags;
        /** UTF-8 encoded symbol, should be exactly one cell wide. */
        std::string sym;
        /** hint for tilesets that don't have a tile for this monster */
        std::string looks_like;
        bodytype_id bodytype;
        shearing_data shearing;

        std::map<itype_id, int> starting_ammo; // Amount of ammo the monster spawns with.
        // Name of item group that is used to create item dropped upon death, or empty.
        item_group_id death_drops;

        std::set<species_id> species;
        std::set<std::string> categories;
        std::map<material_id, int> mat;
        // Effects that can modify regeneration
        std::map<efftype_id, int> regeneration_modifiers;

        std::set<scenttype_id> scents_tracked; /**Types of scent tracked by this mtype*/
        std::set<scenttype_id> scents_ignored; /**Types of scent ignored by this mtype*/

        resistances armor;
        // Traps avoided by this monster
        std::set<trap_str_id> trap_avoids;
    private:
        std::set<std::string> weakpoints_deferred_deleted;
    public:
        // special attack frequencies and function pointers
        std::map<std::string, mtype_special_attack> special_attacks;
        /** Emission sources that cycle each turn the monster remains alive */
        std::map<emit_id, time_duration> emit_fields;
        std::optional<resistances> armor_proportional; /**load-time only*/
        std::optional<resistances> armor_relative; /**load-time only*/

        /** Mount-specific items this monster spawns with */
        mount_item_data mount_items;
    private:

        translation name;
        translation description;
    public:

        // Pet food category this monster is in
        pet_food_data petfood;
    private:

        behavior::node_t goals;

    public:
        monster_death_effect mdeath_effect;
        ::weakpoints weakpoints;

    private:
        ::weakpoints weakpoints_deferred_inline;
    public:
        int mat_portion_total = 0;
        units::volume volume;
        creature_size size;
        phase_id phase;
        int difficulty = 0;     /** many uses; 30 min + (diff-3)*30 min = earliest appearance */
        // difficulty from special attacks instead of from melee attacks, defenses, HP, etc.
        int difficulty_base = 0;

        int hp = 0;
        int speed = 0;          /** e.g. human = 100 */
        int agro = 0;           /** chance will attack [-100,100] */
        int morale = 0;         /** initial morale level at spawn */
        int stomach_size = 0;         /** how many times this monster will eat */
        int amount_eaten = 0;         /** how many times it has eaten */

        // how close the monster is willing to approach its target while under the MATT_FOLLOW attitude
        int tracking_distance = 8;

        // Number of hitpoints regenerated per turn.
        int regenerates = 0;
        // mountable ratio for rider weight vs. mount weight, default 0.2
        float mountable_weight_ratio = 0.2f;

        int attack_cost = 100;  /** moves per regular attack */
        int melee_skill = 0;    /** melee hit skill, 20 is superhuman hitting abilities */
        int melee_dice = 0;     /** number of dice of bonus bashing damage on melee hit */
        int melee_sides = 0;    /** number of sides those dice have */

        int grab_strength = 1;    /**intensity of the effect_grabbed applied*/
        int sk_dodge = 0;       /** dodge skill */

        // Multiplier to chance to apply status effects (only zapped for now)
        float status_chance_multiplier = 1.0f;

        // Bleed rate in percent, 0 makes the monster immune to bleeding
        int bleed_rate = 100;

        // The amount of volume in milliliters that this monster needs to absorb to gain 1 HP (default 250)
        int absorb_ml_per_hp = 250;

        // The move cost for this monster splitting via SPLITS_ABSORBS flag (default 200)
        int split_move_cost = 200;

        // Move cost per ml of matter consumed for this monster (default 0.025).
        float absorb_move_cost_per_ml = 0.025f;

        // Minimum move cost for this monster to absorb an item (default 1)
        int absorb_move_cost_min = 1;
        // Maximum move cost for this monster to absorb an item (default -1, -1 for no limit)
        int absorb_move_cost_max = -1;

        float luminance;           // 0 is default, >0 gives luminance to lightmap
        // Vision range is linearly scaled depending on lighting conditions
        int vision_day = 40;    /** vision range in bright light */
        int vision_night = 1;   /** vision range in total darkness */

        unsigned int def_chance; // How likely a special "defensive" move is to trigger (0-100%, default 0)
        // Monster's ability to destroy terrain and vehicles
        int bash_skill;

        // Monster upgrade variables
        int half_life;
        int age_grow;

        // Monster reproduction variables
        int baby_count;

        /**
         * If this monster is a rideable mech with enhanced strength, this is the strength it gives to the player
         */
        int mech_str_bonus = 0;

        // Grinding cap for training player's melee skills when hitting this monster, defaults to MAX_SKILL.
        int melee_training_cap;

        nc_color color = c_white;

        std::optional<int> upgrade_multi_range;

        // Monster reproduction variables
        std::optional<time_duration> baby_timer;

        // Monster biosignature variables
        std::optional<time_duration> biosig_timer;

        pathfinding_settings path_settings;

        // All the bools together for space efficiency
        //
        // Monster regenerates very quickly in poorly lit tiles.
        bool regenerates_in_dark = false;
        // Will stop fleeing if at max hp, and regen anger and morale.
        bool regen_morale = false;
        bool upgrade_null_despawn;
        // TODO: maybe make this private as well? It must be set to `true` only once,
        // and must never be set back to `false`.
        bool was_loaded = false;
        bool upgrades;
        bool reproduces;
        bool biosignatures;

        // Do we indiscriminately attack characters, or should we wait until one annoys us?
        bool aggro_character = true;

        mtype();
        /**
         * Check if this type is of the same species as the other one, because
         * species is a set and can contain several species, one entry that is
         * in both monster types fulfills that test.
         */
        bool same_species( const mtype &other ) const;

        // Used to fetch the properly pluralized monster type name
        std::string nname( unsigned int quantity = 1 ) const;
        bool has_special_attack( const std::string &attack_name ) const;
        bool has_flag( const mon_flag_id &flag ) const;
        void set_flag( const mon_flag_id &flag, bool state = true );
        bool made_of( const material_id &material ) const;
        bool made_of_any( const std::set<material_id> &materials ) const;
        bool has_anger_trigger( mon_trigger trigger ) const;
        bool has_fear_trigger( mon_trigger trigger ) const;
        bool has_placate_trigger( mon_trigger trigger ) const;
        bool in_category( const std::string &category ) const;
        bool in_species( const species_id &spec ) const;
        std::vector<std::string> species_descriptions() const;
        field_type_id get_bleed_type() const;
        //Used for corpses.
        field_type_id bloodType() const;
        field_type_id gibType() const;
        // The item id of the meat items that are produced by this monster (or "null")
        // if there is no matching item type. e.g. "veggy" for plant monsters.
        itype_id get_meat_itype() const;
        int get_meat_chunks_count() const;
        std::string get_description() const;
        ascii_art_id get_picture_id() const;
        std::string get_footsteps() const;
        void set_strategy();
        void add_goal( const std::string &goal_id );
        const behavior::node_t *get_goals() const;
        void faction_display( catacurses::window &w, const point &top_left, int width ) const;

        // Historically located in monstergenerator.cpp
        void load( const JsonObject &jo, const std::string &src );

    private:

        void add_special_attacks( const JsonObject &jo, std::string_view member_name,
                                  const std::string &src );
        void remove_special_attacks( const JsonObject &jo, std::string_view member_name,
                                     std::string_view src );

        void add_special_attack( const JsonArray &inner, std::string_view src );
        void add_special_attack( const JsonObject &obj, const std::string &src );

        void add_regeneration_modifiers( const JsonObject &jo, std::string_view member_name,
                                         std::string_view src );
        void remove_regeneration_modifiers( const JsonObject &jo, std::string_view member_name,
                                            std::string_view src );

        void add_regeneration_modifier( const JsonArray &inner, std::string_view src );
};

#endif // CATA_SRC_MTYPE_H

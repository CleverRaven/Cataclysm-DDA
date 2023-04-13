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

    LAST               // This item must always remain last.
};

template<>
struct enum_traits<mon_trigger> {
    static constexpr mon_trigger last = mon_trigger::LAST;
};

// Feel free to add to m_flags.  Order shouldn't matter, just keep it tidy!
// And comment them well. ;)
// TODO: And rename them to 'mon_flags'
// TODO: And turn them into an enum class (like mon_trigger).
enum m_flag : int {
    MF_SEES,                // It can see you (and will run/follow)
    MF_HEARS,               // It can hear you
    MF_GOODHEARING,         // Pursues sounds more than most monsters
    MF_SMELLS,              // It can smell you
    MF_KEENNOSE,            // Keen sense of smell
    MF_STUMBLES,            // Stumbles in its movement
    MF_WARM,                // Warm blooded
    MF_NEMESIS,             // Is a nemesis monster
    MF_NOHEAD,              // Headshots not allowed!
    MF_HARDTOSHOOT,         // It's one size smaller for ranged attacks, no less then creature_size::tiny
    MF_GRABS,               // Its attacks may grab us!
    MF_BASHES,              // Bashes down doors
    MF_DESTROYS,            // Bashes down walls and more
    MF_BORES,               // Tunnels through just about anything
    MF_POISON,              // Poisonous to eat
    MF_VENOM,               // Attack may poison the player
    MF_BADVENOM,            // Attack may SEVERELY poison the player
    MF_PARALYZE,            // Attack may paralyze the player with venom
    MF_WEBWALK,             // Doesn't destroy webs
    MF_DIGS,                // Digs through the ground
    MF_CAN_DIG,             // Can dig and walk
    MF_FLIES,               // Can fly (over water, etc)
    MF_AQUATIC,             // Confined to water
    MF_SWIMS,               // Treats water as 50 movement point terrain
    MF_ATTACKMON,           // Attacks other monsters
    MF_ANIMAL,              // Is an "animal" for purposes of the Animal Empath trait
    MF_PLASTIC,             // Absorbs physical damage to a great degree
    MF_SUNDEATH,            // Dies in full sunlight
    MF_ELECTRIC,            // Shocks unarmed attackers
    MF_ACIDPROOF,           // Immune to acid
    MF_ACIDTRAIL,           // Leaves a trail of acid
    MF_SHORTACIDTRAIL,      // Leaves an intermittent trail of acid
    MF_FIREPROOF,           // Immune to fire
    MF_SLUDGEPROOF,         // Ignores the effect of sludge trails
    MF_SLUDGETRAIL,         // Causes monster to leave a sludge trap trail when moving
    MF_SMALLSLUDGETRAIL,    // Causes monster to leave a low intensity, 1 tile sludge pool approximately every other tile when moving
    MF_COLDPROOF,           // Immune to cold damage
    MF_COMBAT_MOUNT,        // Mount has better chance to ignore hostile monster fear
    MF_FIREY,               // Burns stuff and is immune to fire
    MF_QUEEN,               // When it dies, local populations start to die off too
    MF_ELECTRONIC,          // e.g. a robot; affected by EMP blasts, and other stuff
    MF_CONSOLE_DESPAWN,     // Despawns when a nearby console is properly hacked
    MF_IMMOBILE,            // Doesn't move (e.g. turrets)
    MF_ID_CARD_DESPAWN,     // Despawns when a science ID card is used on a nearby console
    MF_RIDEABLE_MECH,       // A rideable mech that is immobile until ridden.
    MF_MILITARY_MECH,       // A rideable mech that was designed for military work.
    MF_MECH_RECON_VISION,   // This mech gives you IR night-vision.
    MF_MECH_DEFENSIVE,      // This mech gives you thorough protection.
    MF_HIT_AND_RUN,         // Flee for several turns after a melee attack
    MF_PAY_BOT,             // You can pay this bot to be your friend for a time
    MF_HUMAN,               // It's a live human, as long as it's alive
    MF_NO_BREATHE,          // Creature can't drown and is unharmed by gas, smoke, or poison
    MF_FLAMMABLE,           // Monster catches fire, burns, and spreads fire to nearby objects
    MF_REVIVES,             // Monster corpse will revive after a short period of time
    MF_VERMIN,              // Obsolete flag labeling "nuisance" or "scenery" monsters, now used to prevent loading the same.
    MF_NOGIB,               // Creature won't leave gibs / meat chunks when killed with huge damage.
    MF_ARTHROPOD_BLOOD,     // Forces monster to bleed hemolymph.
    MF_ACID_BLOOD,          // Makes monster bleed acid. Fun stuff! Does not automatically dissolve in a pool of acid on death.
    MF_BILE_BLOOD,          // Makes monster bleed bile.
    MF_FILTHY,              // Any clothing it drops will be filthy.
    MF_FISHABLE,            // It is fishable.
    MF_GROUP_BASH,          // Monsters that can pile up against obstacles and add their strength together to break them.
    MF_SWARMS,              // Monsters that like to group together and form loose packs
    MF_GROUP_MORALE,        // Monsters that are more courageous when near friends
    MF_INTERIOR_AMMO,       // Monster contain's its ammo inside itself, no need to load on launch. Prevents ammo from being dropped on disable.
    MF_CLIMBS,              // Monsters that can climb certain terrain and furniture
    MF_PACIFIST,            // Monsters that will never use melee attack, useful for having them use grab without attacking the player
    MF_KEEP_DISTANCE,       // Attempts to keep a short distance (tracking_distance) from its current target.  The default tracking distance is 8 tiles
    MF_PUSH_MON,            // Monsters that can push creatures out of their way
    MF_PUSH_VEH,            // Monsters that can push vehicles out of their way
    MF_NIGHT_INVISIBILITY,  // Monsters that are invisible in poor light conditions
    MF_REVIVES_HEALTHY,     // When revived, this monster has full hitpoints and speed
    MF_NO_NECRO,            // This monster can't be revived by necros. It will still rise on its own.
    MF_AVOID_DANGER_1,      // This monster will path around some dangers instead of through them.
    MF_AVOID_DANGER_2,      // This monster will path around most dangers instead of through them.
    MF_AVOID_FIRE,          // This monster will path around heat-related dangers instead of through them.
    MF_AVOID_FALL,          // This monster will path around cliffs instead of off of them.
    MF_PRIORITIZE_TARGETS,  // This monster will prioritize targets depending on their danger levels
    MF_NOT_HALLU,           // Monsters that will NOT appear when player's producing hallucinations
    MF_CANPLAY,             // This monster can be played with if it's a pet.
    MF_CAN_BE_CULLED,       // This monster can be culled if it's a pet.
    MF_PET_MOUNTABLE,       // This monster can be mounted and ridden when tamed.
    MF_PET_HARNESSABLE,     // This monster can be harnessed when tamed.
    MF_DOGFOOD,             // This monster will respond to the dog whistle.
    MF_MILKABLE,            // This monster is milkable.
    MF_SHEARABLE,           // This monster is shearable.
    MF_NO_BREED,            // This monster doesn't breed, even though it has breed data
    MF_NO_FUNG_DMG,         // This monster can't be damaged by fungal spores and can't be fungalized either.
    MF_PET_WONT_FOLLOW,     // This monster won't follow the player automatically when tamed.
    MF_DRIPS_NAPALM,        // This monster occasionally drips napalm on move
    MF_DRIPS_GASOLINE,      // This monster occasionally drips gasoline on move
    MF_ELECTRIC_FIELD,      // This monster is surrounded by an electrical field that ignites flammable liquids near it
    MF_LOUDMOVES,           // This monster makes move noises as if ~2 sizes louder, even if flying.
    MF_CAN_OPEN_DOORS,      // This monster can open doors.
    MF_STUN_IMMUNE,         // This monster is immune to the stun effect
    MF_DROPS_AMMO,          // This monster drops ammo. Should not be set for monsters that use pseudo ammo.
    MF_INSECTICIDEPROOF,    // This monster is immune to insecticide, even though it's made of bug flesh
    MF_RANGED_ATTACKER,     // This monster has any sort of ranged attack
    MF_CAMOUFLAGE,          // This monster is hard to spot, even in broad daylight
    MF_WATER_CAMOUFLAGE,    // This monster is hard to spot if it is underwater, especially if you aren't
    MF_ATTACK_UPPER,        // This monster is capable of hitting upper limbs
    MF_ATTACK_LOWER,        // This monster is incapable of hitting upper limbs regardless of other factors
    MF_DEADLY_VIRUS,        // This monster can inflict the zombie_virus effect
    MF_VAMP_VIRUS,          // This monster can inflict the vampire_virus effect
    MF_ALWAYS_VISIBLE,      // This monster can always be seen regardless of los or light or anything
    MF_ALWAYS_SEES_YOU,     // This monster always knows where the avatar is
    MF_ALL_SEEING,          // This monster can see everything within its vision range regardless of light or obstacles
    MF_NEVER_WANDER,        // This monster will never join wandering hordes.
    MF_CONVERSATION,        // This monster can engage in conversation.  Will need to have chat_topics as well.
    MF_MAX                  // Sets the length of the flags - obviously must be LAST
};

template<>
struct enum_traits<m_flag> {
    static constexpr m_flag last = m_flag::MF_MAX;
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

struct mtype {
    private:
        friend class MonsterGenerator;
        translation name;
        translation description;

        ascii_art_id picture_id;

        enum_bitset<m_flag> flags;

        enum_bitset<mon_trigger> anger;
        enum_bitset<mon_trigger> fear;
        enum_bitset<mon_trigger> placate;

        behavior::node_t goals;

        void add_special_attacks( const JsonObject &jo, const std::string &member_name,
                                  const std::string &src );
        void remove_special_attacks( const JsonObject &jo, const std::string &member_name,
                                     const std::string &src );

        void add_special_attack( const JsonArray &inner, const std::string &src );
        void add_special_attack( const JsonObject &obj, const std::string &src );

        void add_regeneration_modifiers( const JsonObject &jo, const std::string &member_name,
                                         const std::string &src );
        void remove_regeneration_modifiers( const JsonObject &jo, const std::string &member_name,
                                            const std::string &src );

        void add_regeneration_modifier( const JsonArray &inner, const std::string &src );

    public:
        mtype_id id;

        std::map<itype_id, int> starting_ammo; // Amount of ammo the monster spawns with.
        // Name of item group that is used to create item dropped upon death, or empty.
        item_group_id death_drops;

        /** Stores effect data for effects placed on attack */
        std::vector<mon_effect_data> atk_effs;

        /** Mod origin */
        std::vector<std::pair<mtype_id, mod_id>> src;

        std::set<species_id> species;
        std::set<std::string> categories;
        std::map<material_id, int> mat;
        int mat_portion_total = 0;
        /** UTF-8 encoded symbol, should be exactly one cell wide. */
        std::string sym;
        /** hint for tilesets that don't have a tile for this monster */
        std::string looks_like;
        mfaction_str_id default_faction;
        bodytype_id bodytype;
        units::mass weight;
        units::volume volume;
        nc_color color = c_white;
        creature_size size;
        phase_id phase;

        int difficulty = 0;     /** many uses; 30 min + (diff-3)*30 min = earliest appearance */
        // difficulty from special attacks instead of from melee attacks, defenses, HP, etc.
        int difficulty_base = 0;
        int hp = 0;
        int speed = 0;          /** e.g. human = 100 */
        int agro = 0;           /** chance will attack [-100,100] */
        int morale = 0;         /** initial morale level at spawn */

        // how close the monster is willing to approach its target while under the MATT_FOLLOW attitude
        int tracking_distance = 8;

        // Number of hitpoints regenerated per turn.
        int regenerates = 0;
        // Effects that can modify regeneration
        std::map<efftype_id, int> regeneration_modifiers;
        // mountable ratio for rider weight vs. mount weight, default 0.2
        float mountable_weight_ratio = 0.2f;
        // Monster regenerates very quickly in poorly lit tiles.
        bool regenerates_in_dark = false;
        // Will stop fleeing if at max hp, and regen anger and morale.
        bool regen_morale = false;

        int attack_cost = 100;  /** moves per regular attack */
        int melee_skill = 0;    /** melee hit skill, 20 is superhuman hitting abilities */
        int melee_dice = 0;     /** number of dice of bonus bashing damage on melee hit */
        int melee_sides = 0;    /** number of sides those dice have */

        int grab_strength = 1;    /**intensity of the effect_grabbed applied*/
        int sk_dodge = 0;       /** dodge skill */

        std::set<scenttype_id> scents_tracked; /**Types of scent tracked by this mtype*/
        std::set<scenttype_id> scents_ignored; /**Types of scent ignored by this mtype*/

        /** If unset (-1) then values are calculated automatically from other properties */
        int armor_bash = -1;    /** innate armor vs. bash */
        int armor_cut  = -1;    /** innate armor vs. cut */
        int armor_stab = -1;    /** innate armor vs. stabbing */
        int armor_bullet = -1;  /** innate armor vs. bullet */
        int armor_acid = -1;    /** innate armor vs. acid */
        int armor_fire = -1;    /** innate armor vs. fire */
        int armor_elec = -1;    /** innate armor vs. electricity */
        int armor_cold = -1;    /** innate armor vs. cold **/
        int armor_pure = -1;    /** innate armor vs. pure **/
        int armor_biological = -1; /** innate armor vs. biological **/
        ::weakpoints weakpoints;
        weakpoint_families families;

        // Traps avoided by this monster
        std::set<trap_str_id> trap_avoids;

    private:
        std::vector<weakpoints_id> weakpoints_deferred;
        ::weakpoints weakpoints_deferred_inline;
        std::set<std::string> weakpoints_deferred_deleted;

    public:

        // Pet food category this monster is in
        pet_food_data petfood;

        // Multiplier to chance to apply status effects (only zapped for now)
        float status_chance_multiplier = 1.0f;

        // Bleed rate in percent, 0 makes the monster immune to bleeding
        int bleed_rate = 100;

        // The amount of volume in milliliters that this monster needs to absorb to gain 1 HP (default 250)
        int absorb_ml_per_hp = 250;

        // The type of material this monster can absorb. Leave unspecified for all materials.
        std::vector<material_id> absorb_material;

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

        damage_instance melee_damage; // Basic melee attack damage
        harvest_id harvest;
        harvest_id dissect;
        shearing_data shearing;
        speed_description_id speed_desc;

        // special attack frequencies and function pointers
        std::map<std::string, mtype_special_attack> special_attacks;
        std::vector<std::string> special_attacks_names; // names of attacks, in json load order
        monster_death_effect mdeath_effect;
        std::vector<std::string> chat_topics; // What it has to say.
        // This monster's special "defensive" move that may trigger when the monster is attacked.
        // Note that this can be anything, and is not necessarily beneficial to the monster
        mon_action_defend sp_defense;

        unsigned int def_chance; // How likely a special "defensive" move is to trigger (0-100%, default 0)
        // Monster's ability to destroy terrain and vehicles
        int bash_skill;

        // Monster upgrade variables
        int half_life;
        int age_grow;
        mtype_id upgrade_into;
        mongroup_id upgrade_group;
        mtype_id burn_into;
        std::optional<int> upgrade_multi_range;
        bool upgrade_null_despawn;

        mtype_id zombify_into; // mtype_id this monster zombifies into
        mtype_id fungalize_into; // mtype_id this monster fungalize into

        // Monster reproduction variables
        std::optional<time_duration> baby_timer;
        int baby_count;
        mtype_id baby_monster;
        itype_id baby_egg;
        std::vector<std::string> baby_flags;

        // Monster biosignature variables
        itype_id biosig_item;
        std::optional<time_duration> biosig_timer;

        // All the bools together for space efficiency

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
        /** Emission sources that cycle each turn the monster remains alive */
        std::map<emit_id, time_duration> emit_fields;

        /**
         * If this monster is a rideable mech with enhanced strength, this is the strength it gives to the player
         */
        int mech_str_bonus = 0;

        // Grinding cap for training player's melee skills when hitting this monster, defaults to MAX_SKILL.
        int melee_training_cap;

        pathfinding_settings path_settings;
        // Used to fetch the properly pluralized monster type name
        std::string nname( unsigned int quantity = 1 ) const;
        bool has_special_attack( const std::string &attack_name ) const;
        bool has_flag( m_flag flag ) const;
        void set_flag( m_flag flag, bool state = true );
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
};

#endif // CATA_SRC_MTYPE_H

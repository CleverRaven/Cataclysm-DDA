#pragma once
#ifndef MTYPE_H
#define MTYPE_H
// SEE ALSO: monitemsdef.cpp, which defines data on which items any given
// monster may carry.

#include "enums.h"
#include "color.h"
#include "int_id.h"
#include "string_id.h"
#include "damage.h"
#include "pathfinding.h"
#include "mattack_common.h"

#include <bitset>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <math.h>

class Creature;
class monster;
class monfaction;
class emit;
using emit_id = string_id<emit>;
struct dealt_projectile_attack;
struct species_type;
enum field_id : int;
enum body_part : int;
enum m_size : int;

using mon_action_death  = void ( * )( monster & );
using mon_action_attack = bool ( * )( monster * );
using mon_action_defend = void ( * )( monster &, Creature *, dealt_projectile_attack const * );
struct MonsterGroup;
using mongroup_id = string_id<MonsterGroup>;
struct mtype;
using mtype_id = string_id<mtype>;
using mfaction_id = int_id<monfaction>;
using species_id = string_id<species_type>;
class effect_type;
using efftype_id = string_id<effect_type>;
class JsonArray;
class JsonIn;
class JsonObject;
class material_type;
using material_id = string_id<material_type>;

typedef std::string itype_id;

class emit;
using emit_id = string_id<emit>;

class harvest_list;
using harvest_id = string_id<harvest_list>;

// These are triggers which may affect the monster's anger or morale.
// They are handled in monster::check_triggers(), in monster.cpp
enum monster_trigger : int {
    MTRIG_NULL = 0,
    MTRIG_STALK,  // Increases when following the player
    MTRIG_MEAT,  // Meat or a corpse nearby
    MTRIG_HOSTILE_WEAK, // Hurt hostile player/npc/monster seen
    MTRIG_HOSTILE_CLOSE, // Hostile creature within a few tiles
    MTRIG_HURT,  // We are hurt
    MTRIG_FIRE,  // Fire nearby
    MTRIG_FRIEND_DIED, // A monster of the same type died
    MTRIG_FRIEND_ATTACKED, // A monster of the same type attacked
    MTRIG_SOUND,  // Heard a sound
    N_MONSTER_TRIGGERS
};

// Feel free to add to m_flags.  Order shouldn't matter, just keep it tidy!
// And comment them well. ;)
enum m_flag : int {
    MF_NULL = 0,            //
    MF_SEES,                // It can see you (and will run/follow)
    MF_HEARS,               // It can hear you
    MF_GOODHEARING,         // Pursues sounds more than most monsters
    MF_SMELLS,              // It can smell you
    MF_KEENNOSE,            //Keen sense of smell
    MF_STUMBLES,            // Stumbles in its movement
    MF_WARM,                // Warm blooded
    MF_NOHEAD,              // Headshots not allowed!
    MF_HARDTOSHOOT,         // It's one size smaller for ranged attacks, no less then MS_TINY
    MF_GRABS,               // Its attacks may grab us!
    MF_BASHES,              // Bashes down doors
    MF_DESTROYS,            // Bashes down walls and more
    MF_BORES,               // Tunnels through just about anything
    MF_POISON,              // Poisonous to eat
    MF_VENOM,               // Attack may poison the player
    MF_BADVENOM,            // Attack may SEVERELY poison the player
    MF_PARALYZE,            // Attack may paralyze the player with venom
    MF_BLEED,               // Causes player to bleed
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
    MF_FIREPROOF,           // Immune to fire
    MF_SLUDGEPROOF,         // Ignores the effect of sludge trails
    MF_SLUDGETRAIL,         // Causes monster to leave a sludge trap trail when moving
    MF_FIREY,               // Burns stuff and is immune to fire
    MF_QUEEN,               // When it dies, local populations start to die off too
    MF_ELECTRONIC,          // e.g. a robot; affected by EMP blasts, and other stuff
    MF_FUR,                 // May produce fur when butchered
    MF_LEATHER,             // May produce leather when butchered
    MF_WOOL,                // May produce wool when butchered
    MF_FEATHER,             // May produce feather when butchered
    MF_BONES,               // May produce bones and sinews when butchered; if combined with POISON flag, tainted bones, if combined with HUMAN, human bones
    MF_FAT,                 // May produce fat when butchered; if combined with POISON flag, tainted fat
    MF_IMMOBILE,            // Doesn't move (e.g. turrets)
    MF_HIT_AND_RUN,         // Flee for several turns after a melee attack
    MF_GUILT,               // You feel guilty for killing it
    MF_HUMAN,               // It's a live human, as long as it's alive
    MF_NO_BREATHE,          // Creature can't drown and is unharmed by gas, smoke, or poison
    MF_REGENERATES_50,      // Monster regenerates very quickly over time
    MF_REGENERATES_10,      // Monster regenerates quickly over time
    MF_REGENERATES_IN_DARK, // Monster regenerates very quickly in poorly lit tiles
    MF_FLAMMABLE,           // Monster catches fire, burns, and spreads fire to nearby objects
    MF_REVIVES,             // Monster corpse will revive after a short period of time
    MF_CHITIN,              // May produce chitin when butchered
    MF_VERMIN,              // Obsolete flag labeling "nuisance" or "scenery" monsters, now used to prevent loading the same.
    MF_NOGIB,               // Creature won't leave gibs / meat chunks when killed with huge damage.
    MF_LARVA,               // Creature is a larva. Currently used for gib and blood handling.
    MF_ARTHROPOD_BLOOD,     // Forces monster to bleed hemolymph.
    MF_ACID_BLOOD,          // Makes monster bleed acid. Fun stuff! Does not automatically dissolve in a pool of acid on death.
    MF_BILE_BLOOD,          // Makes monster bleed bile.
    MF_ABSORBS,             // Consumes objects it moves over which gives bonus hp.
    MF_ABSORBS_SPLITS,      // Consumes objects it moves over which gives bonus hp. If it gets enough bonus HP, it spawns a copy of itself.
    MF_REGENMORALE,         // Will stop fleeing if at max hp, and regen anger and morale to positive values.
    MF_CBM_CIV,             // May produce a common CBM a power CBM when butchered.
    MF_CBM_POWER,           // May produce a power CBM when butchered, independent of MF_CBM_wev.
    MF_CBM_SCI,             // May produce a bionic from bionics_sci when butchered.
    MF_CBM_OP,              // May produce a bionic from bionics_op when butchered, and the power storage is mk 2.
    MF_CBM_TECH,            // May produce a bionic from bionics_tech when butchered.
    MF_CBM_SUBS,            // May produce a bionic from bionics_subs when butchered.
    MF_FISHABLE,            // It is fishable.
    MF_GROUP_BASH,          // Monsters that can pile up against obstacles and add their strength together to break them.
    MF_SWARMS,              // Monsters that like to group together and form loose packs
    MF_GROUP_MORALE,        // Monsters that are more courageous when near friends
    MF_INTERIOR_AMMO,       // Monster contain's its ammo inside itself, no need to load on launch. Prevents ammo from being dropped on disable.
    MF_CLIMBS,              // Monsters that can climb certain terrain and furniture
    MF_PUSH_MON,            // Monsters that can push creatures out of their way
    MF_NIGHT_INVISIBILITY,  // Monsters that are invisible in poor light conditions
    MF_REVIVES_HEALTHY,     // When revived, this monster has full hitpoints and speed
    MF_NO_NECRO,            // This monster can't be revived by necros. It will still rise on its own.
    MF_AVOID_DANGER_1,      // This monster will path around some dangers instead of through them.
    MF_AVOID_DANGER_2,      // This monster will path around most dangers instead of through them.
    MF_PRIORITIZE_TARGETS,  // This monster will prioritize targets depending on their danger levels
    MF_NOT_HALLU,           // Monsters that will NOT appear when player's producing hallucinations
    MF_CATFOOD,             // This monster will become friendly when fed cat food.
    MF_CATTLEFODDER,        // This monster will become friendly when fed cattle fodder.
    MF_BIRDFOOD,         // This monster will become friendly when fed bird food.
    MF_DOGFOOD,             // This monster will become friendly when fed dog food.
    MF_MILKABLE,            // This monster is milkable.
    MF_PET_WONT_FOLLOW,     // This monster won't follow the player automatically when tamed.
    MF_DRIPS_NAPALM,        // This monster ocassionally drips napalm on move
    MF_MAX                  // Sets the length of the flags - obviously must be LAST
};

/** Used to store monster effects placed on attack */
struct mon_effect_data {
    efftype_id id;
    int duration;
    bool affect_hit_bp;
    body_part bp;
    bool permanent;
    int chance;

    mon_effect_data( const efftype_id &nid, int dur, bool ahbp, body_part nbp, bool perm,
                     int nchance ) :
        id( nid ), duration( dur ), affect_hit_bp( ahbp ), bp( nbp ), permanent( perm ),
        chance( nchance ) {};
};

struct mtype {
    private:
        friend class MonsterGenerator;
        std::string name;
        std::string name_plural;
        std::string description;

        std::set< const species_type * > species_ptrs;

        void add_special_attacks( JsonObject &jo, const std::string &member_name, const std::string &src );
        void remove_special_attacks( JsonObject &jo, const std::string &member_name,
                                     const std::string &src );

        void add_special_attack( JsonArray jarr, const std::string &src );
        void add_special_attack( JsonObject jo, const std::string &src );

    public:
        mtype_id id;
        // TODO: maybe make this private as well? It must be set to `true` only once,
        // and must never be set back to `false`.
        bool was_loaded = false;
        std::set<species_id> species;
        std::set<std::string> categories;
        mfaction_id default_faction;
        /** UTF-8 encoded symbol, should be exactly one cell wide. */
        std::string sym;
        nc_color color = c_white;
        /** hint for tilesets that don't have a tile for this monster */
        std::string looks_like;
        m_size size;
        std::vector<material_id> mat;
        phase_id phase;
        std::set<m_flag> flags;
        std::set<monster_trigger> anger, placate, fear;

        std::bitset<MF_MAX> bitflags;
        std::bitset<N_MONSTER_TRIGGERS> bitanger, bitfear, bitplacate;

        /** Stores effect data for effects placed on attack */
        std::vector<mon_effect_data> atk_effs;

        int difficulty = 0;     /** many uses; 30 min + (diff-3)*30 min = earliest appearance */
        int hp = 0;
        int speed = 0;          /** e.g. human = 100 */
        int agro = 0;           /** chance will attack [-100,100] */
        int morale = 0;         /** initial morale level at spawn */

        int attack_cost = 100;  /** moves per regular attack */
        int melee_skill = 0;    /** melee hit skill, 20 is superhuman hitting abilities */
        int melee_dice = 0;     /** number of dice of bonus bashing damage on melee hit */
        int melee_sides = 0;    /** number of sides those dice have */

        int sk_dodge = 0;       /** dodge skill */

        /** If unset (-1) then values are calculated automatically from other properties */
        int armor_bash = -1;    /** innate armor vs. bash */
        int armor_cut  = -1;    /** innate armor vs. cut */
        int armor_stab = -1;    /** innate armor vs. stabbing */
        int armor_acid = -1;    /** innate armor vs. acid */
        int armor_fire = -1;    /** innate armor vs. fire */

        // Vision range is linearly scaled depending on lighting conditions
        int vision_day = 40;    /** vision range in bright light */
        int vision_night = 1;   /** vision range in total darkness */

        damage_instance melee_damage; // Basic melee attack damage

        std::map<std::string, int> starting_ammo; // Amount of ammo the monster spawns with.
        // Name of item group that is used to create item dropped upon death, or empty.
        std::string death_drops;
        harvest_id harvest;
        float luminance;           // 0 is default, >0 gives luminance to lightmap
        // special attack frequencies and function pointers
        std::map<std::string, mtype_special_attack> special_attacks;
        std::vector<std::string> special_attacks_names; // names of attacks, in json load order

        unsigned int def_chance; // How likely a special "defensive" move is to trigger (0-100%, default 0)

        std::vector<mon_action_death>  dies;       // What happens when this monster dies

        // This monster's special "defensive" move that may trigger when the monster is attacked.
        // Note that this can be anything, and is not necessarily beneficial to the monster
        mon_action_defend sp_defense;

        // Monster upgrade variables
        bool upgrades;
        int half_life;
        int age_grow;
        mtype_id upgrade_into;
        mongroup_id upgrade_group;
        mtype_id burn_into;

        // Monster reproduction variables
        bool reproduces;
        int baby_timer;
        int baby_count;
        mtype_id baby_monster;
        itype_id baby_egg;
        std::vector<std::string> baby_flags;

        // Monster biosignature variables
        bool biosignatures;
        int biosig_timer;
        itype_id biosig_item;

        // Monster's ability to destroy terrain and vehicles
        int bash_skill;

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

        /** Emission sources that cycle each turn the monster remains alive */
        std::set<emit_id> emit_fields;

        pathfinding_settings path_settings;

        // Used to fetch the properly pluralized monster type name
        std::string nname( unsigned int quantity = 1 ) const;
        bool has_special_attack( const std::string &attack_name ) const;
        bool has_flag( m_flag flag ) const;
        bool has_flag( const std::string &flag ) const;
        bool made_of( const material_id &material ) const;
        void set_flag( const std::string &flag, bool state );
        bool has_anger_trigger( monster_trigger trigger ) const;
        bool has_fear_trigger( monster_trigger trigger ) const;
        bool has_placate_trigger( monster_trigger trigger ) const;
        bool in_category( const std::string &category ) const;
        bool in_species( const species_id &spec ) const;
        bool in_species( const species_type &spec ) const;
        //Used for corpses.
        field_id bloodType() const;
        field_id gibType() const;
        // The item id of the meat items that are produced by this monster (or "null")
        // if there is no matching item type. e.g. "veggy" for plant monsters.
        itype_id get_meat_itype() const;
        int get_meat_chunks_count() const;
        std::string get_description() const;

        // Historically located in monstergenerator.cpp
        void load( JsonObject &jo, const std::string &src );
};

mon_effect_data load_mon_effect_data( JsonObject &e );

#endif

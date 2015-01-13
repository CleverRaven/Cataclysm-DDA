#ifndef MTYPE_H
#define MTYPE_H
// SEE ALSO: monitemsdef.cpp, which defines data on which items any given
// monster may carry.

#include <bitset>
#include <string>
#include <vector>
#include <set>
#include <math.h>
#include "mondeath.h"
#include "monattack.h"
#include "mondefense.h"
#include "material.h"
#include "enums.h"
#include "color.h"
#include "field.h"

typedef std::string itype_id;

enum m_size {
    MS_TINY = 0,    // Squirrel
    MS_SMALL,      // Dog
    MS_MEDIUM,    // Human
    MS_LARGE,    // Cow
    MS_HUGE     // TAAAANK
};

// These are triggers which may affect the monster's anger or morale.
// They are handled in monster::check_triggers(), in monster.cpp
enum monster_trigger {
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
// mfb(n) converts a flag to its appropriate position in mtype's bitfield
#ifndef mfb
#define mfb(n) static_cast <unsigned long> (1 << (n))
#endif
enum m_flag {
    MF_NULL = 0,            //
    MF_SEES,                // It can see you (and will run/follow)
    MF_VIS50,               // Vision -10
    MF_VIS40,               // Vision -20
    MF_VIS30,               // Vision -30
    MF_VIS20,               // Vision -40
    MF_VIS10,               // Vision -50
    MF_HEARS,               // It can hear you
    MF_GOODHEARING,         // Pursues sounds more than most monsters
    MF_SMELLS,              // It can smell you
    MF_KEENNOSE,            //Keen sense of smell
    MF_STUMBLES,            // Stumbles in its movement
    MF_WARM,                // Warm blooded
    MF_NOHEAD,              // Headshots not allowed!
    MF_HARDTOSHOOT,         // Some shots are actually misses
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
    MF_SLUDGEPROOF,         // Ignores the effect of sludge trails
    MF_SLUDGETRAIL,         // Causes monster to leave a sludge trap trail when moving
    MF_LEAKSGAS,            // Occasionally leaks gas when moving
    MF_FIREY,               // Burns stuff and is immune to fire
    MF_QUEEN,               // When it dies, local populations start to die off too
    MF_ELECTRONIC,          // e.g. a robot; affected by emp blasts, and other stuff
    MF_FUR,                 // May produce fur when butchered
    MF_LEATHER,             // May produce leather when butchered
    MF_FEATHER,             // May produce feather when butchered
    MF_CBM_CIV,             // May produce a common cbm or two when butchered
    MF_BONES,               // May produce bones and sinews when butchered; if combined with POISON flag, tainted bones, if combined with HUMAN, human bones
    MF_FAT,                 // May produce fat when butchered; if combined with POISON flag, tainted fat
    MF_IMMOBILE,            // Doesn't move (e.g. turrets)
    MF_HIT_AND_RUN,         // Flee for several turns after a melee attack
    MF_GUILT,               // You feel guilty for killing it
    MF_HUMAN,               // It's a live human, as long as it's alive
    MF_NO_BREATHE,          // Creature can't drown and is unharmed by gas, smoke, or poison
    MF_REGENERATES_50,      // Monster regenerates very quickly over time
    MF_REGENERATES_10,      // Monster regenerates very quickly over time
    MF_FLAMMABLE,           // Monster catches fire, burns, and spreads fire to nearby objects
    MF_REVIVES,             // Monster corpse will revive after a short period of time
    MF_CHITIN,              // May produce chitin when butchered
    MF_VERMIN,              // Creature is too small for normal combat, butchering, etc.
    MF_NOGIB,               // Creature won't leave gibs / meat chunks when killed with huge damage.
    MF_HUNTS_VERMIN,        // Creature uses vermin as a food source
    MF_SMALL_BITER,         // Creature can cause a painful, non-damaging bite
    MF_LARVA,               // Creature is a larva. Currently used for gib and blood handling.
    MF_ARTHROPOD_BLOOD,     // Forces monster to bleed hemolymph.
    MF_ACID_BLOOD,          // Makes monster bleed acid. Fun stuff! Does not automatically dissolve in a pool of acid on death.
    MF_BILE_BLOOD,          // Makes monster bleed bile.
    MF_ABSORBS,             // Consumes objects it moves over.
    MF_REGENMORALE,         // Will stop fleeing if at max hp, and regen anger and morale to positive values.
    MF_CBM_POWER,           // May produce a power CBM when butchered, independent of MF_CBM_wev.
    MF_CBM_SCI,             // May produce a bionic from bionics_sci when butchered.
    MF_CBM_OP,              // May produce a bionic from bionics_op when butchered, and the power storage is mk 2.
    MF_CBM_TECH,            // May produce a bionic from bionics_tech when butchered.
    MF_CBM_SUBS,            // May produce a bionic from bionics_subs when butchered.
    MF_FISHABLE,            // Its fishable.
    MF_GROUP_BASH,          // Monsters that can pile up against obstacles and add their strength together to break them.
    MF_SWARMS,              // Monsters that like to group together and form loose packs
    MF_GROUP_MORALE,        // Monsters that are more courageous when near friends
    MF_MAX                  // Sets the length of the flags - obviously must be LAST
};

/** Used to store monster effects placed on attack */
struct mon_effect_data
{
    std::string id;
    int duration;
    body_part bp;
    bool permanent;
    int chance;
    
    mon_effect_data(std::string nid, int dur, body_part nbp, bool perm, int nchance) :
                    id(nid), duration(dur), bp(nbp), permanent(perm), chance(nchance) {};
};

struct mtype {
    private:
        friend class MonsterGenerator;
        std::string name;
        std::string name_plural;
    public:
        std::string id;
        std::string description;
        std::set<std::string> species, categories;
        std::set< int > species_id;
        int default_faction; // 0 is factionless, -1 player ally, -2 by species
        /** UTF-8 encoded symbol, should be exactyle one cell wide. */
        std::string sym;
        nc_color color;
        m_size size;
        std::string mat;
        phase_id phase;
        std::set<m_flag> flags;
        std::set<monster_trigger> anger, placate, fear;

        std::bitset<MF_MAX> bitflags;
        std::bitset<N_MONSTER_TRIGGERS> bitanger, bitfear, bitplacate;
        
        /** Stores effect data for effects placed on attack */
        std::vector<mon_effect_data> atk_effs;

        int difficulty; // Used all over; 30 min + (diff-3)*30 min = earliest appearance
        int agro;       // How likely to attack; -100 to 100
        int morale;     // Default morale level

        // Vision range is linearly scaled depending on lighting conditions
        int vision_day;  // Vision range in bright light
        int vision_night; // Vision range in total darkness

        int  speed;       // Speed; human = 100
        unsigned char melee_skill; // Melee hit skill, 20 is superhuman hitting abilities.
        unsigned char melee_dice;  // Number of dice on melee hit
        unsigned char melee_sides; // Number of sides those dice have
        unsigned char melee_cut;   // Bonus cutting damage
        unsigned char sk_dodge;    // Dodge skill; should be 0 to 5
        unsigned char armor_bash;  // Natural armor vs. bash
        unsigned char armor_cut;   // Natural armor vs. cut
        std::map<std::string, int> starting_ammo; // Amount of ammo the monster spawns with.
        // Name of item group that is used to create item dropped upon death, or empty.
        std::string death_drops;
        float luminance;           // 0 is default, >0 gives luminance to lightmap
        int hp;
        std::vector<unsigned int> sp_freq;     // How long sp_attack takes to charge
        std::vector<void (mdeath::*)(monster *)> dies; // What happens when this monster dies
        unsigned int def_chance; // How likely a special "defensive" move is to trigger (0-100%, default 0)
        std::vector<void (mattack::*)(monster *, int index)> sp_attack; // This monster's special attack
        // This monster's special "defensive" move that may trigger when the monster is attacked.
        // Note that this can be anything, and is not necessarily beneficial to the monster
        void (mdefense::*sp_defense)(monster *, Creature *, const projectile *);
        // Default constructor
        mtype ();
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

        // Used to fetch the properly pluralized monster type name
        std::string nname(unsigned int quantity = 1) const;
        bool has_flag(m_flag flag) const;
        bool has_flag(std::string flag) const;
        void set_flag(std::string flag, bool state);
        bool has_anger_trigger(monster_trigger trigger) const;
        bool has_fear_trigger(monster_trigger trigger) const;
        bool has_placate_trigger(monster_trigger trigger) const;
        bool in_category(std::string category) const;
        bool in_species(std::string _species) const;
        bool in_species( int spec_id ) const;
        //Used for corpses.
        field_id bloodType () const;
        field_id gibType () const;
        // The item id of the meat items that are produced by this monster (or "null")
        // if there is no matching item type. e.g. "veggy" for plant monsters.
        itype_id get_meat_itype() const;
};

#endif

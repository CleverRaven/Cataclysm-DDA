#ifndef _MTYPE_H_
#define _MTYPE_H_
// SEE ALSO: monitemsdef.cpp, which defines data on which items any given
// monster may carry.

#include <bitset>
#include <string>
#include <vector>
#include <set>
#include <math.h>
#include "mondeath.h"
#include "monattack.h"
#include "material.h"
#include "enums.h"
#include "color.h"

/*
  On altering any entries in this enum please add or remove the appropriate entry to the monster_names array in tile_id_data.h
*/
enum mon_id {
    mon_null = 0,
    // Wildlife
    mon_squirrel, mon_rabbit, mon_deer, mon_moose, mon_wolf, mon_coyote, mon_bear, mon_cougar, mon_crow,
    // Friendly animals
    mon_dog, mon_cat,
    // Ants
    mon_ant_larva, mon_ant, mon_ant_soldier, mon_ant_queen, mon_ant_fungus,
    // Bees
    mon_fly, mon_bee, mon_wasp,
    // Worms
    mon_graboid, mon_worm, mon_halfworm,
    // Wild Mutants
    mon_sludge_crawler,
    // Zombies
    mon_zombie, mon_zombie_cop, mon_zombie_shrieker, mon_zombie_spitter, mon_zombie_electric,
    mon_zombie_smoker, mon_zombie_swimmer,
    mon_zombie_dog, mon_zombie_brute, mon_zombie_hulk, mon_zombie_fungus,
    mon_boomer, mon_boomer_fungus, mon_skeleton, mon_zombie_necro,
    mon_zombie_scientist, mon_zombie_soldier, mon_zombie_grabber,
    mon_zombie_master,  mon_beekeeper, mon_zombie_child, mon_zombie_fireman, mon_zombie_survivor,
    // Flesh Golem
    mon_jabberwock,
    // Triffids
    mon_triffid, mon_triffid_young, mon_triffid_queen, mon_creeper_hub,
    mon_creeper_vine, mon_biollante, mon_vinebeast, mon_triffid_heart,
    // Fungaloids TODO: Remove dormant fungaloid when it won't break save compatibility
    mon_fungaloid, mon_fungaloid_dormant, mon_fungaloid_young, mon_spore,
    mon_fungaloid_queen, mon_fungal_wall,
    // Blobs
    mon_blob, mon_blob_small,
    // Sewer mutants
    mon_chud, mon_one_eye, mon_crawler,
    // Sewer animals
    mon_sewer_fish, mon_sewer_snake, mon_sewer_rat, mon_rat_king,
    // Swamp monsters
    mon_mosquito, mon_dragonfly, mon_centipede, mon_frog, mon_slug,
    mon_dermatik_larva, mon_dermatik,
    // SPIDERS
    mon_spider_wolf, mon_spider_web, mon_spider_jumping, mon_spider_trapdoor,
    mon_spider_widow,
    // Unearthed Horrors
    mon_dark_wyrm, mon_amigara_horror, mon_dog_thing, mon_headless_dog_thing,
    mon_thing,
    // Spiral monsters
    mon_human_snail, mon_twisted_body, mon_vortex,
    // Subspace monsters
    mon_flying_polyp, mon_hunting_horror, mon_mi_go, mon_yugg, mon_gelatin,
    mon_flaming_eye, mon_kreck, mon_gracke, mon_blank, mon_gozu, mon_shadow, mon_breather_hub,
    mon_breather, mon_shadow_snake, mon_shoggoth,
    // Cult, lobotomized creatures that are human/undead hybrids
    mon_dementia, mon_homunculus, mon_blood_sacrifice, mon_flesh_angel,
    // Robots
    mon_eyebot, mon_manhack, mon_skitterbot, mon_secubot, mon_hazmatbot, mon_copbot, mon_molebot,
    mon_tripod, mon_chickenbot, mon_tankbot, mon_turret, mon_exploder,
    // Hallucinations
    mon_hallu_mom,
    // Special monsters
    mon_generator,
    // 0.8 -> 0.9
    mon_turkey, mon_raccoon, mon_opossum, mon_rattlesnake,
    mon_giant_crayfish, mon_fungal_fighter,
    mon_black_rat,
    mon_mosquito_giant, mon_dragonfly_giant, mon_centipede_giant,
    mon_frog_giant, mon_slug_giant,
    mon_spider_jumping_giant, mon_spider_trapdoor_giant,
    mon_spider_web_giant, mon_spider_widow_giant, mon_spider_wolf_giant,
    mon_bat, mon_beaver, mon_bobcat,
    mon_chicken, mon_chipmunk, mon_cow, mon_coyote_wolf,
    mon_deer_mouse, mon_fox_gray, mon_fox_red,
    mon_groundhog, mon_hare, mon_horse, mon_lemming,
    mon_mink, mon_muskrat, mon_otter, mon_pig,
    mon_sheep, mon_shrew, mon_squirrel_red,
    mon_weasel,
    // 0.9 -> 0.10
    mon_dog_skeleton, mon_dog_zombie_cop, mon_dog_zombie_rot,
    num_monsters
};

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
    MTRIG_PLAYER_WEAK, // The player is hurt
    MTRIG_PLAYER_CLOSE, // The player gets within a few tiles
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
    MF_NULL = 0,            // Helps with setvector
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
    MF_FIREY,               // Burns stuff and is immune to fire
    MF_QUEEN,               // When it dies, local populations start to die off too
    MF_ELECTRONIC,          // e.g. a robot; affected by emp blasts, and other stuff
    MF_FUR,                 // May produce fur when butchered
    MF_LEATHER,             // May produce leather when butchered
    MF_FEATHER,             // May produce feather when butchered
    MF_CBM,                 // May produce a cbm or two when butchered
    MF_BONES,               // May produce bones and sinews when butchered
    MF_FAT,                 // May produce fat when butchered
    MF_IMMOBILE,            // Doesn't move (e.g. turrets)
    MF_FRIENDLY_SPECIAL,    // Use our special attack, even if friendly
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
    MF_HUNTS_VERMIN,        // Creature uses vermin as a food source
    MF_SMALL_BITER,         // Creature can cause a painful, non-damaging bite
    MF_LARVA,               // Creature is a larva. Currently used for gib and blood handling.
    MF_ARTHROPOD_BLOOD,     // Forces monster to bleed hemolymph.
    MF_ACID_BLOOD,          // Makes monster bleed acid. Fun stuff!
    MF_ABSORBS,             // Consumes objects it moves over.
    MF_MAX                  // Sets the length of the flags - obviously must be LAST
};

struct mtype {
    std::string id, name, description;
    std::set<std::string> species, categories;
    long sym;
    nc_color color;
    m_size size;
    std::string mat;
    phase_id phase;
    std::set<m_flag> flags;
    std::set<monster_trigger> anger, placate, fear;

    std::bitset<MF_MAX> bitflags;
    std::bitset<N_MONSTER_TRIGGERS> bitanger, bitfear, bitplacate;

    int difficulty; // Used all over; 30 min + (diff-3)*30 min = earlist appearance
    int agro;       // How likely to attack; -100 to 100
    int morale;     // Default morale level

    unsigned int  speed;       // Speed; human = 100
    unsigned char melee_skill; // Melee hit skill, 20 is superhuman hitting abilities.
    unsigned char melee_dice;  // Number of dice on melee hit
    unsigned char melee_sides; // Number of sides those dice have
    unsigned char melee_cut;   // Bonus cutting damage
    unsigned char sk_dodge;    // Dodge skill; should be 0 to 5
    unsigned char armor_bash;  // Natural armor vs. bash
    unsigned char armor_cut;   // Natural armor vs. cut
    signed char item_chance;   // Higher # means higher chance of loot
                               // Negative # means one item gen'd, tops
    float luminance;           // 0 is default, >0 gives luminance to lightmap
    int hp;
    unsigned int sp_freq;     // How long sp_attack takes to charge
    void (mdeath::*dies)(monster *); // What happens when this monster dies
    void (mattack::*sp_attack)(monster *); // This monster's special attack

    // Default constructor
    mtype ();

    bool has_flag(m_flag flag) const;
    bool has_anger_trigger(monster_trigger trigger) const;
    bool has_fear_trigger(monster_trigger trigger) const;
    bool has_placate_trigger(monster_trigger trigger) const;
    bool in_category(std::string category) const;
    bool in_species(std::string _species) const;
};

#endif

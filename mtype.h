#ifndef _MTYPE_H_
#define _MTYPE_H_
// SEE ALSO: monitemsdef.cpp, which defines data on which items any given
// monster may carry.

#include <string>
#include <vector>
#include <math.h>
#include "mondeath.h"
#include "monattack.h"
#include "material.h"
#include "enums.h"
#include "color.h"

class mdeath;

enum monster_species {
species_none = 0,
species_mammal,
species_insect,
species_worm,
species_zombie,
species_plant,
species_fungus,
species_nether,
species_robot,
species_hallu,
num_species
};

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
 mon_zombie_smoker,
 mon_zombie_fast, mon_zombie_brute, mon_zombie_hulk, mon_zombie_fungus,
 mon_boomer, mon_boomer_fungus, mon_skeleton, mon_zombie_necro,
 mon_zombie_scientist, mon_zombie_soldier, mon_zombie_grabber,
 mon_zombie_master,  mon_beekeeper, mon_zombie_child,
// Flesh Golem
 mon_jabberwock,
// Triffids
mon_triffid, mon_triffid_young, mon_triffid_queen, mon_creeper_hub,
 mon_creeper_vine, mon_biollante, mon_vinebeast, mon_triffid_heart,
// Fungaloids
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
 mon_breather, mon_shadow_snake,
// Cult, lobotomized creatures that are human/undead hybrids
mon_dementia, mon_homunculus, mon_blood_sacrifice, mon_flesh_angel,
// Robots
mon_eyebot, mon_manhack, mon_skitterbot, mon_secubot, mon_hazmatbot, mon_copbot, mon_molebot,
 mon_tripod, mon_chickenbot, mon_tankbot, mon_turret, mon_exploder,
// Hallucinations
mon_hallu_zom, mon_hallu_bee, mon_hallu_ant, mon_hallu_mom,
// Special monsters
mon_generator,
num_monsters
};

enum m_size {
MS_TINY = 0,	// Rodent
MS_SMALL,	// Half human
MS_MEDIUM,	// Human
MS_LARGE,	// Cow
MS_HUGE		// TAAAANK
};

// These are triggers which may affect the monster's anger or morale.
// They are handled in monster::check_triggers(), in monster.cpp
enum monster_trigger {
MTRIG_NULL = 0,
MTRIG_STALK,		// Increases when following the player
MTRIG_MEAT,		// Meat or a corpse nearby
MTRIG_PLAYER_WEAK,	// The player is hurt
MTRIG_PLAYER_CLOSE,	// The player gets within a few tiles
MTRIG_HURT,		// We are hurt
MTRIG_FIRE,		// Fire nearby
MTRIG_FRIEND_DIED,	// A monster of the same type died
MTRIG_FRIEND_ATTACKED,	// A monster of the same type attacked
MTRIG_SOUND,		// Heard a sound
N_MONSTER_TRIGGERS
};

// These functions are defined at the bottom of mtypedef.cpp
std::vector<monster_trigger> default_anger(monster_species spec);
std::vector<monster_trigger> default_fears(monster_species spec);


// Feel free to add to m_flags.  Order shouldn't matter, just keep it tidy!
// And comment them well. ;)
// mfb(n) converts a flag to its appropriate position in mtype's bitfield
#ifndef mfb
#define mfb(n) static_cast <unsigned long> (1 << (n))
#endif
enum m_flag {
MF_NULL = 0,	// Helps with setvector
MF_SEES,	// It can see you (and will run/follow)
MF_VIS50, //Vision -10
MF_VIS40, //Vision -20
MF_VIS30, //Vision -30
MF_VIS20, //Vision -40
MF_VIS10, //Vision -50
MF_HEARS,	// It can hear you
MF_GOODHEARING,	// Pursues sounds more than most monsters
MF_SMELLS,	// It can smell you
MF_KEENNOSE, //Keen sense of smell
MF_STUMBLES,	// Stumbles in its movement
MF_WARM,	// Warm blooded
MF_NOHEAD,	// Headshots not allowed!
MF_HARDTOSHOOT,	// Some shots are actually misses
MF_GRABS,	// Its attacks may grab us!
MF_BASHES,	// Bashes down doors
MF_DESTROYS,	// Bashes down walls and more
MF_POISON,	// Poisonous to eat
MF_VENOM,	// Attack may poison the player
MF_BADVENOM,	// Attack may SEVERELY poison the player
MF_BLEED,       // Causes player to bleed
MF_WEBWALK,	// Doesn't destroy webs
MF_DIGS,	// Digs through the ground
MF_FLIES,	// Can fly (over water, etc)
MF_AQUATIC,	// Confined to water
MF_SWIMS,	// Treats water as 50 movement point terrain
MF_ATTACKMON,	// Attacks other monsters
MF_ANIMAL,	// Is an "animal" for purposes of the Animal Empath trait
MF_PLASTIC,	// Absorbs physical damage to a great degree
MF_SUNDEATH,	// Dies in full sunlight
MF_ELECTRIC,	// Shocks unarmed attackers
MF_ACIDPROOF,	// Immune to acid
MF_ACIDTRAIL,	// Leaves a trail of acid
MF_SLUDGEPROOF, // Ignores the effect of sludge trails
MF_SLUDGETRAIL, // Causes monster to leave a sludge trap trail when moving
MF_FIREY,	// Burns stuff and is immune to fire
MF_QUEEN,	// When it dies, local populations start to die off too
MF_ELECTRONIC,	// e.g. a robot; affected by emp blasts, and other stuff
MF_FUR,		// May produce fur when butchered
MF_LEATHER,	// May produce leather when butchered
MF_FEATHER, // May produce feather when butchered
MF_CBM, // May produce a cbm or two when butchered
MF_BONES, // May produce bones and sinews when butchered
MF_IMMOBILE,	// Doesn't move (e.g. turrets)
MF_FRIENDLY_SPECIAL, // Use our special attack, even if friendly
MF_HIT_AND_RUN,	// Flee for several turns after a melee attack
MF_GUILT,	// You feel guilty for killing it
MF_HUMAN,	// It's a live human
MF_NO_BREATHE, //Provides immunity to inhalation effects from gas, smoke, and poison
MF_REGENERATES_50, // Monster regenerates very quickly over time
MF_FLAMMABLE, // Monster catches fire, burns, and passes the fire on to nearby objects
MF_MAX		// Sets the length of the flags - obviously MUST be last
};

enum m_category {
MC_NULL = 0, // No category.
MC_CLASSIC, // Only monsters we expect in a classic zombie movie.
MC_WILDLIFE, // The natural animals.
MC_MAX // Size of flag array.
};

struct mtype {
 int id;
 std::string name;
 std::string description;
 monster_species species;
 long sym;	// Symbol on the map
 nc_color color;// Color of symbol (see color.h)

 m_size size;
 std::string mat;	// See materials.json for material list.  Generally, flesh; veggy?
 phase_id phase;
 std::vector<m_flag> flags;
 std::vector<m_category> categories;
 std::vector<monster_trigger> anger;   // What angers us?
 std::vector<monster_trigger> placate; // What reduces our anger?
 std::vector<monster_trigger> fear;    // What are we afraid of?

 int difficulty;// Used all over; 30 min + (diff-3)*30 min = earlist appearance
 int agro;	// How likely to attack; -100 to 100
 int morale;	// Default morale level

 unsigned int  speed;		// Speed; human = 100
 unsigned char melee_skill;	// Melee hit skill, 20 is superhuman hitting abilities.
 unsigned char melee_dice;	// Number of dice on melee hit
 unsigned char melee_sides;	// Number of sides those dice have
 unsigned char melee_cut;	// Bonus cutting damage
 unsigned char sk_dodge;	// Dodge skill; should be 0 to 5
 unsigned char armor_bash;	// Natural armor vs. bash
 unsigned char armor_cut;	// Natural armor vs. cut
 signed char item_chance;	// Higher # means higher chance of loot
				// Negative # means one item gen'd, tops
 int hp;
 unsigned char sp_freq;			// How long sp_attack takes to charge
 void (mdeath::*dies)(game *, monster *); // What happens when this monster dies
 void (mattack::*sp_attack)(game *, monster *); // This monster's special attack


 // Default constructor
 mtype ();
 // Non-default (messy)
 mtype (int pid, std::string pname, monster_species pspecies, char psym,
        nc_color pcolor, m_size psize, std::string pmat,
	    unsigned int pdiff, signed char pagro,
        signed char pmorale, unsigned int pspeed, unsigned char pml_skill,
        unsigned char pml_dice, unsigned char pml_sides, unsigned char pml_cut,
        unsigned char pdodge, unsigned char parmor_bash,
        unsigned char parmor_cut, signed char pitem_chance, int php,
        unsigned char psp_freq,
        void (mdeath::*pdies)      (game *, monster *),
        void (mattack::*psp_attack)(game *, monster *),
        std::string pdescription );

 bool has_flag(m_flag flag);
 bool in_category(m_category category);
};

#endif

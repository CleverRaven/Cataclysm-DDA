#ifndef _MTYPE_H_
#define _MTYPE_H_
// SEE ALSO: monitemsdef.cpp, which defines data on which items any given
// monster may carry.

#include <string>
#include <math.h>
#include "mondeath.h"
#include "monattack.h"
#include "enums.h"
#include "color.h"

class mdeath;

enum mon_id {
mon_null = 0,
// Wildlife
mon_squirrel, mon_rabbit, mon_deer, mon_wolf, mon_bear,
// Friendly animals
mon_dog,
// Ants
mon_ant_larva, mon_ant, mon_ant_soldier, mon_ant_queen, mon_ant_fungus,
// Bees
mon_bee,
// Worms
mon_graboid, mon_worm, mon_halfworm,
// Zombies
mon_zombie, mon_zombie_shrieker, mon_zombie_spitter, mon_zombie_electric,
 mon_zombie_fast, mon_zombie_brute, mon_zombie_hulk, mon_zombie_fungus,
 mon_boomer, mon_boomer_fungus, mon_skeleton, mon_zombie_necro,
 mon_zombie_scientist,
// Triffids
mon_triffid, mon_triffid_young, mon_triffid_queen,
// Fungaloids
mon_fungaloid, mon_fungaloid_dormant, mon_spore,
// Blobs
mon_blob, mon_blob_small,
// Sewer mutants
mon_chud, mon_one_eye, mon_crawler,
// Sewer animals
mon_sewer_fish, mon_sewer_snake, mon_sewer_rat,
// Subspace monsters
mon_flying_polyp, mon_hunting_horror, mon_mi_go, mon_yugg, mon_gelatin,
 mon_flaming_eye, mon_kreck, mon_blank,
// Robots
mon_eyebot, mon_manhack, mon_skitterbot, mon_secubot, mon_molebot, mon_tripod,
 mon_chickenbot, mon_tankbot, mon_turret,
// Hallucinations
mon_hallu_zom, mon_hallu_bee, mon_hallu_ant, mon_hallu_mom,
num_monsters
};

enum m_size {
MS_TINY = 0,	// Rodent
MS_SMALL,	// Half human
MS_MEDIUM,	// Human
MS_LARGE,	// Cow
MS_HUGE		// TAAAANK
};

// Feel free to add to m_flags.  Order shouldn't matter, just keep it tidy!
// There is a maximum number of 32 flags, including MF_MAX, so...
// And comment them well. ;)
// mfb(n) converts a flag to its appropriate position in mtype's bitfield
#ifndef mfb
#define mfb(n) int(pow(2,(int)n))
#endif
enum m_flags {
MF_SEES,	// It can see you (and will run/follow)
MF_HEARS,	// It can hear you
MF_GOODHEARING,	// Pursues sounds more than most monsters
MF_SMELLS,	// It can smell you
MF_STUMBLES,	// Stumbles in its movement
MF_WARM,	// Warm blooded
MF_NOHEAD,	// Headshots not allowed!
MF_HARDTOSHOOT,	// Some shots are actually misses
MF_BASHES,	// Bashes down doors
MF_DESTROYS,	// Bashes down walls and more
MF_POISON,	// Poisonous to eat
MF_VENOM,	// Attack may poison the player
MF_DIGS,	// Digs through the ground
MF_FLIES,	// Can fly (over water, etc)
MF_AQUATIC,	// Confined to water
MF_SWIMS,	// Treats water as 50 movement point terrain
MF_ATTACKMON,	// Attacks other monsters
MF_ANIMAL,	// Is an "animal" for purposes of the Animal Empath trait
MF_PLASTIC,	// Absorbs physical damage to a great degree
MF_SUNDEATH,	// Dies in full sunlight
MF_ACIDPROOF,	// Immune to acid
MF_FIREY,	// Burns stuff and is immune to fire
MF_SHOCK,	// Shocks the player if they attack w/out gloves
MF_ELECTRONIC,	// e.g. a robot; affected by emp blasts, and other stuff
MF_FUR,		// May produce fur when butchered.
MF_LEATHER,	// May produce leather when butchered
MF_IMMOBILE,	// Doesn't move (e.g. turrets)
MF_MAX		// Sets the length of the flags - obviously MUST be last
};

struct mtype {
 int id;
 std::string name;
 std::string description;
 char sym;	// Symbol on the map
 nc_color color;// Color of symbol (see color.h)

 m_size size;
 material mat;	// See enums.h for material list.  Generally, flesh; veggy?
 unsigned flags : MF_MAX; // Bitfield of m_flags

 unsigned char frequency;	// How often do these show up? 0 (never) to ??
 int difficulty;// Used all over; 30 min + (diff-3)*30 min = earlist appearance
 signed char agro;		// How likely to attack; -5 to 5

 unsigned int speed;		// Speed; human = 100
 unsigned char melee_skill;	// Melee skill; should be 0 to 5
 unsigned char melee_dice;	// Number of dice on melee hit
 unsigned char melee_sides;	// Number of sides those dice have
 unsigned char melee_cut;	// Bonus cutting damage
 unsigned char sk_dodge;	// Dodge skill; should be 0 to 5
 unsigned char armor;		// Natural armor
 signed char item_chance;	// Higher # means higher chance of loot
				// Negative # means one item gen'd, tops
 int hp;

 unsigned char sp_freq;			// How long sp_attack takes to charge
 void (mdeath::*dies)(game *, monster *); // What happens when this monster dies
 void (mattack::*sp_attack)(game *, monster *); // This monster's special attack
 

 // Default constructor
 mtype () {
  id = 0;
  name = "human";
  description = "";
  sym = ' ';
  color = c_white;
  size = MS_MEDIUM;
  mat = FLESH;
  flags = 0;
  difficulty = 0;
  frequency = 0;
  agro = 0;
  speed = 0;
  melee_skill = 0;
  melee_dice = 0;
  melee_sides = 0;
  melee_cut = 0;
  sk_dodge = 0;
  armor = 0;
  hp = 0;
  sp_freq = 0;
  item_chance = 0;
  dies = NULL;
  sp_attack = NULL;
 }
 // Non-default (messy)
 mtype (int pid, std::string pname, char psym, nc_color pcolor, m_size psize,
        material pmat, unsigned pflags, unsigned char pfreq, unsigned int pdiff,
        signed char pagro, unsigned int pspeed, unsigned char pml_skill,
        unsigned char pml_dice, unsigned char pml_sides, unsigned char pml_cut,
        unsigned char pdodge, unsigned char parmor, signed char pitem_chance,
        int php, unsigned char psp_freq,
        void (mdeath::*pdies)      (game *, monster *),
        void (mattack::*psp_attack)(game *, monster *),
        std::string pdescription ) { 
  id = pid; 
  name = pname; 
  sym = psym;
  color = pcolor;
  size = psize;
  mat = pmat;
  flags = pflags;
  frequency = pfreq;
  difficulty = pdiff;
  agro = pagro;
  speed = pspeed;
  melee_skill = pml_skill;
  melee_dice = pml_dice;
  melee_sides = pml_sides;
  melee_cut = pml_cut;
  sk_dodge = pdodge;
  armor = parmor;
  item_chance = pitem_chance;
  hp = php;
  sp_freq = psp_freq;
  dies = pdies;
  sp_attack = psp_attack;
  description = pdescription;
 }
};

#endif

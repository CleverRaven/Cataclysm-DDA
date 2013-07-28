#include "game.h"
#include "mondeath.h"
#include "monattack.h"
#include "itype.h"
#include "setvector.h"


 // Default constructor
 mtype::mtype () {
  id = 0;
  name = _("human");
  description = "";
  species = species_none;
  sym = ' ';
  color = c_white;
  size = MS_MEDIUM;
  mat = "hflesh";
  phase = SOLID;
  difficulty = 0;
  agro = 0;
  morale = 0;
  speed = 0;
  melee_skill = 0;
  melee_dice = 0;
  melee_sides = 0;
  melee_cut = 0;
  sk_dodge = 0;
  armor_bash = 0;
  armor_cut = 0;
  hp = 0;
  sp_freq = 0;
  item_chance = 0;
  dies = NULL;
  sp_attack = NULL;
  flags.push_back(MF_HUMAN);
 }
 // Non-default (messy)
 mtype::mtype (int pid, std::string pname, monster_species pspecies, char psym,
        nc_color pcolor, m_size psize, std::string pmat,
	    unsigned int pdiff, signed char pagro,
        signed char pmorale, unsigned int pspeed, unsigned char pml_skill,
        unsigned char pml_dice, unsigned char pml_sides, unsigned char pml_cut,
        unsigned char pdodge, unsigned char parmor_bash,
        unsigned char parmor_cut, signed char pitem_chance, int php,
        unsigned char psp_freq,
        void (mdeath::*pdies)      (game *, monster *),
        void (mattack::*psp_attack)(game *, monster *),
        std::string pdescription ) {
  id = pid;
  name = pname;
  species = pspecies;
  sym = psym;
  color = pcolor;
  size = psize;
  mat = pmat;
  difficulty = pdiff;
  agro = pagro;
  morale = pmorale;
  speed = pspeed;
  melee_skill = pml_skill;
  melee_dice = pml_dice;
  melee_sides = pml_sides;
  melee_cut = pml_cut;
  sk_dodge = pdodge;
  armor_bash = parmor_bash;
  armor_cut = parmor_cut;
  item_chance = pitem_chance;
  hp = php;
  sp_freq = psp_freq;
  dies = pdies;
  sp_attack = psp_attack;
  description = pdescription;

  anger = default_anger(species);
  fear = default_fears(species);
 }

 bool mtype::has_flag(m_flag flag)
 {
  for (int i = 0; i < flags.size(); i++) {
   if (flags[i] == flag)
    return true;
  }
  return false;
 }

 bool mtype::in_category(m_category category)
 {
  for (int i = 0; i < categories.size(); i++) {
   if (categories[i] == category)
    return true;
  }
  return false;
 }

// This function populates the master list of monster types.
// If you edit this function, you'll also need to edit:
//  * mtype.h - enum mon_id MUST match the order of this list!
//  * monster.cpp - void make_fungus() should be edited, or fungal infection
//                  will simply kill the monster
//  * mongroupdef.cpp - void init_mongroups() should be edited, so the monster
//                      spawns with the proper group
// PLEASE NOTE: The description is AT MAX 4 lines of 46 characters each.

void game::init_mtypes ()
{
 int id = 0;
// Null monster named "None".
 mtypes.push_back(new mtype);

#define mon(name, species, sym, color, size, mat,\
            diff, agro, morale, speed, melee_skill, melee_dice,\
            melee_sides, melee_cut, dodge, arm_bash, arm_cut, item_chance, HP,\
            sp_freq, death, sp_att, desc) \
id++;\
mtypes.push_back(new mtype(id, name, species, sym, color, size, mat,\
diff, agro, morale, speed, melee_skill, melee_dice, melee_sides,\
melee_cut, dodge, arm_bash, arm_cut, item_chance, HP, sp_freq, death, sp_att,\
desc))

#define FLAGS(...)   setvector(&(mtypes[id]->flags),   __VA_ARGS__, NULL)
#define CATEGORIES(...)   setvector(&(mtypes[id]->categories),   __VA_ARGS__, NULL)
#define ANGER(...)   setvector(&(mtypes[id]->anger),   __VA_ARGS__, NULL)
#define FEARS(...)   setvector(&(mtypes[id]->fear),    __VA_ARGS__, NULL)
#define PLACATE(...) setvector(&(mtypes[id]->placate), __VA_ARGS__, NULL)

// PLEASE NOTE: The description is AT MAX 4 lines of 46 characters each.

// FOREST ANIMALS
mon(_("squirrel"),	species_mammal, 'r',	c_ltgray,	MS_TINY,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,-99, -8,140,  0,  1,  1,  0,  4,  0,  0,  0,  1,  0,
	&mdeath::normal,	&mattack::none, _("\
A small woodland animal.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_ANIMAL, MF_WARM, MF_FUR, MF_BONES);
CATEGORIES(MC_WILDLIFE);

mon(_("rabbit"),	species_mammal, 'r',	c_white,	MS_TINY,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,-99, -7,180, 0,  0,  0,  0,  6,  0,  0,  0,  4,  0,
	&mdeath::normal,	&mattack::none, _("\
A cute wiggling nose, cotton tail, and\n\
delicious flesh.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_ANIMAL, MF_WARM, MF_FUR, MF_BONES);
CATEGORIES(MC_WILDLIFE);

mon(_("deer"),	species_mammal, 'd',	c_brown,	MS_LARGE,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1,-99, -5,300,  4,  3,  3,  0,  3,  0,  0,  0, 80, 0,
	&mdeath::normal,	&mattack::none, _("\
A large buck, fast-moving and strong.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_ANIMAL, MF_WARM, MF_FUR, MF_BONES);
CATEGORIES(MC_WILDLIFE);

mon(_("moose"), species_mammal, 'M',	c_brown,	MS_LARGE,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1,-10, 30,200,  10,  3,  4,  0,  1,  4,  1,  0, 100, 0,
	&mdeath::normal,	&mattack::none, _("\
A buck of the largest deer species.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_ANIMAL, MF_WARM, MF_FUR, MF_BLEED, MF_VIS40, MF_BONES);
ANGER(MTRIG_PLAYER_CLOSE, MTRIG_HURT);
FEARS(MTRIG_FIRE);
CATEGORIES(MC_WILDLIFE);

mon(_("wolf"),	species_mammal, 'w',	c_ltgray,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 12,  0, 20,165, 14,  2,  3,  4,  4,  1,  0,  0, 28,  0,
	&mdeath::normal,	&mattack::none, _("\
A grey wolf. A vicious and fast pack hunter.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_ANIMAL, MF_WARM, MF_FUR, MF_HIT_AND_RUN, MF_KEENNOSE, MF_BLEED, MF_ATTACKMON, MF_BONES, MF_VIS50);
ANGER(MTRIG_PLAYER_CLOSE, MTRIG_FRIEND_ATTACKED, MTRIG_PLAYER_WEAK, MTRIG_HURT);
PLACATE(MTRIG_MEAT);
FEARS(MTRIG_FIRE, MTRIG_FRIEND_DIED);
CATEGORIES(MC_WILDLIFE);

mon(_("coyote"),	species_mammal, 'w',	c_brown,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 10,  0, 20,155, 12,  2,  3,  2,  3,  1,  0,  0, 22,  0,
	&mdeath::normal,	&mattack::none, _("\
An eastern coyote, also called tweed wolf. It is an hybrid of grey wolves and the smaller western coyotes.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_ANIMAL, MF_WARM, MF_FUR, MF_HIT_AND_RUN, MF_KEENNOSE, MF_BLEED, MF_ATTACKMON, MF_BONES, MF_VIS50);
ANGER(MTRIG_PLAYER_CLOSE, MTRIG_FRIEND_ATTACKED, MTRIG_PLAYER_WEAK, MTRIG_HURT);
PLACATE(MTRIG_MEAT);
FEARS(MTRIG_FIRE, MTRIG_FRIEND_DIED);
CATEGORIES(MC_WILDLIFE);

mon(_("bear"),	species_mammal, 'B',	c_dkgray,	MS_LARGE,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 10,-10, 40,140, 10,  3,  4,  6,  3,  2,  0,  0, 90, 0,
	&mdeath::normal,	&mattack::none, _("\
An american black bear. Remember, only YOU can prevent forest fires.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_ANIMAL, MF_WARM, MF_FUR, MF_BLEED, MF_VIS40, MF_BONES);
ANGER(MTRIG_PLAYER_CLOSE, MTRIG_HURT);
PLACATE(MTRIG_MEAT);
FEARS(MTRIG_FIRE);
CATEGORIES(MC_WILDLIFE);

mon(_("cougar"),	species_mammal, 'C',	c_brown,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 12,  0, 20,180, 14,  2,  3,  4,  4,  1,  0,  0, 15,  5,
	&mdeath::normal,	&mattack::leap, _("\
A vicious and fast hunter.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_ANIMAL, MF_WARM, MF_FUR, MF_HIT_AND_RUN, MF_KEENNOSE, MF_BLEED, MF_VIS50, MF_BONES);
ANGER(MTRIG_STALK, MTRIG_PLAYER_WEAK, MTRIG_HURT, MTRIG_PLAYER_CLOSE);
PLACATE(MTRIG_MEAT);
FEARS(MTRIG_FIRE, MTRIG_FRIEND_DIED);
CATEGORIES(MC_WILDLIFE);

mon(_("crow"),	species_mammal, 'v',	c_dkgray,	MS_TINY,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 10,-99, -8,140,  0,  1,  1,  0,  4,  0,  0,  0,  1,  0,
	&mdeath::normal,	&mattack::none, _("\
A small woodland animal.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_ANIMAL, MF_WARM, MF_FLIES, MF_VIS40, MF_BONES, MF_FEATHER);
CATEGORIES(MC_WILDLIFE);

// DOMESICATED ANIMALS
mon(_("dog"),	species_mammal, 'd',	c_white,	MS_SMALL,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  5,  2, 15,150, 12,  2,  3,  3,  3,  0,  0,  0, 25,  0,
	&mdeath::normal,	&mattack::none, _("\
A medium-sized domesticated dog, gone feral.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_ANIMAL, MF_WARM, MF_FUR, MF_HIT_AND_RUN, MF_KEENNOSE, MF_BLEED, MF_ATTACKMON, MF_VIS40, MF_BONES);
ANGER(MTRIG_HURT);
PLACATE(MTRIG_MEAT);
FEARS(MTRIG_FIRE);
CATEGORIES(MC_WILDLIFE);

mon(_("cat"),	species_mammal, 'c',	c_white,	MS_TINY,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  -40, 15,150, 12,  1,  3,  2,  4,  0,  0,  0, 10,  0,
	&mdeath::normal,	&mattack::none, _("\
A small domesticated cat, gone feral.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_ANIMAL, MF_WARM, MF_FUR, MF_HIT_AND_RUN, MF_VIS40, MF_BONES);
ANGER(MTRIG_HURT);
PLACATE(MTRIG_MEAT);
FEARS(MTRIG_FIRE, MTRIG_SOUND);
CATEGORIES(MC_WILDLIFE);

// INSECTOIDS
mon(_("ant larva"),species_insect, 'a',	c_white,	MS_SMALL,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0, -1, 10, 65,  4,  1,  3,  0,  0,  0,  0,  0, 10,  0,
	&mdeath::normal,	&mattack::none, _("\
The size of a large cat, this pulsating mass\n\
of glistening white flesh turns your stomach.")
);
FLAGS(MF_SMELLS, MF_POISON);

mon(_("giant ant"),species_insect, 'a',	c_brown,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  7, 15, 60,100,  9,  1,  6,  4,  2,  4,  8,-40, 40,  0,
	&mdeath::normal,	&mattack::none, _("\
A red ant the size of a crocodile. It is\n\
covered in chitinous armor, and has a pair of\n\
vicious mandibles.")
);
FLAGS(MF_SMELLS);

mon(_("soldier ant"),species_insect, 'a',	c_blue,		MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 16, 25,100,115, 12,  2,  4,  6,  2,  5, 10,-50, 80,  0,
	&mdeath::normal,	&mattack::none, _("\
Darker in color than the other ants, this\n\
more aggresive variety has even larger\n\
mandibles.")
);
FLAGS(MF_SMELLS);

mon(_("queen ant"),species_insect, 'a',	c_ltred,	MS_LARGE,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
     13,  0,100, 60,  6,  3,  4,  4,  1,  6, 14,-40,180, 1,
	&mdeath::normal,	&mattack::antqueen, _("\
This ant has a long, bloated thorax, bulging\n\
with hundreds of small ant eggs.  It moves\n\
slowly, tending to nearby eggs and laying\n\
still more.")
);
FLAGS(MF_SMELLS, MF_QUEEN);

mon(_("fungal insect"),species_fungus, 'a',c_ltgray,	MS_MEDIUM,	"veggy",
//  dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
      5, 80,100, 75,  5,  1,  5,  3,  1,  1,  1,  0, 30, 60,
	&mdeath::normal,	&mattack::fungus, _("\
This insect is pale gray in color, its\n\
chitin weakened by the fungus sprouting\n\
from every joint on its body.")
);
FLAGS(MF_SMELLS, MF_POISON);

mon(_("giant fly"),species_insect, 'a',	c_ltgray,	MS_SMALL,	"flesh",
//	 dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  8, 50, 30,120,  3,  1,  3,  0,  5,  0,  0,  0, 25,  0,
	&mdeath::normal,	&mattack::none, _("\
A large housefly the size of a small dog.\n\
It buzzes around incessantly.")
);
FLAGS(MF_SMELLS, MF_FLIES, MF_STUMBLES, MF_HIT_AND_RUN);

mon(_("giant bee"),species_insect, 'a',	c_yellow,	MS_SMALL,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 15,-10, 50,140,  4,  1,  1,  5,  6,  0,  5,-50, 20,  0,
	&mdeath::normal,	&mattack::none, _("\
A honey bee the size of a small dog. It\n\
buzzes angrily through the air, dagger-\n\
sized sting pointed forward.")
);
FLAGS(MF_SMELLS, MF_VENOM, MF_FLIES, MF_STUMBLES, MF_HIT_AND_RUN);
ANGER(MTRIG_HURT, MTRIG_FRIEND_DIED, MTRIG_PLAYER_CLOSE);

mon(_("giant wasp"),species_insect, 'a', 	c_red,		MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
     22,  5, 60,150,  6,  1,  3,  7,  7,  0,  7,-40, 35, 0,
	&mdeath::normal,	&mattack::none, _("\
An evil-looking, slender-bodied wasp with\n\
a vicious sting on its abdomen.")
);
FLAGS(MF_SMELLS, MF_POISON, MF_VENOM, MF_FLIES);
ANGER(MTRIG_HURT, MTRIG_FRIEND_DIED, MTRIG_PLAYER_CLOSE, MTRIG_SOUND);

// GIANT WORMS

mon(_("graboid"),	species_worm, 'S',	c_red,		MS_HUGE,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 17, 30,120,180, 11,  3,  8,  4,  0,  5,  5,  0,180,  0,
	&mdeath::worm,		&mattack::none, _("\
A hideous slithering beast with a tri-\n\
sectional mouth that opens to reveal\n\
hundreds of writhing tongues. Most of its\n\
enormous body is hidden underground.")
);
FLAGS(MF_DIGS, MF_HEARS, MF_GOODHEARING, MF_DESTROYS, MF_WARM, MF_LEATHER);

mon(_("giant worm"),species_worm, 'S',	c_pink,		MS_LARGE,	"flesh",
//  dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 10, 30,100, 85,  9,  4,  5,  2,  0,  2,  0,  0, 50,  0,
	&mdeath::worm,		&mattack::none, _("\
Half of this monster is emerging from a\n\
hole in the ground. It looks like a huge\n\
earthworm, but the end has split into a\n\
large, fanged mouth.")
);
FLAGS(MF_DIGS, MF_HEARS, MF_GOODHEARING, MF_WARM, MF_LEATHER);

mon(_("half worm"),species_worm, 's',	c_pink,		MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2, 10, 40, 80,  5,  3,  5,  0,  0,  0,  0,  0, 20,  0,
	&mdeath::normal,	&mattack::none, _("\
A portion of a giant worm that is still alive.")
);
FLAGS(MF_DIGS, MF_HEARS, MF_GOODHEARING, MF_WARM, MF_LEATHER);

// Wild Mutants
mon(_("sludge crawler"),species_none, 'S',	c_dkgray,		MS_LARGE,	"none",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  15, 100, 100, 60,  7,  3,  5,  0,  12,  4,  0,  0, 300,  0,
	&mdeath::melt,	&mattack::none, _("\
A sluglike creature, eight feet long and the width of a refrigerator, it's black \
body glistens as it oozes it's way along the ground. Eye stalks occassionally push their \
way out of the oily mass and look around.")
);
FLAGS(MF_NOHEAD, MF_SEES, MF_POISON, MF_HEARS, MF_REGENERATES_50, MF_SMELLS, MF_VIS30, 
MF_SLUDGEPROOF, MF_SLUDGETRAIL, MF_SWIMS, MF_FLAMMABLE);


// ZOMBIES
mon(_("zombie"),	species_zombie, 'Z',	c_ltgreen,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3,100,100, 70,  8,  1,  5,  2,  1,  0,  0, 40, 50,  5,
	&mdeath::zombie,	&mattack::bite, _("\
A human body, stumbling slowly forward on\n\
uncertain legs, possessed with an unstoppable\n\
rage.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON, MF_BLEED, MF_NO_BREATHE, MF_VIS40);
CATEGORIES(MC_CLASSIC);

mon(_("zombie cop"),	species_zombie, 'Z',	c_blue,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3,100,100, 70,  8,  1,  5,  0,  1,  0,  0, 40, 50,  5,
	&mdeath::zombie,	&mattack::bite, _("\
A human body, encapsulated in tough riot\n\
armour, this zombie was clearly a cop gearing\n\
up to fight the infection.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON, MF_BLEED, MF_NO_BREATHE, MF_VIS30);
CATEGORIES(MC_CLASSIC);

mon(_("shrieker zombie"),species_zombie, 'Z',c_magenta,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	10,100,100, 95,  9,  1,  2,  0,  4,  0,  0, 45, 50, 10,
	&mdeath::zombie,	&mattack::shriek, _("\
This zombie's jaw has been torn off, leaving\n\
a gaping hole from mid-neck up.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON, MF_NO_BREATHE, MF_VIS50);

mon(_("spitter zombie"),species_zombie, 'Z',c_yellow,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	9,100,100,105,  8,  1,  5,  0,  4,  0,  0, 30, 60, 20,
	&mdeath::acid,		&mattack::acid,	_("\
This zombie's mouth is deformed into a round\n\
spitter, and its body throbs with a dense\n\
yellow fluid.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON,
      MF_ACIDPROOF, MF_NO_BREATHE, MF_VIS40);

mon(_("shocker zombie"),species_zombie,'Z',c_ltcyan,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	10,100,100,110,  8,  1,  6,  0,  4,  0,  0, 40, 65, 25,
	&mdeath::zombie,	&mattack::shockstorm, _("\
This zombie's flesh is pale blue, and it\n\
occasionally crackles with small bolts of\n\
lightning.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON,
      MF_ELECTRIC, MF_CBM, MF_NO_BREATHE,  MF_VIS40);

mon(_("smoker zombie"),species_zombie,'Z',c_ltgray,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	10,100,100,110,  8,  1,  6,  0,  4,  0,  0, 0, 65, 1,
	&mdeath::smokeburst,	&mattack::smokecloud, _("\
This zombie emits a constant haze of\n\
thick, obfuscating smoke.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON,
      MF_HARDTOSHOOT, MF_FRIENDLY_SPECIAL, MF_NO_BREATHE, MF_VIS40);

mon(_("zombie dog"),species_zombie, 'd',	c_ltgreen,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	12,100,100,150, 10,  1,  4,  3,  4,  0,  0, 0, 40,  5,
	&mdeath::normal,	&mattack::bite, _("\
This deformed, sinewy canine stays close to\n\
the ground, loping forward faster than most\n\
humans ever could.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON,
      MF_HIT_AND_RUN, MF_NO_BREATHE, MF_VIS50);
CATEGORIES(MC_CLASSIC);

mon(_("zombie brute"),species_zombie, 'Z',	c_red,		MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
    25,100,100,115,  9,  4,  4,  2,  0,  6,  3, 60, 80,  5,
	&mdeath::zombie,	&mattack::bite, _("\
A hideous beast of a zombie, bulging with\n\
distended muscles on both arms and legs.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON, MF_NO_BREATHE, MF_VIS40);

mon(_("zombie hulk"),species_zombie, 'Z',	c_ltred,		MS_HUGE,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
     50,100,100,130,  9,  4,  8,  0,  0, 12,  8, 80,260,  0,
	&mdeath::zombie,	&mattack::none, _("\
A zombie that has somehow grown to the size of\n\
6 men, with arms as wide as a trash can.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES,
      MF_DESTROYS, MF_POISON, MF_ATTACKMON, MF_LEATHER, MF_NO_BREATHE, MF_VIS50);

mon(_("fungal zombie"),species_fungus, 'Z',c_ltgray,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  6,100,100, 45,  6,  1,  6,  0,  0,  0,  0, 20, 40, 50,
	&mdeath::zombie,	&mattack::fungus, _("\
A diseased zombie. Fungus sprouts from its\n\
mouth and eyes, and thick gray mold grows all\n\
over its translucent flesh.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON, MF_NO_BREATHE, MF_VIS30);

mon(_("boomer"),	species_zombie, 'Z',	c_pink,		MS_LARGE,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
    5,100,100, 55,  7,  2,  4,  0,  1,  3,  0, 35, 40,  20,
	&mdeath::boomer,	&mattack::boomer, _("\
A bloated zombie sagging with fat. It emits a\n\
horrible odor, and putrid, pink sludge drips\n\
from its mouth.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON, MF_NO_BREATHE, MF_VIS40);

mon(_("fungal boomer"),species_fungus, 'B',c_ltgray,	MS_LARGE,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
    7,100,100, 40,  5,  2,  6,  0,  0,  3,  0, 20, 20, 30,
	&mdeath::fungus,	&mattack::fungus, _("\
A bloated zombie that is coated with slimy\n\
gray mold. Its flesh is translucent and gray,\n\
and it dribbles a gray sludge from its mouth.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON, MF_NO_BREATHE, MF_VIS30);

mon(_("skeleton"),	species_zombie, 'Z',	c_white,	MS_MEDIUM,	"stone",
//	 dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
      8,100,100, 90, 10,  1,  5,  3,  2,  0, 15,  0, 40, 5,
	&mdeath::zombie,	&mattack::bite, _("\
A skeleton picked clean of all but a few\n\
rotten scraps of flesh, somehow still in\n\
motion.")
);
FLAGS(MF_SEES, MF_HEARS, MF_BLEED, MF_HARDTOSHOOT, MF_NO_BREATHE, MF_VIS30);

mon(_("zombie necromancer"),species_zombie, 'Z',c_dkgray,	MS_MEDIUM,	"flesh",
//	 dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
      13,  0,100,100,  4,  2,  3,  0,  4,  0,  0, 50,140, 4,
	&mdeath::zombie,	&mattack::resurrect, _("\
A zombie with jet black skin and glowing red\n\
eyes.  As you look at it, you're gripped by a\n\
feeling of dread and terror.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON, MF_NO_BREATHE, MF_NO_BREATHE, MF_VIS50);
ANGER(MTRIG_HURT, MTRIG_PLAYER_WEAK);

mon(_("zombie scientist"),species_zombie, 'Z',c_ltgray,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
      3,100,100, 75,  7,  1,  3,  0,  1,  0,  0, 50, 35, 20,
	&mdeath::zombie,	&mattack::science, _("\
A zombie wearing a tattered lab coat and\n\
some sort of utility belt.  It looks weaker\n\
than most zombies, but more resourceful too.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON, MF_BLEED,
      MF_ACIDPROOF, MF_NO_BREATHE, MF_VIS50);

mon(_("zombie soldier"),	species_zombie,	'Z',c_ltblue,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
     20,100,100, 80, 12,  2,  4,  0,  0,  8, 16, 60,100, 5,
	&mdeath::zombie,	&mattack::bite, _("\
This zombie was clearly a soldier before.\n\
Its tattered armor gives it strong defense,\n\
and it is much more physically fit than\n\
most zombies.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_WARM, MF_BASHES, MF_POISON, MF_BLEED, MF_NO_BREATHE, MF_VIS30);
CATEGORIES(MC_CLASSIC);

mon(_("grabber zombie"),	species_zombie,	'Z',c_green,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
    8,100,100, 70, 10,  1,  2,  0,  4,  0,  0, 40, 65, 0,
	&mdeath::zombie,	&mattack::none, _("\
This zombie seems to have slightly longer\n\
than ordinary arms, and constantly gropes\n\
at its surroundings as it moves.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON, MF_BLEED,
      MF_GRABS, MF_NO_BREATHE, MF_VIS30);

mon(_("master zombie"),	species_zombie, 'M',c_yellow,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	16,  5,100, 90,  4,  1,  3,  0,  4,  0,  0, 60,120, 3,
	&mdeath::zombie,	&mattack::upgrade, _("\
This zombie seems to have a cloud of black\n\
dust surrounding it.  It also seems to have\n\
a better grasp of movement than most...")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_WARM, MF_BASHES, MF_POISON, MF_NO_BREATHE, MF_VIS50);
ANGER(MTRIG_HURT, MTRIG_PLAYER_WEAK);

mon(_("scarred zombie"),	species_zombie, 'Z',	c_yellow,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 3,100,100, 40,  8,  1,  5,  0,  1, 12, 8, 20, 25,  0,
	&mdeath::zombie,	&mattack::none, _("\
Hundreds of bee stings have given this zombie\n\
a thick covering of scar tissue, it will be\n\
much harder to damage than an ordinary zombie\n\
but moves a bit slower")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON, MF_NO_BREATHE, MF_VIS30);

 mon(_("child zombie"),species_zombie, 'z',	c_ltgreen,	MS_SMALL,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2, 65,100,  70, 8,  1,  3,  2,  2,  0,  0, 20, 25,  0,
	&mdeath::zombie,	&mattack::none, _("\
A horrifying child zombie, you feel\n\
a twinge of remorse looking at it.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_BLEED,
      MF_POISON, MF_GUILT, MF_NO_BREATHE, MF_VIS30);
CATEGORIES(MC_CLASSIC);

//Somewhere between a zombie and a blob creature
mon(_("jabberwock"),	species_none, 'J',	c_dkgray_red,	MS_HUGE,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
     50,100,100,140,  9,  4,  8,  3,  3, 12,  8,  0,400,  0,
	&mdeath::normal,	&mattack::flesh_golem, _("\
An amalgamation of putrid human and animal\n\
parts that have become fused in this golem\n\
of flesh.  The eyes of all the heads dart\n\
rapidly and the mouths scream or groan.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_DESTROYS,
      MF_ATTACKMON, MF_LEATHER, MF_BONES, MF_VIS50, MF_POISON);

// PLANTS & FUNGI
mon(_("triffid"),	species_plant, 'F',	c_ltgreen,	MS_MEDIUM,	"veggy",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
 	 16, 20,100, 75,  9,  2,  4,  5,  0, 10,  2,  0, 80,  0,
	&mdeath::normal,	&mattack::none, _("\
A plant that grows as high as your head,\n\
with one thick, bark-coated stalk\n\
supporting a flower-like head with a sharp\n\
sting within.")
);
FLAGS(MF_HEARS, MF_SMELLS, MF_BASHES, MF_NOHEAD);

mon(_("young triffid"),species_plant, 'f',	c_ltgreen,	MS_SMALL,	"veggy",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2,  0, 10, 65,  7,  1,  4,  3,  0,  0,  0,  0, 40,  0,
	&mdeath::normal,	&mattack::none, _("\
A small triffid, only a few feet tall. It\n\
has not yet developed bark, but its sting\n\
is still sharp and deadly.")
);
FLAGS(MF_HEARS, MF_SMELLS, MF_NOHEAD);

mon(_("queen triffid"),species_plant, 'F',	c_red,		MS_LARGE,	"veggy",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 28,100,200, 85, 14,  2,  7,  8,  0, 10,  4,  0,280, 2,
	&mdeath::normal,	&mattack::growplants, _("\
A very large triffid, with a particularly\n\
vicious sting and thick bark.  As it\n\
moves, plant matter drops off its body\n\
and immediately takes root.")
);
FLAGS(MF_HEARS, MF_SMELLS, MF_BASHES, MF_NOHEAD);

mon(_("creeper hub"),species_plant, 'V',	c_dkgray,	MS_MEDIUM,	"veggy",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 16,100,100,100,  0,  0,  0,  0,  0,  8,  0,  0,100, 2,
	&mdeath::kill_vines,	&mattack::grow_vine, _("\
A thick stalk, rooted to the ground.\n\
It rapidly sprouts thorny vines in all\n\
directions.")
);
FLAGS(MF_NOHEAD, MF_IMMOBILE);

mon(_("creeping vine"),species_plant, 'v',	c_green,	MS_TINY,	"veggy",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  4,100,100, 75,  0,  0,  0,  0,  0,  2,  0,  0,  2, 2,
	&mdeath::vine_cut,	&mattack::vine, _("\
A thorny vine.  It twists wildly as\n\
it grows, spreading rapidly.")
);
FLAGS(MF_NOHEAD, MF_HARDTOSHOOT, MF_PLASTIC, MF_IMMOBILE);

mon(_("biollante"),species_plant, 'F',	c_magenta,	MS_LARGE,	"veggy",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 20,100,100,100,  0,  0,  0,  0,  0,  0,  0,-80,120, 2,
	&mdeath::normal,	&mattack::spit_sap, _("\
A thick stalk topped with a purple\n\
flower.  The flower's petals are closed,\n\
and pulsate ominously.")
);
FLAGS(MF_NOHEAD, MF_IMMOBILE);

mon(_("vinebeast"),species_plant, 'V',	c_red,		MS_LARGE,	"veggy",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 14, 60, 40, 75, 15,  2,  4,  2,  4, 10,  0,  0,100, 0,
	&mdeath::normal,	&mattack::none,	_("\
This appears to be a mass of vines, moving\n\
with surprising speed.  It is so thick and\n\
tangled that you cannot see what lies in\n\
the middle.")
);
FLAGS(MF_HEARS, MF_GOODHEARING, MF_NOHEAD, MF_HARDTOSHOOT, MF_GRABS,
      MF_SWIMS, MF_PLASTIC);

mon(_("triffid heart"),species_plant, 'T',	c_red,		MS_HUGE,	"veggy",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
     45,100,100,100,  0,  0,  0,  0,  0, 14,  4,  0,300, 5,
	&mdeath::triffid_heart,	&mattack::triffid_heartbeat, _("\
A knot of roots that looks bizarrely like a\n\
heart.  It beats slowly with sap, powering\n\
the root walls around it.")
);
FLAGS(MF_NOHEAD, MF_IMMOBILE, MF_QUEEN);

mon(_("fungaloid"),species_fungus, 'F',	c_ltgray,	MS_MEDIUM,	"veggy",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 12,  0,100, 45,  8,  3,  3,  0,  0,  4,  0,  0, 80, 200,
	&mdeath::fungus,	&mattack::fungus, _("\
A pale white fungus, one meaty gray stalk\n\
supporting a bloom at the top. A few\n\
tendrils extend from the base, allowing\n\
mobility and a weak attack.")
);
ANGER(MTRIG_PLAYER_CLOSE, MTRIG_PLAYER_WEAK);
FLAGS(MF_HEARS, MF_SMELLS, MF_POISON, MF_NO_BREATHE, MF_NOHEAD);

// This is a "dormant" fungaloid that doesn't waste CPU cycles ;)
mon(_("fungaloid"),species_fungus, 'F',	c_ltgray,	MS_MEDIUM,	"veggy",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  0,100,  1,  8,  2,  4,  0,  0,  4,  0,  0,  1, 0,
	&mdeath::fungusawake,	&mattack::none, _("\
A pale white fungus, one meaty gray stalk\n\
supporting a bloom at the top. A few\n\
tendrils extend from the base, allowing\n\
mobility and a weak attack.")
);
FLAGS(MF_HEARS, MF_SMELLS, MF_POISON, MF_NO_BREATHE, MF_NOHEAD);

mon(_("young fungaloid"),species_fungus, 'f',c_ltgray,	MS_SMALL,	"veggy",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  6,100,100, 65,  8,  1,  4,  6,  0,  4,  4,  0, 70, 0,
	&mdeath::normal,	&mattack::none, _("\
A fungal tendril just a couple feet tall.  Its\n\
exterior is hardened into a leathery bark and\n\
covered in thorns; it also moves faster than\n\
full-grown fungaloids.")
);
FLAGS(MF_HEARS, MF_SMELLS, MF_POISON, MF_NO_BREATHE, MF_NOHEAD);

mon(_("spore"),	species_fungus, 'o',	c_ltgray,	MS_TINY,	"veggy",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1,-50,100,100,  0,  0,  0,  0,  6,  0,  0,  0,  5, 50,
	&mdeath::disintegrate,	&mattack::plant, _("\
A wispy spore, about the size of a fist,\n\
wafting on the breeze.")
);
FLAGS(MF_STUMBLES, MF_FLIES, MF_POISON, MF_NO_BREATHE, MF_NOHEAD);

mon(_("fungal spire"),species_fungus, 'T',	c_ltgray,	MS_HUGE,	"stone",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 40,100,100,100,  0,  0,  0,  0,  0, 10, 10,  0,300, 5,
	&mdeath::fungus,	&mattack::fungus_sprout, _("\
An enormous fungal spire, towering 30 feet\n\
above the ground.  It pulsates slowly,\n\
continuously growing new defenses.")
);
FLAGS(MF_NOHEAD, MF_POISON, MF_IMMOBILE, MF_NO_BREATHE, MF_QUEEN);

mon(_("fungal wall"),species_fungus, 'F',	c_dkgray,	MS_HUGE,	"veggy",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  5,100,100,100,  0,  0,  0,  0,  0, 10, 10,  0, 10, 8,
	&mdeath::disintegrate,	&mattack::fungus,_( "\
A veritable wall of fungus, grown as a\n\
natural defense by the fungal spire. It\n\
looks very tough, and spews spores at an\n\
alarming rate.")
);
FLAGS(MF_NOHEAD, MF_POISON, MF_NO_BREATHE, MF_IMMOBILE);

// BLOBS & SLIMES &c
mon(_("blob"),	species_nether, 'O',	c_dkgray,	MS_MEDIUM,	"null",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 19,100,100, 85,  9,  2,  4,  0,  0,  6,  0,  0, 85, 30,
	&mdeath::blobsplit,	&mattack::formblob, _("\
A black blob of viscous goo that oozes\n\
across the ground like a mass of living\n\
oil.")
);
FLAGS(MF_SMELLS, MF_HEARS, MF_GOODHEARING, MF_NOHEAD, MF_POISON, MF_NO_BREATHE, MF_ACIDPROOF);
mtypes[mon_blob]->phase = LIQUID;

mon(_("small blob"),species_nether, 'o',	c_dkgray,	MS_SMALL,	"null",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2,100,100, 50,  6,  1,  4,  0,  0,  2,  0,  0, 50, 0,
	&mdeath::blobsplit,	&mattack::none, "\
A small blob of viscous goo that oozes\n\
across the ground like a mass of living\n\
oil."
);
FLAGS(MF_SMELLS, MF_HEARS, MF_GOODHEARING, MF_NOHEAD, MF_POISON, MF_NO_BREATHE, MF_ACIDPROOF);
mtypes[mon_blob_small]->phase = LIQUID;

// CHUDS & SUBWAY DWELLERS
mon(_("C.H.U.D."),	species_none, 'S',	c_ltgray,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  8, 30, 30,110, 10,  1,  5,  0,  3,  0,  0, 25, 60, 0,
	&mdeath::normal,	&mattack::none,	_("\
Cannibalistic Humanoid Underground Dweller.\n\
A human, turned pale and mad from years in\n\
the subways.")
);
FLAGS(MF_SEES, MF_HEARS, MF_WARM, MF_BASHES, MF_HUMAN, MF_VIS40, MF_BONES);
FEARS(MTRIG_HURT, MTRIG_FIRE);

mon(_("one-eyed mutant"),species_none, 'S',c_ltred,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 18, 30, 30,130, 20,  2,  4,  0,  5,  0,  0, 40, 80, 0,
	&mdeath::normal,	&mattack::none,	_("\
A relatively humanoid mutant with purple\n\
hair and a grapefruit-sized bloodshot eye.")
);
FLAGS(MF_SEES, MF_HEARS, MF_WARM, MF_BASHES, MF_HUMAN, MF_BONES);
FEARS(MTRIG_HURT, MTRIG_FIRE);

mon(_("crawler mutant"),species_none, 'S',	c_red,		MS_LARGE,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 16, 40, 40, 80, 10,  2,  6,  0,  1,  8,  0,  0,180, 0,
	&mdeath::normal,	&mattack::none,	_("\
Two or three humans fused together somehow,\n\
slowly dragging their thick-hided, hideous\n\
body across the ground.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_WARM, MF_BASHES, MF_POISON, MF_HUMAN, MF_VIS40, MF_BONES);
FEARS(MTRIG_HURT, MTRIG_FIRE);

mon(_("sewer fish"),species_none, 's',	c_ltgreen,	MS_SMALL,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 13,100,100,120, 17,  1,  3,  3,  6,  0,  0,  0, 20, 0,
	&mdeath::normal,	&mattack::none,	_("\
A large green fish, it's mouth lined with\n\
three rows of razor-sharp teeth.")
);
FLAGS(MF_SEES, MF_SMELLS, MF_WARM, MF_AQUATIC, MF_VIS30, MF_BONES);
FEARS(MTRIG_HURT);

mon(_("sewer snake"),species_none, 's',	c_yellow,	MS_SMALL,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2, 20, 40, 60, 12,  1,  2,  5,  1,  0,  0,  0, 10, 0,
	&mdeath::normal,	&mattack::none,	_("\
A large snake, turned pale yellow from its\n\
underground life.")
);
FLAGS(MF_SEES, MF_SMELLS, MF_WARM, MF_VENOM, MF_SWIMS, MF_LEATHER, MF_VIS30, MF_BONES);
FEARS(MTRIG_HURT);

mon(_("sewer rat"),species_mammal, 's',	c_dkgray,	MS_SMALL,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3, 20, 40,105, 10,  1,  2,  1,  2,  0,  0,  0, 10, 0,
	&mdeath::normal,	&mattack::none, _("\
A large, mangey rat with red eyes.  It\n\
scampers quickly across the ground, squeaking\n\
hungrily.")
);
FLAGS(MF_SEES, MF_SMELLS, MF_HEARS, MF_WARM, MF_SWIMS, MF_ANIMAL, MF_FUR, MF_VIS40, MF_BONES);

mon(_("rat king"),species_mammal, 'S',	c_dkgray,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 18, 10,100, 40,  4,  1,  3,  1,  0,  0,  0,  0,220, 3,
	&mdeath::ratking,	&mattack::ratking, _("\
A group of several rats, their tails\n\
knotted together in a filthy mass.  A wave\n\
of nausea washes over you in its presence.")
);

// SWAMP CREATURES
mon(_("giant mosquito"),species_insect, 'y',c_ltgray,	MS_SMALL,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 12, 20, 20,120,  8,  1,  1,  1,  5,  0,  0,  0, 20, 0,
	&mdeath::normal,	&mattack::none, _("\
An enormous mosquito, fluttering erratically,\n\
its face dominated by a long, spear-tipped\n\
proboscis.")
);
FLAGS(MF_SMELLS, MF_HEARS, MF_STUMBLES, MF_VENOM, MF_FLIES, MF_HIT_AND_RUN);

mon(_("giant dragonfly"),species_insect, 'y',c_ltgreen,	MS_SMALL,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 13, 20,100,155, 12,  1,  3,  6,  5,  0,  6,-20, 70, 0,
	&mdeath::normal,	&mattack::none, _("\
A ferocious airborne predator, flying swiftly\n\
through the air, its mouth a cluster of fangs.")
);
FLAGS(MF_SEES, MF_SMELLS, MF_FLIES, MF_HIT_AND_RUN, MF_VIS40);

mon(_("giant centipede"),species_insect, 'a',c_ltgreen,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  9, 20,100,120, 10,  1,  3,  5,  2,  0,  8,-30, 60, 0,
	&mdeath::normal,	&mattack::none, _("\
A meter-long centipede, moving swiftly on\n\
dozens of thin legs, a pair of venomous\n\
pincers attached to its head.")
);
FLAGS(MF_SMELLS, MF_HEARS, MF_POISON, MF_VENOM);

mon(_("giant frog"),species_none, 'F',	c_green,	MS_LARGE,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 10, 10,100, 90,  8,  2,  3,  0,  2,  4,  0,  0, 70, 5,
	&mdeath::normal,	&mattack::leap, _("\
A thick-skinned green frog.  It eyes you\n\
much as you imagine it might eye an insect.")
);
FLAGS(MF_SEES, MF_SMELLS, MF_HEARS, MF_SWIMS, MF_LEATHER, MF_VIS40, MF_BONES);
FEARS(MTRIG_HURT);

mon(_("giant slug"),species_none, 'S',	c_yellow,	MS_HUGE,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 16, 20,100, 60,  7,  1,  5,  1,  0,  8,  2,  0,190, 10,
	&mdeath::normal,	&mattack::acid, _("\
A gigantic slug, the size of a small car.\n\
It moves slowly, dribbling acidic goo from\n\
its fang-lined mouth.")
);
FLAGS(MF_SEES, MF_SMELLS, MF_BASHES, MF_ACIDPROOF, MF_ACIDTRAIL, MF_VIS30);
FEARS(MTRIG_HURT, MTRIG_PLAYER_CLOSE);

mon(_("dermatik larva"),species_insect, 'i',c_white,	MS_TINY,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  4,-20,-20, 20,  1,  1,  2,  2,  1,  0,  0,  0, 10, 0,
	&mdeath::normal,	&mattack::none, _("\
A fat, white grub the size of your foot, with\n\
a set of mandibles that look more suited for\n\
digging than fighting.")
);
FLAGS(MF_HEARS, MF_SMELLS, MF_POISON, MF_DIGS);

mon(_("dermatik"),	species_insect, 'i',	c_red,		MS_TINY,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 18, 30, 30,100,  5,  1,  1,  6, 7,   0,  6,  0, 60, 50,
	&mdeath::normal,	&mattack::dermatik, _("\
A wasp-like flying insect, smaller than most\n\
mutated wasps.  It does not looke very\n\
threatening, but has a large ovipositor in\n\
place of a sting.")
);
FLAGS(MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_POISON, MF_FLIES);

// SPIDERS
mon(_("wolf spider"),species_insect, 's',	c_brown,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 20, 20,100,110, 7,   1,  1,  8,  6,  2,  8,-70, 40, 0,
	&mdeath::normal,	&mattack::none, _("\
A large, brown spider, which moves quickly\n\
and aggresively.")
);
FLAGS(MF_SMELLS, MF_HEARS, MF_VENOM);

mon(_("web spider"),species_insect, 's',	c_yellow,	MS_SMALL,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 16, 30, 80,120,  5,  1,  1,  7,  5,  2,  7,-70, 35, 0,
	&mdeath::normal,	&mattack::none, _("\
A yellow spider the size of a dog.  It lives\n\
in webs, waiting for prey to become\n\
entangled before pouncing and biting.")
);
FLAGS(MF_SMELLS, MF_HEARS, MF_VENOM, MF_WEBWALK);

mon(_("jumping spider"),species_insect, 's',c_white,	MS_SMALL,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 14, 40, 80,100,  7,  1,  1,  4,  8,  0,  3,-60, 30, 2,
	&mdeath::normal,	&mattack::leap, _("\
A small, almost cute-looking spider.  It\n\
leaps so quickly that it almost appears to\n\
instantaneously move from one place to\n\
another.")
);
FLAGS(MF_SMELLS, MF_HEARS, MF_VENOM, MF_HIT_AND_RUN);

mon(_("trap door spider"),species_insect, 's',c_blue,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 20, 60,100,110,  5,  1,  2,  7,  3,  2,  8,-80, 70, 0,
	&mdeath::normal,	&mattack::none, _("\
A large spider with a bulbous thorax.  It\n\
creates a subterranean nest and lies in\n\
wait for prey to fall in and become trapped\n\
in its webs.")
);
FLAGS(MF_SMELLS, MF_HEARS, MF_VENOM, MF_WEBWALK);

mon(_("black widow"),species_insect, 's',	c_dkgray,	MS_SMALL,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 20,-10,100, 90,  6,  1,  1,  6,  3,  0,  3,-50, 40, 0,
	&mdeath::normal,	&mattack::none, _("\
A spider with a characteristic red\n\
hourglass on its black carapace.  It is\n\
known for its highly toxic venom.")
);
FLAGS(MF_SMELLS, MF_HEARS, MF_BADVENOM, MF_WEBWALK);
ANGER(MTRIG_PLAYER_WEAK, MTRIG_PLAYER_CLOSE, MTRIG_HURT);

// UNEARTHED HORRORS
mon(_("dark wyrm"),species_none, 'S',	c_blue,		MS_LARGE,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 20,100,100,100,  8,  2,  6,  4,  4,  6,  0,  0,120, 0,
	&mdeath::normal,	&mattack::none, _("\
A huge, black worm, its flesh glistening\n\
with an acidic, blue slime.  It has a gaping\n\
round mouth lined with dagger-like teeth.")
);
FLAGS(MF_SMELLS, MF_HEARS, MF_GOODHEARING, MF_DESTROYS, MF_POISON, MF_SUNDEATH,
      MF_ACIDPROOF, MF_ACIDTRAIL);

mon(_("Amigara horror"),species_none, 'H',	c_white,	MS_LARGE,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 30,100,100, 70, 10,  2,  4,  0,  2,  0,  0,  0,250, 0,
	&mdeath::amigara,	&mattack::fear_paralyze, _("\
A spindly body, standing at least 15 feet\n\
tall.  It looks vaguely human, but its face is\n\
grotesquely stretched out, and its limbs are\n\
distorted to the point of being tentacles.")
);
FLAGS(MF_SMELLS, MF_HEARS, MF_SEES, MF_STUMBLES, MF_HARDTOSHOOT);

// This "dog" is, in fact, a disguised Thing
mon(_("dog"),	species_nether, 'd',	c_white,	MS_SMALL,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  5,100,100,150, 12,  2,  3,  3,  3,  0,  0,  0, 25, 40,
	&mdeath::thing,		&mattack::dogthing, _("\
A medium-sized domesticated dog, gone feral.")
);
FLAGS(MF_SEES, MF_SMELLS, MF_HEARS, MF_ANIMAL, MF_WARM, MF_FUR,
      MF_FRIENDLY_SPECIAL);

mon(_("tentacle dog"),species_nether, 'd',	c_dkgray,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 14,100,100,120, 12,  2,  4,  0,  3,  0,  0,  0,120, 5,
	&mdeath::thing,		&mattack::tentacle, _("\
A dog's body with a mass of ropy, black\n\
tentacles extending from its head.")
);
FLAGS(MF_SEES, MF_SMELLS, MF_HEARS, MF_BASHES);

mon(_("Thing"),	species_nether, 'H',	c_dkgray,	MS_LARGE,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 25,100,100,135, 14,  2,  4,  0,  5,  8,  0,  0,160, 5,
	&mdeath::melt,		&mattack::tentacle, _("\
An amorphous black creature which seems to\n\
sprout tentacles rapidly.")
);
FLAGS(MF_SMELLS, MF_HEARS, MF_NOHEAD, MF_BASHES, MF_SWIMS, MF_ATTACKMON,
      MF_PLASTIC, MF_ACIDPROOF);

mon(_("human snail"),species_none, 'h',	c_green,	MS_LARGE,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 10,  0, 30, 50,  4,  1,  5,  0,  0,  6, 12,  0, 50, 15,
	&mdeath::normal,	&mattack::acid, _("\
A large snail, with an oddly human face.")
);
FLAGS(MF_SMELLS, MF_HEARS, MF_POISON, MF_ACIDPROOF, MF_ACIDTRAIL);
ANGER(MTRIG_PLAYER_WEAK, MTRIG_FRIEND_DIED);
FEARS(MTRIG_PLAYER_CLOSE, MTRIG_HURT);

mon(_("twisted body"),species_none, 'h',	c_pink,		MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 12,100,100, 90,  5,  2,  4,  0,  6,  0,  0,  0, 65, 0,
	&mdeath::normal,	&mattack::none, _("\
A human body, but with its limbs, neck, and\n\
hair impossibly twisted.")
);
FLAGS(MF_SEES, MF_HEARS, MF_GOODHEARING, MF_POISON, MF_HUMAN, MF_VIS40);

mon(_("vortex"),	species_none, 'v',	c_white,	MS_SMALL,	"powder",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 30,100,100,120,  0,  0,  0,  0,  0,  0,  0,  0, 20, 6,
	&mdeath::melt,		&mattack::vortex, _("\
A twisting spot in the air, with some kind\n\
of morphing mass at its center.")
);
FLAGS(MF_HEARS, MF_GOODHEARING, MF_STUMBLES, MF_NOHEAD, MF_HARDTOSHOOT,
      MF_FLIES, MF_PLASTIC, MF_NO_BREATHE, MF_FRIENDLY_SPECIAL);

// NETHER WORLD INHABITANTS
mon(_("flying polyp"),species_nether, 'H',	c_dkgray,	MS_HUGE,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 42,100,100,280, 16,  3,  8,  6,  7,  8,  0,  0,350, 0,
	&mdeath::melt,		&mattack::none, _("\
An amorphous mass of twisting black flesh\n\
that flies through the air swiftly.")
);
FLAGS(MF_SMELLS, MF_HEARS, MF_GOODHEARING, MF_NOHEAD, MF_BASHES, MF_FLIES,
      MF_ATTACKMON, MF_PLASTIC, MF_NO_BREATHE, MF_HIT_AND_RUN);

mon(_("hunting horror"),species_nether, 'h',c_dkgray,	MS_SMALL,	"null",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
     28,100,100,180, 15,  3,  4,  0,  6,  0,  0,  0, 80, 0,
	&mdeath::melt,		&mattack::none, _("\
A ropy, worm-like creature that flies on\n\
bat-like wings. Its form continually\n\
shifts and changes, twitching and\n\
writhing.")
);
FLAGS(MF_SEES, MF_SMELLS, MF_HEARS, MF_NOHEAD, MF_HARDTOSHOOT, MF_FLIES,
      MF_PLASTIC, MF_SUNDEATH, MF_NO_BREATHE, MF_HIT_AND_RUN);

mon(_("Mi-go"),	species_nether, 'H',	c_pink,		MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
     26, 20, 30,120, 14,  5,  3, 10,  7,  4, 12,  0,110, 0,
	&mdeath::normal,	&mattack::parrot, _("\
A pinkish, fungoid crustacean-like\n\
creature with numerous pairs of clawed\n\
appendages and a head covered with waving\n\
antennae.")
);
FLAGS(MF_SEES, MF_SMELLS, MF_HEARS, MF_WARM, MF_BASHES, MF_POISON, MF_NO_BREATHE, MF_VIS50);

mon(_("yugg"),	species_nether, 'H',	c_white,	MS_HUGE,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 32,100,100, 80, 12,  3,  5,  8,  1,  6,  0,  0,320, 20,
	&mdeath::normal,	&mattack::gene_sting, _("\
An enormous white flatworm writhing\n\
beneath the earth. Poking from the\n\
ground is a bulbous head dominated by a\n\
pink mouth, lined with rows of fangs.")
);
FLAGS(MF_SEES, MF_SMELLS, MF_HEARS, MF_BASHES, MF_DESTROYS, MF_POISON, MF_VENOM, MF_NO_BREATHE,
      MF_DIGS, MF_VIS40);

mon(_("gelatinous blob"),species_nether, 'O',c_ltgray,	MS_LARGE,	"null",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 20,  0,100, 40,  8,  2,  3,  0,  0, 10,  0,  0,200, 4,
	&mdeath::melt,		&mattack::formblob, _("\
A shapeless blob the size of a cow.  It\n\
oozes slowly across the ground, small\n\
chunks falling off of its sides.")
);
FLAGS(MF_SMELLS, MF_HEARS, MF_PLASTIC, MF_NO_BREATHE, MF_NOHEAD);
mtypes[mon_gelatin]->phase = LIQUID;

mon(_("flaming eye"),species_nether, 'E',	c_red,		MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 27,  0,100, 90,  0,  0,  0,  0,  1,  3,  0,  0,300, 12,
	&mdeath::normal,	&mattack::stare, _("\
An eyeball the size of an easy chair and\n\
covered in rolling blue flames. It floats\n\
through the air.")
);
FLAGS(MF_SEES, MF_WARM, MF_FLIES, MF_FIREY, MF_NO_BREATHE, MF_NOHEAD);

mon(_("kreck"),	species_nether, 'h',	c_ltred,	MS_SMALL,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  6,100,100,105,  6,  1,  3,  1,  5,  0,  5,  0, 35, 0,
	&mdeath::melt,		&mattack::none, _("\
A small humanoid, the size of a dog, with\n\
twisted red flesh and a distended neck. It\n\
scampers across the ground, panting and\n\
grunting.")
);
FLAGS(MF_SEES, MF_SMELLS, MF_HEARS, MF_WARM, MF_BASHES, MF_HIT_AND_RUN, MF_NO_BREATHE, MF_VIS50, MF_BONES);

mon(_("gracken"),species_nether, 'G',      c_white,        MS_MEDIUM,      "flesh",
//      dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
          5,  0,100, 180,  9,  1,  4,  0,  1,  0,  0,  0,100, 10,
        &mdeath::normal,        &mattack::none, _("\
An eldritch creature, shuffling\n\
along, its hands twitching so\n\
fast they appear as nothing but\n\
blurs.")
);
FLAGS(MF_SMELLS, MF_HEARS, MF_WARM, MF_NO_BREATHE, MF_ANIMAL);


mon(_("blank body"),species_nether, 'h',	c_white,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 5,  0,100, 80,  9,  1,  4,  0,  1,  0,  0,  0,100, 10,
	&mdeath::normal,	&mattack::shriek, _("\
This looks like a human body, but its\n\
flesh is snow-white and its face has no\n\
features save for a perfectly round\n\
mouth.")
);
FLAGS(MF_SMELLS, MF_HEARS, MF_WARM, MF_ANIMAL, MF_SUNDEATH, MF_NO_BREATHE, MF_HUMAN, MF_BONES);

mon(_("Gozu"),	species_nether, 'G',	c_white,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 20, 10,100, 80, 12,  2,  5,  0,  5,  0,  0,  0,400, 20,
	&mdeath::normal,	&mattack::fear_paralyze, _("\
A beast with the body of a slightly-overweight\n\
man and the head of a cow.  It walks slowly,\n\
milky white drool dripping from its mouth,\n\
wearing only a pair of white underwear.")
);
FLAGS(MF_SEES, MF_SMELLS, MF_HEARS, MF_WARM, MF_BASHES, MF_ANIMAL, MF_FUR, MF_NO_BREATHE, MF_VIS30, MF_BONES);

mon(_("shadow"),	species_nether,'S',	c_dkgray,	MS_TINY,	"null",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 14,100,100, 90,  6,  1,  2,  0,  4,  0,  0,  0, 60, 20,
	&mdeath::melt,		&mattack::disappear, _("\
A strange dark area in the area.  It whispers\n\
softly as it moves.")
);
FLAGS(MF_SEES, MF_HEARS, MF_GOODHEARING, MF_SMELLS, MF_NOHEAD, MF_HARDTOSHOOT,
      MF_GRABS, MF_WEBWALK, MF_FLIES, MF_PLASTIC, MF_SUNDEATH, MF_ELECTRIC,
      MF_ACIDPROOF, MF_HIT_AND_RUN, MF_NO_BREATHE, MF_VIS50);

// The hub
mon(_("breather"),	species_nether,'O',	c_pink,		MS_MEDIUM,	"null",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2,  0,  0,100,  0,  0,  0,  0,  0,  0,  0,  0,100, 6,
	&mdeath::kill_breathers,&mattack::breathe, _("\
A strange, immobile pink goo.  It seems to\n\
be breathing slowly.")
);
FLAGS(MF_IMMOBILE);

// The branches
mon(_("breather"),	species_nether,'o',	c_pink,		MS_MEDIUM,	"null",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2,  0,  0,100,  0,  0,  0,  0,  0,  0,  0,  0,100, 6,
	&mdeath::melt,		&mattack::breathe, _("\
A strange, immobile pink goo.  It seems to\n\
be breathing slowly.")
);
FLAGS(MF_IMMOBILE);

mon(_("shadow snake"),species_none, 's',	c_dkgray,	MS_SMALL,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  6,100,100, 90, 12,  1,  4,  5,  1,  0,  0,  0, 40, 20,
	&mdeath::melt,		&mattack::disappear, _("\
A large snake, translucent black."));
FLAGS(MF_SEES, MF_SMELLS, MF_WARM, MF_SWIMS, MF_LEATHER, MF_PLASTIC,
      MF_SUNDEATH);

// CULT

mon(_("dementia"),    species_zombie, 'd',    c_red,    MS_MEDIUM,    "flesh",
//    dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
      3,100,100, 105,  5,  1,  5,  2,  1,  0,  0, 0, 80,  0,
    &mdeath::zombie,    &mattack::none, _("\
An insane individual with many bloody holes\n\
on the sides of their shaved head.  Some form\n\
of lobotomy has left it with a partially re-\n\
animated brain.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_WARM, MF_BASHES, MF_BLEED, MF_HUMAN, MF_POISON);

mon(_("homunculus"),    species_zombie, 'h',    c_white,    MS_MEDIUM,    "flesh",
//    dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
      3,100,100, 110,  8,  1,  5,  2,  4,  3,  3, 0, 110,  0,
    &mdeath::zombie,    &mattack::none, _("\
A pale white man with a physically flawless athletic\n\
body and shaved head.  His eyes are completely black\n\
as bloody tears pour forth from them.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_WARM, MF_BASHES, MF_BLEED, MF_HUMAN);

mon(_("blood sacrifice"),    species_zombie, 'S',    c_red,    MS_MEDIUM,    "flesh",
//    dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
      0,100,100, 200,  0,  1,  5,  2,  0,  0,  0, 0, 40,  0,
    &mdeath::zombie,    &mattack::fear_paralyze, _("\
This poor victim was sliced open and bled to\n\
death long ago.  Yet, chained down it thrashes\n\
in eternal misery from its tortures.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_WARM, MF_BLEED, MF_IMMOBILE, MF_GUILT, MF_POISON);

mon(_("flesh angel"),    species_zombie, 'A',    c_red,    MS_LARGE,    "flesh",
//    dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
      20,100,100, 120, 10,  3,  4,  0,  2,  0,  0,  0, 200, 0,
    &mdeath::zombie,    &mattack::fear_paralyze, _("\
Slender and terrifying, this gigantic man lacks\n\
any skin yet moves swiftly and gracefully without\n\
it.  Wings of flesh protrude uselessly from his\n\
back and a third eye dominates his forehead.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_WARM, MF_BLEED, MF_HARDTOSHOOT, MF_HUMAN, MF_POISON);

// ROBOTS
mon(_("eyebot"),	species_robot, 'r',	c_ltblue,	MS_SMALL,	"steel",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2,  0,100,120, 0,  0,  0,  0,  3,  10, 10, 70,  20, 30,
	&mdeath::normal,	&mattack::photograph, _("\
A roughly spherical robot that hovers about\n\
five feet of the ground.  Its front side is\n\
dominated by a huge eye and a flash bulb.\n\
Frequently used for reconaissance.")
);
FLAGS(MF_SEES, MF_FLIES, MF_ELECTRONIC, MF_NO_BREATHE, MF_NOHEAD);

mon(_("manhack"),	species_robot, 'r',	c_green,	MS_TINY,	"steel",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  7,100,100,130, 12,  1,  1,  8,  4,  0,  6, 10, 15, 0,
	&mdeath::normal,	&mattack::none, _("\
A fist-sized robot that flies swiftly through\n\
the air.  It's covered with whirring blades\n\
and has one small, glowing red eye.")
);
FLAGS(MF_SEES, MF_FLIES, MF_NOHEAD, MF_ELECTRONIC, MF_HIT_AND_RUN, MF_NO_BREATHE, MF_VIS40);

mon(_("skitterbot"),species_robot, 'r',	c_ltred,	MS_SMALL,	"steel",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 13,100,100,105,  0,  0,  0,  0,  0, 12, 12, 60, 40, 5,
	&mdeath::normal,	&mattack::tazer, _("\
A robot with an insectoid design, about\n\
the size of a small dog.  It skitters\n\
quickly across the ground, two electric\n\
prods at the ready.")
);
FLAGS(MF_SEES, MF_HEARS, MF_GOODHEARING, MF_ELECTRONIC, MF_NO_BREATHE, MF_VIS50);

mon(_("secubot"),	species_robot, 'R',	c_dkgray,	MS_SMALL,	"steel",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 19,100,100, 70,  0,  0,  0,  0,  0, 14, 14, 80, 80, 2,
	&mdeath::explode,	&mattack::smg, _("\
A boxy robot about four feet high.  It moves\n\
slowly on a set of treads, and is armed with\n\
a large machine gun type weapon.  It is\n\
heavily armored.")
);
FLAGS(MF_SEES, MF_HEARS, MF_BASHES, MF_ATTACKMON, MF_ELECTRONIC, MF_NO_BREATHE, MF_VIS50);

mon(_("hazmatbot"),	species_robot, 'R',	c_ltgray,	MS_MEDIUM,	"steel",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 12,100, 40, 70,  8,  3,  2,  8,  3, 12,  10, 0, 120, 3,
	&mdeath::explode,	&mattack::none, _("\
A utility robot designed for hazardous\n\
conditions.  Its only means to stop intruders\n\
appears to involve thrashing around one of its\n\
multiple legs.")
);
FLAGS(MF_SEES, MF_HEARS, MF_BASHES, MF_ATTACKMON, MF_ELECTRONIC, MF_NO_BREATHE, MF_VIS50);

mon(_("copbot"),	species_robot, 'R',	c_ltblue,	MS_MEDIUM,	"steel",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 12,100, 40,100,  4,  3,  2,  0,  8, 12,  8, 80, 80, 3,
	&mdeath::normal,	&mattack::copbot, _("\
A blue-painted robot that moves quickly on a\n\
set of three omniwheels.  It has a nightstick\n\
readied, and appears to be well-armored.")
);
FLAGS(MF_SEES, MF_HEARS, MF_BASHES, MF_ATTACKMON, MF_ELECTRONIC, MF_NO_BREATHE, MF_VIS50);

mon(_("molebot"),	species_robot, 'R',	c_brown,	MS_MEDIUM,	"steel",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 17,100,100, 40, 13,  1,  4, 10,  0, 14, 14, 82, 80, 0,
	&mdeath::normal,	&mattack::none,	_("\
A snake-shaped robot that tunnels through the\n\
ground slowly.  When it emerges from the\n\
ground it can attack with its large, spike-\n\
covered head.")
);
FLAGS(MF_HEARS, MF_GOODHEARING, MF_DIGS, MF_NO_BREATHE, MF_ELECTRONIC);

mon(_("tripod robot"),species_robot, 'R',	c_white,	MS_LARGE,	"steel",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 26,100,100, 90, 15,  2,  4,  7,  0, 12,  8, 82, 80, 10,
	&mdeath::normal,	&mattack::flamethrower, _("\
A 8-foot-tall robot that walks on three long\n\
legs.  It has a pair of spiked tentacles, as\n\
well as a flamethrower mounted on its head.")
);
FLAGS(MF_SEES, MF_HEARS, MF_GOODHEARING, MF_BASHES, MF_NO_BREATHE, MF_ELECTRONIC);

mon(_("chicken walker"),species_robot, 'R',c_red,		MS_LARGE,	"steel",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 32,100,100,115, 0,  0,  0,  0,  0,  18, 14, 85, 90, 5,
	&mdeath::explode,	&mattack::smg, _("\
A 10-foot-tall, heavily-armored robot that\n\
walks on a pair of legs with the knees\n\
facing backwards.  It's armed with a\n\
nasty-looking machine gun.")
);
FLAGS(MF_SEES, MF_HEARS, MF_BASHES, MF_NO_BREATHE, MF_ELECTRONIC);

mon(_("tankbot"),	species_robot, 'R',	c_blue,		MS_HUGE,	"steel",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 52,100,100,100, 0,  0,  0,  0,  0,  22, 20, 92,240, 4,
	&mdeath::normal,	&mattack::multi_robot, _("\
This fearsome robot is essentially an\n\
autonomous tank.  It moves surprisingly fast\n\
on its treads, and is armed with a variety of\n\
deadly weapons.")
);
FLAGS(MF_SEES, MF_HEARS, MF_GOODHEARING, MF_NOHEAD, MF_BASHES, MF_DESTROYS,
      MF_ATTACKMON, MF_NO_BREATHE, MF_ELECTRONIC);

mon(_("turret"),	species_robot, 't',	c_ltgray,	MS_SMALL,	"steel",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 14,100,100,100,  0,  0,  0,  0,  0, 14, 16, 80, 30, 1,
	&mdeath::explode,	&mattack::smg, _("\
A small, round turret which extends from\n\
the floor.  Two SMG barrels swivel 360\n\
degrees.")
);
FLAGS(MF_SEES, MF_NOHEAD, MF_ELECTRONIC, MF_IMMOBILE, MF_NO_BREATHE, MF_FRIENDLY_SPECIAL);

mon(_("exploder"),	species_robot, 'm',	c_ltgray,	MS_LARGE,	"steel",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 14,100,100,100,  0,  0,  0,  0,  0,  0,  0, 88,  1, 1,
	&mdeath::explode,	&mattack::none, _("\
A small, round turret which extends from\n\
the floor.  Two SMG barrels swivel 360\n\
degrees.")
);
FLAGS(MF_IMMOBILE, MF_NO_BREATHE);


// HALLUCINATIONS
mon(_("zombie"),	species_hallu, 'Z',	c_ltgreen,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,100,100, 65,  3,  0,  0,  0,  0,  0,  0,  0,  1,  20,
	&mdeath::disappear,	&mattack::disappear, _("\
A human body, stumbling slowly forward on\n\
uncertain legs, possessed with an\n\
unstoppable rage.")
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_NO_BREATHE, MF_VIS40);

mon(_("giant bee"),species_hallu, 'a',	c_yellow,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,100,100,180,  2,  0,  0,  0,  0,  0,  0,  0,  1,  20,
	&mdeath::disappear,	&mattack::disappear, _("\
A honey bee the size of a small dog. It\n\
buzzes angrily through the air, dagger-\n\
sized sting pointed forward.")
);
FLAGS(MF_SMELLS, MF_NO_BREATHE, MF_FLIES);

mon(_("giant ant"),species_hallu, 'a',	c_brown,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,100,100,100,  3,  0,  0,  0,  0,  0,  0,  0,  1,  20,
	&mdeath::disappear,	&mattack::disappear, _("\
A red ant the size of a crocodile. It is\n\
covered in chitinous armor, and has a\n\
pair of vicious mandibles.")
);
FLAGS(MF_SMELLS, MF_NO_BREATHE);

mon(_("your mother"),species_hallu, '@',	c_white,	MS_MEDIUM,	"flesh",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,100,100,100,  3,  0,  0,  0,  0,  0,  0,  0,  5,  20,
	&mdeath::disappear,	&mattack::disappear, _("\
Mom?")
);
FLAGS(MF_SEES, MF_HEARS, MF_NO_BREATHE, MF_SMELLS, MF_GUILT);

mon(_("generator"), species_none, 'G',	c_white,	MS_LARGE,	"steel",
//	dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  0,  0,100,  0,  0,  0,  0,  0,  2,  2,  0,500, 1,
	&mdeath::gameover,	&mattack::generator, _("\
Your precious generator, noisily humming\n\
away.  Defend it at all costs!")
);
FLAGS(MF_NOHEAD, MF_ACIDPROOF, MF_IMMOBILE);

}


std::vector<monster_trigger> default_anger(monster_species spec)
{
 std::vector<monster_trigger> ret;
 switch (spec) {
  case species_mammal:
   break;
  case species_insect:
   ret.push_back(MTRIG_FRIEND_DIED);
   break;
  case species_worm:
   break;
  case species_zombie:
   break;
  case species_plant:
   break;
  case species_fungus:
   break;
  case species_nether:
   break;
  case species_robot:
   break;
  case species_hallu:
   break;
 }
 return ret;
}


std::vector<monster_trigger> default_fears(monster_species spec)
{
 std::vector<monster_trigger> ret;
 switch (spec) {
  case species_mammal:
   setvector(&ret, MTRIG_HURT, MTRIG_FIRE, MTRIG_FRIEND_DIED, NULL);
   break;
  case species_insect:
   setvector(&ret, MTRIG_HURT, MTRIG_FIRE, NULL);
   break;
  case species_worm:
   setvector(&ret, MTRIG_HURT, NULL);
   break;
  case species_zombie:
   break;
  case species_plant:
   setvector(&ret, MTRIG_HURT, MTRIG_FIRE, NULL);
   break;
  case species_fungus:
   setvector(&ret, MTRIG_HURT, MTRIG_FIRE, NULL);
   break;
  case species_nether:
   setvector(&ret, MTRIG_HURT, NULL);
   break;
  case species_robot:
   break;
  case species_hallu:
   break;
 }
 return ret;
}



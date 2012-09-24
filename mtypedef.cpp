#include "game.h"
#include "mondeath.h"
#include "monattack.h"
#include "itype.h"
#include "setvector.h"

// This function populates the master list of monster types.
// If you edit this function, you'll also need to edit:
//  * mtype.h - enum mon_id MUST match the order of this list!
//  * monster.cpp - void make_fungus() should be edited, or fungal infection
//                  will simply kill the monster
//  * mongroupdef.cpp - void init_moncats() should be edited, so the monster
//                      spawns with the proper group
// PLEASE NOTE: The description is AT MAX 4 lines of 46 characters each.

void game::init_mtypes ()
{
 int id = 0;
// Null monster named "None".
 mtypes.push_back(new mtype);

#define mon(name, species, sym, color, size, mat, \
            freq, diff, agro, morale, speed, melee_skill, melee_dice,\
            melee_sides, melee_cut, dodge, arm_bash, arm_cut, item_chance, HP,\
            sp_freq, death, sp_att, desc) \
id++;\
mtypes.push_back(new mtype(id, name, species, sym, color, size, mat,\
freq, diff, agro, morale, speed, melee_skill, melee_dice, melee_sides,\
melee_cut, dodge, arm_bash, arm_cut, item_chance, HP, sp_freq, death, sp_att,\
desc))

#define FLAGS(...)   setvector(mtypes[id]->flags,   __VA_ARGS__, NULL)
#define ANGER(...)   setvector(mtypes[id]->anger,   __VA_ARGS__, NULL)
#define FEARS(...)   setvector(mtypes[id]->fear,    __VA_ARGS__, NULL)
#define PLACATE(...) setvector(mtypes[id]->placate, __VA_ARGS__, NULL)

// PLEASE NOTE: The description is AT MAX 4 lines of 46 characters each.

// FOREST ANIMALS
mon("squirrel",	species_mammal, 'r',	c_ltgray,	MS_TINY,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 50,  0,-99, -8,140,  0,  1,  1,  0,  4,  0,  0,  0,  1,  0,
	&mdeath::normal,	&mattack::none, "\
A small woodland animal."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_ANIMAL, MF_WARM, MF_FUR);

mon("rabbit",	species_mammal, 'r',	c_white,	MS_TINY,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 10,  0,-99, -7,180, 0,  0,  0,  0,  6,  0,  0,  0,  4,  0,
	&mdeath::normal,	&mattack::none, "\
A cute wiggling nose, cotton tail, and\n\
delicious flesh."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_ANIMAL, MF_WARM, MF_FUR);

mon("deer",	species_mammal, 'd',	c_brown,	MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3,  1,-99, -5,300,  4,  3,  3,  0,  3,  0,  0,  0, 80, 0,
	&mdeath::normal,	&mattack::none, "\
A large buck, fast-moving and strong."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_ANIMAL, MF_WARM, MF_FUR);

mon("wolf",	species_mammal, 'w',	c_dkgray,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  4, 12,  0, 20,165, 14,  2,  3,  4,  4,  1,  0,  0, 28,  0,
	&mdeath::normal,	&mattack::none, "\
A vicious and fast pack hunter."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_ANIMAL, MF_WARM, MF_FUR, MF_HIT_AND_RUN);
ANGER(MTRIG_TIME, MTRIG_PLAYER_WEAK, MTRIG_HURT);
PLACATE(MTRIG_MEAT);

mon("bear",	species_mammal, 'B',	c_dkgray,	MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2, 10,-10, 40,140, 10,  3,  4,  6,  3,  2,  0,  0, 90, 0,
	&mdeath::normal,	&mattack::none, "\
Remember, only YOU can prevent forest fires."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_ANIMAL, MF_WARM, MF_FUR);
ANGER(MTRIG_PLAYER_CLOSE);
PLACATE(MTRIG_MEAT);

// DOMESICATED ANIMALS
mon("dog",	species_mammal, 'd',	c_white,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 1,   5,  2, 15,150, 12,  2,  3,  3,  3,  0,  0,  0, 25,  0,
	&mdeath::normal,	&mattack::none, "\
A medium-sized domesticated dog, gone feral."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_ANIMAL, MF_WARM, MF_FUR, MF_HIT_AND_RUN);

// INSECTOIDS
mon("ant larva",species_insect, 'a',	c_white,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1,  0, -1, 10, 65,  4,  1,  3,  0,  0,  0,  0,  0, 10,  0,
	&mdeath::normal,	&mattack::none, "\
The size of a large cat, this pulsating mass\n\
of glistening white flesh turns your stomach."
);
FLAGS(MF_SMELLS, MF_POISON);

mon("giant ant",species_insect, 'a',	c_brown,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 20,  7, 15, 60,100,  9,  1,  6,  4,  2,  4,  8,-40, 40,  0,
	&mdeath::normal,	&mattack::none, "\
A red ant the size of a crocodile. It is\n\
covered in chitinous armor, and has a pair of\n\
vicious mandibles."
);
FLAGS(MF_SMELLS);

mon("soldier ant",species_insect, 'a',	c_blue,		MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2, 16, 25,100,115, 12,  2,  4,  6,  2,  5, 10,-50, 80,  0,
	&mdeath::normal,	&mattack::none, "\
Darker in color than the other ants, this\n\
more aggresive variety has even larger\n\
mandibles."
);
FLAGS(MF_SMELLS);

mon("queen ant",species_insect, 'a',	c_ltred,	MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0, 13,  0,100, 60,  6,  3,  4,  4,  1,  6, 14,-40,180, 1,
	&mdeath::normal,	&mattack::antqueen, "\
This ant has a long, bloated thorax, bulging\n\
with hundreds of small ant eggs.  It moves\n\
slowly, tending to nearby eggs and laying\n\
still more."
);
FLAGS(MF_SMELLS, MF_QUEEN);

mon("fungal insect",species_fungus, 'a',c_ltgray,	MS_MEDIUM,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  5, 80,100, 75,  5,  1,  5,  3,  1,  1,  1,  0, 30, 60,
	&mdeath::normal,	&mattack::fungus, "\
This insect is pale gray in color, its\n\
chitin weakened by the fungus sprouting\n\
from every joint on its body."
);
FLAGS(MF_SMELLS, MF_POISON);

mon("giant fly",species_insect, 'a',	c_ltgray,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  8, 50, 30,120,  3,  1,  3,  0,  5,  0,  0,  0, 25,  0,
	&mdeath::normal,	&mattack::none, "\
A large housefly the size of a small dog.\n\
It buzzes around incessantly."
);
FLAGS(MF_SMELLS, MF_FLIES, MF_STUMBLES, MF_HIT_AND_RUN);

mon("giant bee",species_insect, 'a',	c_yellow,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2, 15,-10, 50,140,  4,  1,  1,  5,  6,  0,  5,-50, 20,  0,
	&mdeath::normal,	&mattack::none, "\
A honey bee the size of a small dog. It\n\
buzzes angrily through the air, dagger-\n\
sized sting pointed forward."
);
FLAGS(MF_SMELLS, MF_VENOM, MF_FLIES, MF_STUMBLES, MF_HIT_AND_RUN);
ANGER(MTRIG_FRIEND_DIED, MTRIG_PLAYER_CLOSE);

mon("giant wasp",species_insect, 'a', 	c_red,		MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2, 22,  5, 60,150,  6,  1,  3,  7,  7,  0,  7,-40, 35, 0,
	&mdeath::normal,	&mattack::none, "\
An evil-looking, slender-bodied wasp with\n\
a vicious sting on its abdomen."
);
FLAGS(MF_SMELLS, MF_POISON, MF_VENOM, MF_FLIES);
ANGER(MTRIG_FRIEND_DIED, MTRIG_PLAYER_CLOSE, MTRIG_SOUND);

// GIANT WORMS

mon("graboid",	species_worm, 'S',	c_red,		MS_HUGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 17, 30,120,180, 11,  3,  8,  4,  0,  5,  5,  0,180,  0,
	&mdeath::worm,		&mattack::none, "\
A hideous slithering beast with a tri-\n\
sectional mouth that opens to reveal\n\
hundreds of writhing tongues. Most of its\n\
enormous body is hidden underground."
);
FLAGS(MF_DIGS, MF_HEARS, MF_GOODHEARING, MF_DESTROYS, MF_WARM, MF_LEATHER);

mon("giant worm",species_worm, 'S',	c_pink,		MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 30, 10, 30,100, 85,  9,  4,  5,  2,  0,  2,  0,  0, 50,  0,
	&mdeath::worm,		&mattack::none, "\
Half of this monster is emerging from a\n\
hole in the ground. It looks like a huge\n\
earthworm, but the end has split into a\n\
large, fanged mouth."
);
FLAGS(MF_DIGS, MF_HEARS, MF_GOODHEARING, MF_WARM, MF_LEATHER);

mon("half worm",species_worm, 's',	c_pink,		MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  2, 10, 40, 80,  5,  3,  5,  0,  0,  0,  0,  0, 20,  0,
	&mdeath::normal,	&mattack::none, "\
A portion of a giant worm that is still alive."
);
FLAGS(MF_DIGS, MF_HEARS, MF_GOODHEARING, MF_WARM, MF_LEATHER);

// ZOMBIES
mon("zombie",	species_zombie, 'Z',	c_ltgreen,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 90,  3,100,100, 70,  8,  1,  5,  0,  1,  0,  0, 40, 50,  0,
	&mdeath::normal,	&mattack::none, "\
A human body, stumbling slowly forward on\n\
uncertain legs, possessed with an unstoppable\n\
rage."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON);

mon("shrieker zombie",species_zombie, 'Z',c_magenta,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  4, 10,100,100, 95,  9,  1,  2,  0,  4,  0,  0, 45, 50, 10,
	&mdeath::normal,	&mattack::shriek, "\
This zombie's jaw has been torn off, leaving\n\
a gaping hole from mid-neck up."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON);

mon("spitter zombie",species_zombie, 'Z',c_yellow,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  4,  9,100,100,105,  8,  1,  5,  0,  4,  0,  0, 30, 60, 20,
	&mdeath::acid,		&mattack::acid,	"\
This zombie's mouth is deformed into a round\n\
spitter, and its body throbs with a dense\n\
yellow fluid."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON,
      MF_ACIDPROOF);

mon("shocker zombie",species_zombie,'Z',c_ltcyan,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3, 10,100,100,110,  8,  1,  6,  0,  4,  0,  0, 40, 65, 25,
	&mdeath::normal,	&mattack::shockstorm, "\
This zombie's flesh is pale blue, and it\n\
occasionally crackles with small bolts of\n\
lightning."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON,
      MF_ELECTRIC);


mon("fast zombie",species_zombie, 'Z',	c_ltred,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  6, 12,100,100,150, 10,  1,  4,  3,  4,  0,  0, 45, 40,  0,
	&mdeath::normal,	&mattack::none, "\
This deformed, sinewy zombie stays close to\n\
the ground, loping forward faster than most\n\
humans ever could."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON,
      MF_HIT_AND_RUN);

mon("zombie brute",species_zombie, 'Z',	c_red,		MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  4, 25,100,100,115,  9,  4,  4,  2,  0,  6,  3, 60, 80,  0,
	&mdeath::normal,	&mattack::none, "\
A hideous beast of a zombie, bulging with\n\
distended muscles on both arms and legs."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON);

mon("zombie hulk",species_zombie, 'Z',	c_blue,		MS_HUGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 50,100,100,130,  9,  4,  8,  0,  0, 12,  8, 80,260,  0,
	&mdeath::normal,	&mattack::none, "\
A zombie that has somehow grown to the size of\n\
6 men, with arms as wide as a trash can."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES,
      MF_DESTROYS, MF_POISON, MF_ATTACKMON, MF_LEATHER);

mon("fungal zombie",species_fungus, 'Z',c_ltgray,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2,  6,100,100, 45,  6,  1,  6,  0,  0,  0,  0, 20, 40, 50,
	&mdeath::normal,	&mattack::fungus, "\
A diseased zombie. Fungus sprouts from its\n\
mouth and eyes, and thick gray mold grows all\n\
over its translucent flesh."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON);

mon("boomer",	species_zombie, 'Z',	c_pink,		MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  6,  5,100,100, 55,  7,  2,  4,  0,  1,  3,  0, 35, 40,  20,
	&mdeath::boomer,	&mattack::boomer, "\
A bloated zombie sagging with fat. It emits a\n\
horrible odor, and putrid, pink sludge drips\n\
from its mouth."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON);

mon("fungal boomer",species_fungus, 'B',c_ltgray,	MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1,  7,100,100, 40,  5,  2,  6,  0,  0,  3,  0, 20, 20, 30,
	&mdeath::fungus,	&mattack::fungus, "\
A bloated zombie that is coated with slimy\n\
gray mold. Its flesh is translucent and gray,\n\
and it dribbles a gray sludge from its mouth."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON);

mon("skeleton",	species_zombie, 'Z',	c_white,	MS_MEDIUM,	STONE,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  7,  8,100,100, 90, 10,  1,  5,  3,  2,  0, 15,  0, 40, 0,
	&mdeath::normal,	&mattack::none, "\
A skeleton picked clean of all but a few\n\
rotten scraps of flesh, somehow still in\n\
motion."
);
FLAGS(MF_SEES, MF_HEARS, MF_HARDTOSHOOT);

mon("zombie necromancer",species_zombie, 'Z',c_dkgray,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3, 13,  0,100,100,  4,  2,  3,  0,  4,  0,  0, 50,140, 4,
	&mdeath::normal,	&mattack::resurrect, "\
A zombie with jet black skin and glowing red\n\
eyes.  As you look at it, you're gripped by a\n\
feeling of dread and terror."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON);
ANGER(MTRIG_HURT, MTRIG_PLAYER_WEAK);

mon("zombie scientist",species_zombie, 'Z',c_ltgray,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 20,  3,100,100, 75,  7,  1,  3,  0,  1,  0,  0, 50, 35, 20,
	&mdeath::normal,	&mattack::science, "\
A zombie wearing a tattered lab coat and\n\
some sort of utility belt.  It looks weaker\n\
than most zombies, but more resourceful too."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON,
      MF_ACIDPROOF);

mon("zombie soldier",	species_zombie,	'Z',c_ltblue,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 20,100,100, 80, 12,  2,  4,  0,  0,  8, 16, 60,100, 0,
	&mdeath::normal,	&mattack::none, "\
This zombie was clearly a soldier before.\n\
Its tattered armor gives it strong defense,\n\
and it is much more physically fit than\n\
most zombies."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_WARM, MF_BASHES, MF_POISON);

mon("grabber zombie",	species_zombie,	'Z',c_green,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  6,  8,100,100, 70, 10,  1,  2,  0,  4,  0,  0, 40, 65, 0,
	&mdeath::normal,	&mattack::none, "\
This zombie seems to have slightly longer\n\
than ordinary arms, and constantly gropes\n\
at its surroundings as it moves."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_WARM, MF_BASHES, MF_POISON,
      MF_GRABS);

mon("master zombie",	species_zombie, 'M',c_yellow,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 16,  5,100, 90,  4,  1,  3,  0,  4,  0,  0, 60,120, 3,
	&mdeath::normal,	&mattack::upgrade, "\
This zombie seems to have a cloud of black\n\
dust surrounding it.  It also seems to have\n\
a better grasp of movement than most..."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_WARM, MF_BASHES, MF_POISON);
ANGER(MTRIG_HURT, MTRIG_PLAYER_WEAK);

// PLANTS & FUNGI
mon("triffid",	species_plant, 'F',	c_ltgreen,	MS_MEDIUM,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
 	 24, 16, 20,100, 75,  9,  2,  4,  5,  0, 10,  2,  0, 80,  0,
	&mdeath::normal,	&mattack::none, "\
A plant that grows as high as your head,\n\
with one thick, bark-coated stalk\n\
supporting a flower-like head with a sharp\n\
sting within."
);
FLAGS(MF_HEARS, MF_SMELLS, MF_BASHES, MF_NOHEAD);

mon("young triffid",species_plant, 'f',	c_ltgreen,	MS_SMALL,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 15,  2,  0, 10, 65,  7,  1,  4,  3,  0,  0,  0,  0, 40,  0,
	&mdeath::normal,	&mattack::none, "\
A small triffid, only a few feet tall. It\n\
has not yet developed bark, but its sting\n\
is still sharp and deadly."
);
FLAGS(MF_HEARS, MF_SMELLS, MF_NOHEAD);

mon("queen triffid",species_plant, 'F',	c_red,		MS_LARGE,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3, 28,100,200, 85, 14,  2,  7,  8,  0, 10,  4,  0,280, 2,
	&mdeath::normal,	&mattack::growplants, "\
A very large triffid, with a particularly\n\
vicious sting and thick bark.  As it\n\
moves, plant matter drops off its body\n\
and immediately takes root."
);
FLAGS(MF_HEARS, MF_SMELLS, MF_BASHES, MF_NOHEAD);

mon("creeper hub",species_plant, 'V',	c_dkgray,	MS_MEDIUM,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0, 16,100,100,100,  0,  0,  0,  0,  0,  8,  0,  0,100, 2,
	&mdeath::kill_vines,	&mattack::grow_vine, "\
A thick stalk, rooted to the ground.\n\
It rapidly sprouts thorny vines in all\n\
directions."
);
FLAGS(MF_NOHEAD, MF_IMMOBILE);

mon("creeping vine",species_plant, 'v',	c_green,	MS_TINY,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  4,100,100, 75,  0,  0,  0,  0,  0,  2,  0,  0,  2, 2,
	&mdeath::vine_cut,	&mattack::vine, "\
A thorny vine.  It twists wildly as\n\
it grows, spreading rapidly."
);
FLAGS(MF_NOHEAD, MF_HARDTOSHOOT, MF_PLASTIC, MF_IMMOBILE);

mon("biollante",species_plant, 'F',	c_magenta,	MS_LARGE,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0, 20,100,100,100,  0,  0,  0,  0,  0,  0,  0,-80,120, 2,
	&mdeath::normal,	&mattack::spit_sap, "\
A thick stalk topped with a purple\n\
flower.  The flower's petals are closed,\n\
and pulsate ominously."
);
FLAGS(MF_NOHEAD, MF_IMMOBILE);

mon("vinebeast",species_plant, 'V',	c_red,		MS_LARGE,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  8, 14, 60, 40, 75, 15,  2,  4,  2,  4, 10,  0,  0,100, 0,
	&mdeath::normal,	&mattack::none,	"\
This appears to be a mass of vines, moving\n\
with surprising speed.  It is so thick and\n\
tangled that you cannot see what lies in\n\
the middle."
);
FLAGS(MF_HEARS, MF_GOODHEARING, MF_NOHEAD, MF_HARDTOSHOOT, MF_GRABS,
      MF_SWIMS, MF_PLASTIC);

mon("triffid heart",species_plant, 'T',	c_red,		MS_HUGE,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
          0, 45,100,100,100,  0,  0,  0,  0,  0, 14,  4,  0,300, 5,
	&mdeath::triffid_heart,	&mattack::triffid_heartbeat, "\
A knot of roots that looks bizarrely like a\n\
heart.  It beats slowly with sap, powering\n\
the root walls around it."
);
FLAGS(MF_NOHEAD, MF_IMMOBILE, MF_QUEEN);

mon("fungaloid",species_fungus, 'F',	c_ltgray,	MS_MEDIUM,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 12, 12,  0,100, 45,  8,  3,  3,  0,  0,  4,  0,  0, 80, 200,
	&mdeath::fungus,	&mattack::fungus, "\
A pale white fungus, one meaty gray stalk\n\
supporting a bloom at the top. A few\n\
tendrils extend from the base, allowing\n\
mobility and a weak attack."
);
ANGER(MTRIG_PLAYER_CLOSE, MTRIG_PLAYER_WEAK);
FLAGS(MF_HEARS, MF_SMELLS, MF_POISON, MF_NOHEAD);

// This is a "dormant" fungaloid that doesn't waste CPU cycles ;)
mon("fungaloid",species_fungus, 'F',	c_ltgray,	MS_MEDIUM,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  0,  0,100,  1,  8,  2,  4,  0,  0,  4,  0,  0,  1, 0,
	&mdeath::fungusawake,	&mattack::none, "\
A pale white fungus, one meaty gray stalk\n\
supporting a bloom at the top. A few\n\
tendrils extend from the base, allowing\n\
mobility and a weak attack."
);
FLAGS(MF_HEARS, MF_SMELLS, MF_POISON, MF_NOHEAD);

mon("young fungaloid",species_fungus, 'f',c_ltgray,	MS_SMALL,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  6,  6,100,100, 65,  8,  1,  4,  6,  0,  4,  4,  0, 70, 0,
	&mdeath::normal,	&mattack::none, "\
A fungal tendril just a couple feet tall.  Its\n\
exterior is hardened into a leathery bark and\n\
covered in thorns; it also moves faster than\n\
full-grown fungaloids."
);
FLAGS(MF_HEARS, MF_SMELLS, MF_POISON, MF_NOHEAD);

mon("spore",	species_fungus, 'o',	c_ltgray,	MS_TINY,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  1,-50,100,100,  0,  0,  0,  0,  6,  0,  0,  0,  5, 50,
	&mdeath::disintegrate,	&mattack::plant, "\
A wispy spore, about the size of a fist,\n\
wafting on the breeze."
);
FLAGS(MF_STUMBLES, MF_FLIES, MF_POISON, MF_NOHEAD);

mon("fungal spire",species_fungus, 'T',	c_ltgray,	MS_HUGE,	STONE,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0, 40,100,100,100,  0,  0,  0,  0,  0, 10, 10,  0,300, 5,
	&mdeath::fungus,	&mattack::fungus_sprout, "\
An enormous fungal spire, towering 30 feet\n\
above the ground.  It pulsates slowly,\n\
continuously growing new defenses."
);
FLAGS(MF_NOHEAD, MF_POISON, MF_IMMOBILE, MF_QUEEN);

mon("fungal wall",species_fungus, 'F',	c_dkgray,	MS_HUGE,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  5,100,100,100,  0,  0,  0,  0,  0, 10, 10,  0, 60, 8,
	&mdeath::disintegrate,	&mattack::fungus, "\
A veritable wall of fungus, grown as a\n\
natural defense by the fungal spire. It\n\
looks very tough, and spews spores at an\n\
alarming rate."
);
FLAGS(MF_NOHEAD, MF_POISON, MF_IMMOBILE);

// BLOBS & SLIMES &c
mon("blob",	species_nether, 'O',	c_dkgray,	MS_MEDIUM,	LIQUID,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 10, 19,100,100, 85,  9,  2,  4,  0,  0,  6,  0,  0, 85, 30,
	&mdeath::blobsplit,	&mattack::formblob, "\
A black blob of viscous goo that oozes\n\
across the ground like a mass of living\n\
oil."
);
FLAGS(MF_SMELLS, MF_HEARS, MF_GOODHEARING, MF_NOHEAD, MF_POISON, MF_ACIDPROOF);

mon("small blob",species_nether, 'o',	c_dkgray,	MS_SMALL,	LIQUID,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1,  2,100,100, 50,  6,  1,  4,  0,  0,  2,  0,  0, 50, 0,
	&mdeath::blobsplit,	&mattack::none, "\
A small blob of viscous goo that oozes\n\
across the ground like a mass of living\n\
oil."
);
FLAGS(MF_SMELLS, MF_HEARS, MF_GOODHEARING, MF_NOHEAD, MF_POISON, MF_ACIDPROOF);

// CHUDS & SUBWAY DWELLERS
mon("C.H.U.D.",	species_none, 'S',	c_ltgray,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 50,  8, 30, 30,110, 10,  1,  5,  0,  3,  0,  0, 25, 60, 0,
	&mdeath::normal,	&mattack::none,	"\
Cannibalistic Humanoid Underground Dweller.\n\
A human, turned pale and mad from years in\n\
the subways."
);
FLAGS(MF_SEES, MF_HEARS, MF_WARM, MF_BASHES);
FEARS(MTRIG_HURT, MTRIG_FIRE);

mon("one-eyed mutant",species_none, 'S',c_ltred,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  5, 18, 30, 30,130, 20,  2,  4,  0,  5,  0,  0, 40, 80, 0,
	&mdeath::normal,	&mattack::none,	"\
A relatively humanoid mutant with purple\n\
hair and a grapefruit-sized bloodshot eye."
);
FLAGS(MF_SEES, MF_HEARS, MF_WARM, MF_BASHES);
FEARS(MTRIG_HURT, MTRIG_FIRE);

mon("crawler mutant",species_none, 'S',	c_red,		MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2, 16, 40, 40, 80, 10,  2,  6,  0,  1,  8,  0,  0,180, 0,
	&mdeath::normal,	&mattack::none,	"\
Two or three humans fused together somehow,\n\
slowly dragging their thick-hided, hideous\n\
body across the ground."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_WARM, MF_BASHES, MF_POISON);
FEARS(MTRIG_HURT, MTRIG_FIRE);

mon("sewer fish",species_none, 's',	c_ltgreen,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 30, 13,100,100,120, 17,  1,  3,  3,  6,  0,  0,  0, 20, 0,
	&mdeath::normal,	&mattack::none,	"\
A large green fish, it's mouth lined with\n\
three rows of razor-sharp teeth."
);
FLAGS(MF_SEES, MF_SMELLS, MF_WARM, MF_AQUATIC);
FEARS(MTRIG_HURT);

mon("sewer snake",species_none, 's',	c_yellow,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 15,  2, 20, 40, 60, 12,  1,  2,  5,  1,  0,  0,  0, 40, 0,
	&mdeath::normal,	&mattack::none,	"\
A large snake, turned pale yellow from its\n\
underground life."
);
FLAGS(MF_SEES, MF_SMELLS, MF_WARM, MF_VENOM, MF_SWIMS, MF_LEATHER);
FEARS(MTRIG_HURT);

mon("sewer rat",species_mammal, 's',	c_dkgray,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 18,  3, 20, 40,105, 10,  1,  2,  1,  2,  0,  0,  0, 30, 0,
	&mdeath::normal,	&mattack::none, "\
A large, mangey rat with red eyes.  It\n\
scampers quickly across the ground, squeaking\n\
hungrily."
);
FLAGS(MF_SEES, MF_SMELLS, MF_HEARS, MF_WARM, MF_SWIMS, MF_ANIMAL, MF_FUR);

mon("rat king",species_mammal, 'S',	c_dkgray,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0, 18, 10,100, 40,  4,  1,  3,  1,  0,  0,  0,  0,220, 3,
	&mdeath::ratking,	&mattack::ratking, "\
A group of several rats, their tails\n\
knotted together in a filthy mass.  A wave\n\
of nausea washes over you in its presence."
);

// SWAMP CREATURES
mon("giant mosquito",species_insect, 'y',c_ltgray,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 22, 12, 20, 20,120,  8,  1,  1,  1,  5,  0,  0,  0, 20, 0,
	&mdeath::normal,	&mattack::none, "\
An enormous mosquito, fluttering erratically,\n\
its face dominated by a long, spear-tipped\n\
proboscis."
);
FLAGS(MF_SMELLS, MF_HEARS, MF_STUMBLES, MF_VENOM, MF_FLIES, MF_HIT_AND_RUN);

mon("giant dragonfly",species_insect, 'y',c_ltgreen,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  6, 13, 20,100,155, 12,  1,  3,  6,  5,  0,  6,-20, 70, 0,
	&mdeath::normal,	&mattack::none, "\
A ferocious airborne predator, flying swiftly\n\
through the air, its mouth a cluster of fangs."
);
FLAGS(MF_SEES, MF_SMELLS, MF_FLIES, MF_HIT_AND_RUN);

mon("giant centipede",species_insect, 'a',c_ltgreen,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  7,  9, 20,100,120, 10,  1,  3,  5,  2,  0,  8,-30, 60, 0,
	&mdeath::normal,	&mattack::none, "\
A meter-long centipede, moving swiftly on\n\
dozens of thin legs, a pair of venomous\n\
pincers attached to its head."
);
FLAGS(MF_SMELLS, MF_HEARS, MF_POISON, MF_VENOM);

mon("giant frog",species_none, 'F',	c_green,	MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  5, 10, 10,100, 90,  8,  2,  3,  0,  2,  4,  0,  0, 70, 5,
	&mdeath::normal,	&mattack::leap, "\
A thick-skinned green frog.  It eyes you\n\
much as you imagine it might eye an insect."
);
FLAGS(MF_SEES, MF_SMELLS, MF_HEARS, MF_SWIMS, MF_LEATHER);
FEARS(MTRIG_HURT);

mon("giant slug",species_none, 'S',	c_yellow,	MS_HUGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  4, 16, 20,100, 60,  7,  1,  5,  1,  0,  8,  2,  0,190, 10,
	&mdeath::normal,	&mattack::acid, "\
A gigantic slug, the size of a small car.\n\
It moves slowly, dribbling acidic goo from\n\
its fang-lined mouth."
);
FLAGS(MF_SEES, MF_SMELLS, MF_BASHES, MF_ACIDPROOF, MF_ACIDTRAIL);
FEARS(MTRIG_HURT, MTRIG_PLAYER_CLOSE);

mon("dermatik larva",species_insect, 'i',c_white,	MS_TINY,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  4,-20,-20, 20,  1,  1,  2,  2,  1,  0,  0,  0, 10, 0,
	&mdeath::normal,	&mattack::none, "\
A fat, white grub the size of your foot, with\n\
a set of mandibles that look more suited for\n\
digging than fighting."
);
FLAGS(MF_HEARS, MF_SMELLS, MF_POISON, MF_DIGS);

mon("dermatik",	species_insect, 'i',	c_red,		MS_TINY,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3, 18, 30, 30,100,  5,  1,  1,  6, 7,   0,  6,  0, 60, 50,
	&mdeath::normal,	&mattack::dermatik, "\
A wasp-like flying insect, smaller than most\n\
mutated wasps.  It does not looke very\n\
threatening, but has a large ovipositor in\n\
place of a sting."
);
FLAGS(MF_HEARS, MF_SMELLS, MF_STUMBLES, MF_POISON, MF_FLIES);


// SPIDERS
mon("wolf spider",species_insect, 's',	c_brown,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 20, 20,100,110, 7,   1,  1,  8,  6,  2,  8,-70, 40, 0,
	&mdeath::normal,	&mattack::none, "\
A large, brown spider, which moves quickly\n\
and aggresively."
);
FLAGS(MF_SMELLS, MF_HEARS, MF_VENOM);

mon("web spider",species_insect, 's',	c_yellow,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 16, 30, 80,120,  5,  1,  1,  7,  5,  2,  7,-70, 35, 0,
	&mdeath::normal,	&mattack::none, "\
A yellow spider the size of a dog.  It lives\n\
in webs, waiting for prey to become\n\
entangled before pouncing and biting."
);
FLAGS(MF_SMELLS, MF_HEARS, MF_VENOM, MF_WEBWALK);

mon("jumping spider",species_insect, 's',c_white,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2, 14, 40, 80,100,  7,  1,  1,  4,  8,  0,  3,-60, 30, 2,
	&mdeath::normal,	&mattack::leap, "\
A small, almost cute-looking spider.  It\n\
leaps so quickly that it almost appears to\n\
instantaneously move from one place to\n\
another."
);
FLAGS(MF_SMELLS, MF_HEARS, MF_VENOM, MF_HIT_AND_RUN);

mon("trap door spider",species_insect, 's',c_blue,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 20, 60,100,110,  5,  1,  2,  7,  3,  2,  8,-80, 70, 0,
	&mdeath::normal,	&mattack::none, "\
A large spider with a bulbous thorax.  It\n\
creates a subterranean nest and lies in\n\
wait for prey to fall in and become trapped\n\
in its webs."
);
FLAGS(MF_SMELLS, MF_HEARS, MF_VENOM, MF_WEBWALK);

mon("black widow",species_insect, 's',	c_dkgray,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 20,-10,100, 90,  6,  1,  1,  6,  3,  0,  3,-50, 40, 0,
	&mdeath::normal,	&mattack::none, "\
A spider with a characteristic red\n\
hourglass on its black carapace.  It is\n\
known for its highly toxic venom."
);
FLAGS(MF_SMELLS, MF_HEARS, MF_BADVENOM, MF_WEBWALK);
ANGER(MTRIG_PLAYER_WEAK, MTRIG_PLAYER_CLOSE, MTRIG_HURT);

// UNEARTHED HORRORS
mon("dark wyrm",species_none, 'S',	c_blue,		MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 20,100,100,100,  8,  2,  6,  4,  4,  6,  0,  0,120, 0,
	&mdeath::normal,	&mattack::none, "\
A huge, black worm, its flesh glistening\n\
with an acidic, blue slime.  It has a gaping\n\
round mouth lined with dagger-like teeth."
);
FLAGS(MF_SMELLS, MF_HEARS, MF_GOODHEARING, MF_DESTROYS, MF_POISON, MF_SUNDEATH,
      MF_ACIDPROOF, MF_ACIDTRAIL);

mon("Amigara horror",species_none, 'H',	c_white,	MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 30,100,100, 70, 10,  2,  4,  0,  2,  0,  0,  0,250, 0,
	&mdeath::amigara,	&mattack::fear_paralyze, "\
A spindly body, standing at least 15 feet\n\
tall.  It looks vaguely human, but its face is\n\
grotesquely stretched out, and its limbs are\n\
distorted to the point of being tentacles."
);
FLAGS(MF_SMELLS, MF_HEARS, MF_SEES, MF_STUMBLES, MF_HARDTOSHOOT);

// This "dog" is, in fact, a disguised Thing
mon("dog",	species_nether, 'd',	c_white,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3,  5,100,100,150, 12,  2,  3,  3,  3,  0,  0,  0, 25, 40,
	&mdeath::thing,		&mattack::dogthing, "\
A medium-sized domesticated dog, gone feral."
);
FLAGS(MF_SEES, MF_SMELLS, MF_HEARS, MF_ANIMAL, MF_WARM, MF_FUR,
      MF_FRIENDLY_SPECIAL);

mon("tentacle dog",species_nether, 'd',	c_dkgray,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 14,100,100,120, 12,  2,  4,  0,  3,  0,  0,  0,120, 5,
	&mdeath::thing,		&mattack::tentacle, "\
A dog's body with a mass of ropy, black\n\
tentacles extending from its head."
);
FLAGS(MF_SEES, MF_SMELLS, MF_HEARS, MF_BASHES);

mon("Thing",	species_nether, 'H',	c_dkgray,	MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 25,100,100,135, 14,  2,  4,  0,  5,  8,  0,  0,160, 5,
	&mdeath::melt,		&mattack::tentacle, "\
An amorphous black creature which seems to\n\
sprout tentacles rapidly."
);
FLAGS(MF_SMELLS, MF_HEARS, MF_NOHEAD, MF_BASHES, MF_SWIMS, MF_ATTACKMON,
      MF_PLASTIC, MF_ACIDPROOF);

mon("human snail",species_none, 'h',	c_green,	MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 20, 10,  0, 30, 50,  4,  1,  5,  0,  0,  6, 12,  0, 50, 15,
	&mdeath::normal,	&mattack::acid, "\
A large snail, with an oddly human face."
);
FLAGS(MF_SMELLS, MF_HEARS, MF_POISON, MF_ACIDPROOF, MF_ACIDTRAIL);
ANGER(MTRIG_PLAYER_WEAK, MTRIG_FRIEND_DIED);
FEARS(MTRIG_PLAYER_CLOSE, MTRIG_HURT);

mon("twisted body",species_none, 'h',	c_pink,		MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  5, 12,100,100, 90,  5,  2,  4,  0,  6,  0,  0,  0, 65, 0,
	&mdeath::normal,	&mattack::none, "\
A human body, but with its limbs, neck, and\n\
hair impossibly twisted."
);
FLAGS(MF_SEES, MF_HEARS, MF_GOODHEARING, MF_POISON);

mon("vortex",	species_none, 'v',	c_white,	MS_SMALL,	POWDER,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2, 30,100,100,120,  0,  0,  0,  0,  0,  0,  0,  0, 20, 6,
	&mdeath::melt,		&mattack::vortex, "\
A twisting spot in the air, with some kind\n\
of morphing mass at its center."
);
FLAGS(MF_HEARS, MF_GOODHEARING, MF_STUMBLES, MF_NOHEAD, MF_HARDTOSHOOT,
      MF_FLIES, MF_PLASTIC, MF_FRIENDLY_SPECIAL);

// NETHER WORLD INHABITANTS
mon("flying polyp",species_nether, 'H',	c_dkgray,	MS_HUGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 42,100,100,280, 16,  3,  8,  6,  7,  8,  0,  0,350, 0,
	&mdeath::melt,		&mattack::none, "\
An amorphous mass of twisting black flesh\n\
that flies through the air swiftly."
);
FLAGS(MF_SMELLS, MF_HEARS, MF_GOODHEARING, MF_NOHEAD, MF_BASHES, MF_FLIES,
      MF_ATTACKMON, MF_PLASTIC, MF_HIT_AND_RUN);

mon("hunting horror",species_nether, 'h',c_dkgray,	MS_SMALL,	MNULL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 10, 28,100,100,180, 15,  3,  4,  0,  6,  0,  0,  0, 80, 0,
	&mdeath::melt,		&mattack::none, "\
A ropy, worm-like creature that flies on\n\
bat-like wings. Its form continually\n\
shifts and changes, twitching and\n\
writhing."
);
FLAGS(MF_SEES, MF_SMELLS, MF_HEARS, MF_NOHEAD, MF_HARDTOSHOOT, MF_FLIES,
      MF_PLASTIC, MF_SUNDEATH, MF_HIT_AND_RUN);

mon("Mi-go",	species_nether, 'H',	c_pink,		MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  5, 26, 20, 30,120, 14,  5,  3, 10,  7,  4, 12,  0,110, 0,
	&mdeath::normal,	&mattack::none, "\
A pinkish, fungoid crustacean-like\n\
creature with numerous pairs of clawed\n\
appendages and a head covered with waving\n\
antennae."
);
FLAGS(MF_SEES, MF_SMELLS, MF_HEARS, MF_WARM, MF_BASHES, MF_POISON);

mon("yugg",	species_nether, 'H',	c_white,	MS_HUGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3, 32,100,100, 80, 12,  3,  5,  8,  1,  6,  0,  0,320, 20,
	&mdeath::normal,	&mattack::gene_sting, "\
An enormous white flatworm writhing\n\
beneath the earth. Poking from the\n\
ground is a bulbous head dominated by a\n\
pink mouth, lined with rows of fangs."
);
FLAGS(MF_SEES, MF_SMELLS, MF_HEARS, MF_BASHES, MF_DESTROYS, MF_POISON, MF_VENOM,
      MF_DIGS);

mon("gelatinous blob",species_nether, 'O',c_ltgray,	MS_LARGE,	LIQUID,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  5, 20,  0,100, 40,  8,  2,  3,  0,  0, 10,  0,  0,200, 4,
	&mdeath::melt,		&mattack::formblob, "\
A shapeless blob the size of a cow.  It\n\
oozes slowly across the ground, small\n\
chunks falling off of its sides."
);
FLAGS(MF_SMELLS, MF_HEARS, MF_PLASTIC, MF_NOHEAD);

mon("flaming eye",species_nether, 'E',	c_red,		MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  5, 27,  0,100, 90,  0,  0,  0,  0,  1,  3,  0,  0,300, 12,
	&mdeath::normal,	&mattack::stare, "\
An eyeball the size of an easy chair and\n\
covered in rolling blue flames. It floats\n\
through the air."
);
FLAGS(MF_SEES, MF_WARM, MF_FLIES, MF_FIREY, MF_NOHEAD);

mon("kreck",	species_nether, 'h',	c_ltred,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  9,  6,100,100,105,  6,  1,  3,  1,  5,  0,  5,  0, 35, 0,
	&mdeath::melt,		&mattack::none, "\
A small humanoid, the size of a dog, with\n\
twisted red flesh and a distended neck. It\n\
scampers across the ground, panting and\n\
grunting."
);
FLAGS(MF_SEES, MF_SMELLS, MF_HEARS, MF_WARM, MF_BASHES, MF_HIT_AND_RUN);

mon("blank body",species_nether, 'h',	c_white,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3,  5,  0,100, 80,  9,  1,  4,  0,  1,  0,  0,  0,100, 10,
	&mdeath::normal,	&mattack::shriek, "\
This looks like a human body, but its\n\
flesh is snow-white and its face has no\n\
features save for a perfectly round\n\
mouth."
);
FLAGS(MF_SMELLS, MF_HEARS, MF_WARM, MF_ANIMAL, MF_SUNDEATH);

mon("Gozu",	species_nether, 'G',	c_white,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 1,  20, 10,100, 80, 12,  2,  5,  0,  5,  0,  0,  0,400, 20,
	&mdeath::normal,	&mattack::fear_paralyze, "\
A beast with the body of a slightly-overweight\n\
man and the head of a cow.  It walks slowly,\n\
milky white drool dripping from its mouth,\n\
wearing only a pair of white underwear."
);
FLAGS(MF_SEES, MF_SMELLS, MF_HEARS, MF_WARM, MF_BASHES, MF_ANIMAL, MF_FUR);

mon("shadow",	species_nether,'S',	c_dkgray,	MS_TINY,	MNULL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0, 14,100,100, 90,  6,  1,  2,  0,  4,  0,  0,  0, 60, 20,
	&mdeath::melt,		&mattack::disappear, "\
A strange dark area in the area.  It whispers\n\
softly as it moves."
);
FLAGS(MF_SEES, MF_HEARS, MF_GOODHEARING, MF_SMELLS, MF_NOHEAD, MF_HARDTOSHOOT,
      MF_GRABS, MF_WEBWALK, MF_FLIES, MF_PLASTIC, MF_SUNDEATH, MF_ELECTRIC,
      MF_ACIDPROOF, MF_HIT_AND_RUN);

// The hub
mon("breather",	species_nether,'O',	c_pink,		MS_MEDIUM,	MNULL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  2,  0,  0,100,  0,  0,  0,  0,  0,  0,  0,  0,100, 6,
	&mdeath::kill_breathers,&mattack::breathe, "\
A strange, immobile pink goo.  It seems to\n\
be breathing slowly."
);
FLAGS(MF_IMMOBILE);

// The branches
mon("breather",	species_nether,'o',	c_pink,		MS_MEDIUM,	MNULL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  2,  0,  0,100,  0,  0,  0,  0,  0,  0,  0,  0,100, 6,
	&mdeath::melt,		&mattack::breathe, "\
A strange, immobile pink goo.  It seems to\n\
be breathing slowly."
);
FLAGS(MF_IMMOBILE);

mon("shadow snake",species_none, 's',	c_dkgray,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  6,100,100, 90, 12,  1,  4,  5,  1,  0,  0,  0, 40, 20,
	&mdeath::melt,		&mattack::disappear, "\
A large snake, translucent black.");
FLAGS(MF_SEES, MF_SMELLS, MF_WARM, MF_SWIMS, MF_LEATHER, MF_PLASTIC,
      MF_SUNDEATH);

// ROBOTS
mon("eyebot",	species_robot, 'r',	c_ltblue,	MS_SMALL,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 20,  2,  0,100,120, 0,  0,  0,  0,  3,  10, 10, 70,  20, 30,
	&mdeath::normal,	&mattack::photograph, "\
A roughly spherical robot that hovers about\n\
five feet of the ground.  Its front side is\n\
dominated by a huge eye and a flash bulb.\n\
Frequently used for reconaissance."
);
FLAGS(MF_SEES, MF_FLIES, MF_ELECTRONIC, MF_NOHEAD);

mon("manhack",	species_robot, 'r',	c_green,	MS_TINY,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 18,  7,100,100,130, 12,  1,  1,  8,  4,  0,  6, 10, 15, 0,
	&mdeath::normal,	&mattack::none, "\
A fist-sized robot that flies swiftly through\n\
the air.  It's covered with whirring blades\n\
and has one small, glowing red eye."
);
FLAGS(MF_SEES, MF_FLIES, MF_NOHEAD, MF_ELECTRONIC, MF_HIT_AND_RUN);

mon("skitterbot",species_robot, 'r',	c_ltred,	MS_SMALL,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 10, 13,100,100,105,  0,  0,  0,  0,  0, 12, 12, 60, 40, 5,
	&mdeath::normal,	&mattack::tazer, "\
A robot with an insectoid design, about\n\
the size of a small dog.  It skitters\n\
quickly across the ground, two electric\n\
prods at the ready."
);
FLAGS(MF_SEES, MF_HEARS, MF_GOODHEARING, MF_ELECTRONIC);

mon("secubot",	species_robot, 'R',	c_dkgray,	MS_SMALL,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  7, 19,100,100, 70,  0,  0,  0,  0,  0, 14, 14, 80, 80, 2,
	&mdeath::explode,	&mattack::smg, "\
A boxy robot about four feet high.  It moves\n\
slowly on a set of treads, and is armed with\n\
a large machine gun type weapon.  It is\n\
heavily armored."
);
FLAGS(MF_SEES, MF_HEARS, MF_BASHES, MF_ATTACKMON, MF_ELECTRONIC);

mon("copbot",	species_robot, 'R',	c_dkgray,	MS_MEDIUM,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0, 12,100, 40,100,  4,  3,  2,  0,  8, 12,  8, 80, 80, 3,
	&mdeath::normal,	&mattack::copbot, "\
A blue-painted robot that moves quickly on a\n\
set of three omniwheels.  It has a nightstick\n\
readied, and appears to be well-armored."
);
FLAGS(MF_SEES, MF_HEARS, MF_BASHES, MF_ATTACKMON, MF_ELECTRONIC);

mon("molebot",	species_robot, 'R',	c_brown,	MS_MEDIUM,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2, 17,100,100, 40, 13,  1,  4, 10,  0, 14, 14, 82, 80, 0,
	&mdeath::normal,	&mattack::none,	"\
A snake-shaped robot that tunnels through the\n\
ground slowly.  When it emerges from the\n\
ground it can attack with its large, spike-\n\
covered head."
);
FLAGS(MF_HEARS, MF_GOODHEARING, MF_DIGS, MF_ELECTRONIC);

mon("tripod robot",species_robot, 'R',	c_white,	MS_LARGE,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  5, 26,100,100, 90, 15,  2,  4,  7,  0, 12,  8, 82, 80, 10,
	&mdeath::normal,	&mattack::flamethrower, "\
A 8-foot-tall robot that walks on three long\n\
legs.  It has a pair of spiked tentacles, as\n\
well as a flamethrower mounted on its head."
);
FLAGS(MF_SEES, MF_HEARS, MF_GOODHEARING, MF_BASHES, MF_ELECTRONIC);

mon("chicken walker",species_robot, 'R',c_red,		MS_LARGE,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3, 32,100,100,115, 0,  0,  0,  0,  0,  18, 14, 85, 90, 5,
	&mdeath::explode,	&mattack::smg, "\
A 10-foot-tall, heavily-armored robot that\n\
walks on a pair of legs with the knees\n\
facing backwards.  It's armed with a\n\
nasty-looking machine gun."
);
FLAGS(MF_SEES, MF_HEARS, MF_BASHES, MF_ELECTRONIC);

mon("tankbot",	species_robot, 'R',	c_blue,		MS_HUGE,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 52,100,100,100, 0,  0,  0,  0,  0,  22, 20, 92,240, 4,
	&mdeath::normal,	&mattack::multi_robot, "\
This fearsome robot is essentially an\n\
autonomous tank.  It moves surprisingly fast\n\
on its treads, and is armed with a variety of\n\
deadly weapons."
);
FLAGS(MF_SEES, MF_HEARS, MF_GOODHEARING, MF_NOHEAD, MF_BASHES, MF_DESTROYS,
      MF_ATTACKMON, MF_ELECTRONIC);

mon("turret",	species_robot, 't',	c_ltgray,	MS_SMALL,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0, 14,100,100,100,  0,  0,  0,  0,  0, 14, 16, 88, 30, 1,
	&mdeath::explode,	&mattack::smg, "\
A small, round turret which extends from\n\
the floor.  Two SMG barrels swivel 360\n\
degrees."
);
FLAGS(MF_SEES, MF_NOHEAD, MF_ELECTRONIC, MF_IMMOBILE, MF_FRIENDLY_SPECIAL);

mon("exploder",	species_robot, 'm',	c_ltgray,	MS_LARGE,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0, 14,100,100,100,  0,  0,  0,  0,  0,  0,  0, 88,  1, 1,
	&mdeath::explode,	&mattack::none, "\
A small, round turret which extends from\n\
the floor.  Two SMG barrels swivel 360\n\
degrees."
);
FLAGS(MF_IMMOBILE);


// HALLUCINATIONS
mon("zombie",	species_hallu, 'Z',	c_ltgreen,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  0,100,100, 65,  3,  0,  0,  0,  0,  0,  0,  0,  1,  20,
	&mdeath::disappear,	&mattack::disappear, "\
A human body, stumbling slowly forward on\n\
uncertain legs, possessed with an\n\
unstoppable rage."
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_STUMBLES);

mon("giant bee",species_hallu, 'a',	c_yellow,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  0,100,100,180,  2,  0,  0,  0,  0,  0,  0,  0,  1,  20,
	&mdeath::disappear,	&mattack::disappear, "\
A honey bee the size of a small dog. It\n\
buzzes angrily through the air, dagger-\n\
sized sting pointed forward."
);
FLAGS(MF_SMELLS, MF_FLIES);

mon("giant ant",species_hallu, 'a',	c_brown,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  0,100,100,100,  3,  0,  0,  0,  0,  0,  0,  0,  1,  20,
	&mdeath::disappear,	&mattack::disappear, "\
A red ant the size of a crocodile. It is\n\
covered in chitinous armor, and has a\n\
pair of vicious mandibles."
);
FLAGS(MF_SMELLS);

mon("your mother",species_hallu, '@',	c_white,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  0,100,100,100,  3,  0,  0,  0,  0,  0,  0,  0,  5,  20,
	&mdeath::disappear,	&mattack::disappear, "\
Mom?"
);
FLAGS(MF_SEES, MF_HEARS, MF_SMELLS, MF_GUILT);

mon("generator", species_none, 'G',	c_white,	MS_LARGE,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  0,  0,  0,100,  0,  0,  0,  0,  0,  2,  2,  0,500, 1,
	&mdeath::gameover,	&mattack::generator, "\
Your precious generator, noisily humming\n\
away.  Defend it at all costs!"
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
   setvector(ret, MTRIG_HURT, MTRIG_FIRE, MTRIG_FRIEND_DIED, NULL);
   break;
  case species_insect:
   setvector(ret, MTRIG_HURT, MTRIG_FIRE, NULL);
   break;
  case species_worm:
   setvector(ret, MTRIG_HURT, NULL);
   break;
  case species_zombie:
   break;
  case species_plant:
   setvector(ret, MTRIG_HURT, MTRIG_FIRE, NULL);
   break;
  case species_fungus:
   setvector(ret, MTRIG_HURT, MTRIG_FIRE, NULL);
   break;
  case species_nether:
   setvector(ret, MTRIG_HURT, NULL);
   break;
  case species_robot:
   break;
  case species_hallu:
   break;
 }
 return ret;
}



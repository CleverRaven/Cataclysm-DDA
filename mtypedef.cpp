#include "game.h"
#include "mondeath.h"
#include "monattack.h"
#include "itype.h"

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

#define mon(name, sym, color, size, mat, \
            flags, \
            freq, diff, agro, speed, melee_skill, melee_dice, melee_sides,\
            melee_cut, dodge, armor, item_chance, HP, sp_freq, death, sp_att,\
            desc)\
id++;\
mtypes.push_back(new mtype(id, name, sym, color, size, mat, flags, freq, diff,\
agro, speed, melee_skill, melee_dice, melee_sides, melee_cut, dodge, armor,\
item_chance, HP, sp_freq, death, sp_att, desc))

// PLEASE NOTE: The description is AT MAX 4 lines of 46 characters each.

// FOREST ANIMALS
mon("squirrel",		'r',	c_ltgray,	MS_TINY,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_ANIMAL)|
	 mfb(MF_WARM)|mfb(MF_FUR)),
//	frq dif  agr spd msk mdi m## cut dge arm itm  HP special freq
	 50,  0,  -5,140,  0,  1,  1,  0,  4,  0,  0,  1,  0,
	&mdeath::normal,	&mattack::none, "\
A small woodland animal."
);

mon("rabbit",		'r',	c_white,	MS_TINY,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_ANIMAL)|
	 mfb(MF_WARM)|mfb(MF_FUR)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 10,  0, -5, 180, 0,  0,  0,  0,  6,  0,  0,  4,  0,
	&mdeath::normal,	&mattack::none, "\
A cute wiggling nose, cotton tail, and\n\
delicious flesh."
);

mon("deer",		'd',	c_brown,	MS_LARGE,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_ANIMAL)|
	 mfb(MF_WARM)|mfb(MF_FUR)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 3,  1, -2, 300, 4,  3,  3,  0,  3,  0,  0,  80, 0,
	&mdeath::normal,	&mattack::none, "\
A large buck, fast-moving and strong."
);

mon("wolf",		'w',	c_dkgray,	MS_MEDIUM,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_ANIMAL)|mfb(MF_WARM)|
	 mfb(MF_FUR)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  4, 12,  3,165, 14,  2,  3,  4,  4,  0,  0, 28,  0,
	&mdeath::normal,	&mattack::none, "\
A vicious and fast pack hunter."
);

mon("bear",		'B',	c_dkgray,	MS_LARGE,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_ANIMAL)|mfb(MF_WARM)|
	 mfb(MF_FUR)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  2, 10,  4,140, 10,  3,  4,  6,  3,  0,  0, 90, 0,
	&mdeath::normal,	&mattack::none, "\
Remember, only YOU can prevent forest fires."
);

// DOMESICATED ANIMALS
mon("dog",		'd',	c_white,	MS_SMALL,	FLESH,
	(mfb(MF_SEES)|mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_ANIMAL)|mfb(MF_WARM)|
	 mfb(MF_FUR)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 3,  5,  1,150, 12,  2,  3,  3,  3,  0,  0, 25,  0,
	&mdeath::normal,	&mattack::none, "\
A medium-sized domesticated dog, gone feral."
);

// INSECTOIDS
mon("ant larva",	'a',	c_white,	MS_SMALL,	FLESH,
	(mfb(MF_SMELLS)|mfb(MF_POISON)|mfb(MF_ANIMAL)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  1,  0,  1, 65,  4,  1,  3,  0,  0,  0,  0, 10,  0,
	&mdeath::normal,	&mattack::none, "\
The size of a large cat, this pulsating mass\n\
of glistening white flesh turns your stomach."
);

mon("giant ant",	'a',	c_brown,	MS_MEDIUM,	FLESH,
	(mfb(MF_SMELLS)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 20,  7,  3,100,  9,  1,  6,  4,  2,  8,-40, 40,  0,
	&mdeath::normal,	&mattack::none, "\
A red ant the size of a crocodile. It is\n\
covered in chitinous armor, and has a pair of\n\
vicious mandibles."
);

mon("soldier ant",	'a',	c_blue,		MS_MEDIUM,	FLESH,
	(mfb(MF_SMELLS)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  2, 16,  5,115, 12,  2,  4,  6,  2, 10,-50, 80,  0,
	&mdeath::normal,	&mattack::none, "\
Darker in color than the other ants, this\n\
more aggresive variety has even larger\n\
mandibles."
);

mon("queen ant",	'a',	c_ltred,	MS_LARGE,	FLESH,
	(mfb(MF_SMELLS)|mfb(MF_QUEEN)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  0, 13,  2, 60,  6,  3,  4,  4,  1, 14,-40,180, 5,
	&mdeath::normal,	&mattack::antqueen, "\
This ant has a long, bloated thorax, bulging\n\
with hundreds of small ant eggs.  It moves\n\
slowly, tending to nearby eggs and laying\n\
still more."
);

mon("fungal insect",	'a',	c_ltgray,	MS_MEDIUM,	VEGGY,
	(mfb(MF_SMELLS)|mfb(MF_POISON)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  0,  5,  5, 75,  5,  1,  5,  3,  1,  1,  0, 30, 60,
	&mdeath::normal,	&mattack::fungus, "\
This insect is pale gray in color, its\n\
chitin weakened by the fungus sprouting\n\
from every joint on its body."
);

mon("giant bee",	'a',	c_yellow,	MS_SMALL,	FLESH,
	(mfb(MF_SMELLS)|mfb(MF_VENOM)|mfb(MF_FLIES)|mfb(MF_STUMBLES)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  2, 15,  4,140,  4,  1,  1,  5,  6,  5,-50, 20,  0,
	&mdeath::normal,	&mattack::none, "\
A honey bee the size of a small dog. It\n\
buzzes angrily through the air, dagger-\n\
sized sting pointed forward."
);

mon("giant wasp",	'a', 	c_red,		MS_MEDIUM,	FLESH,
	(mfb(MF_SMELLS)|mfb(MF_POISON)|mfb(MF_VENOM)|mfb(MF_FLIES)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  2, 22,  5,150,  6,  1,  3,  7,  7,  7,-40, 35, 0,
	&mdeath::normal,	&mattack::none, "\
An evil-looking, slender-bodied wasp with\n\
a vicious sting on its abdomen."
);

// GIANT WORMS

mon("graboid",		'S',	c_red,		MS_HUGE,	FLESH,
	(mfb(MF_DIGS)|mfb(MF_HEARS)|mfb(MF_GOODHEARING)|mfb(MF_DESTROYS)|
	 mfb(MF_WARM)|mfb(MF_LEATHER)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  1, 17,  5,180, 11,  3,  8,  4,  0,  5,  0,180,  0,
	&mdeath::worm,		&mattack::none, "\
A hideous slithering beast with a tri-\n\
sectional mouth that opens to reveal\n\
hundreds of writhing tongues. Most of its\n\
enormous body is hidden underground."
);

mon("giant worm",	'S',	c_pink,		MS_LARGE,	FLESH,
	(mfb(MF_DIGS)|mfb(MF_HEARS)|mfb(MF_GOODHEARING)|mfb(MF_WARM)|
	 mfb(MF_LEATHER)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 30, 10,  3, 85,  9,  4,  5,  2,  0,  0,  0, 50,  0,
	&mdeath::worm,		&mattack::none, "\
Half of this monster is emerging from a\n\
hole in the ground. It looks like a huge\n\
earthworm, but the end has split into a\n\
large, fanged mouth."
);

mon("half worm",	's',	c_pink,		MS_MEDIUM,	FLESH,
	(mfb(MF_DIGS)|mfb(MF_HEARS)|mfb(MF_GOODHEARING)|mfb(MF_WARM)|
	 mfb(MF_LEATHER)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  0,  2,  2, 80,  5,  3,  5,  0,  0,  0,  0, 20,  0,
	&mdeath::normal,	&mattack::none, "\
A portion of a giant worm that is still alive."
);

// ZOMBIES
mon("zombie",		'Z',	c_ltgreen,	MS_MEDIUM,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_STUMBLES)|
         mfb(MF_WARM)|mfb(MF_BASHES)|mfb(MF_POISON)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 90,  3,  5, 70,  8,  1,  5,  0,  1,  0, 40, 50,  0,
	&mdeath::normal,	&mattack::none, "\
A human body, stumbling slowly forward on\n\
uncertain legs, possessed with an unstoppable\n\
rage."
);

mon("shrieker zombie",	'Z',	c_magenta,	MS_MEDIUM,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_STUMBLES)|
	 mfb(MF_WARM)|mfb(MF_BASHES)|mfb(MF_POISON)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  4, 10,  4, 95,  9,  1,  2,  0,  4,  0, 45, 50, 10,
	&mdeath::normal,	&mattack::shriek, "\
This zombie's jaw has been torn off, leaving\n\
a gaping hole from mid-neck up."
);

mon("spitter zombie",	'Z',	c_yellow,	MS_MEDIUM,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_WARM)|mfb(MF_BASHES)|
	 mfb(MF_POISON)|mfb(MF_ACIDPROOF)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  4,  9,  5, 105, 8,  1,  5,  0,  4,  0, 30, 60, 20,
	&mdeath::acid,		&mattack::acid,	"\
This zombie's mouth is deformed into a round\n\
spitter, and its body throbs with a dense\n\
yellow fluid."
);

mon("shocker zombie",	'Z',	c_ltcyan,	MS_MEDIUM,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_WARM)|mfb(MF_BASHES)|
	 mfb(MF_POISON)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  3, 10,  5,110,  8,  1,  6,  0,  4,  0, 40, 65, 25,
	&mdeath::normal,	&mattack::shockstorm, "\
This zombie's flesh is pale blue, and it\n\
occasionally crackles with small bolts of\n\
lightning."
);

mon("fast zombie",	'Z',	c_ltred,	MS_MEDIUM,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_STUMBLES)|
	 mfb(MF_WARM)|mfb(MF_BASHES)|mfb(MF_POISON)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  6, 12,  5,150, 10,  1,  4,  3,  4,  0, 45, 40,  0,
	&mdeath::normal,	&mattack::none, "\
This deformed, sinewy zombie stays close to\n\
the ground, loping forward faster than most\n\
humans ever could."
);

mon("zombie brute",	'Z',	c_red,		MS_MEDIUM,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_WARM)|mfb(MF_BASHES)|
	 mfb(MF_POISON)|mfb(MF_LEATHER)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  4, 25,  5,115,  9,  4,  4,  2,  0,  8, 60, 80,  0,
	&mdeath::normal,	&mattack::none, "\
A hideous beast of a zombie, bulging with\n\
distended muscles on both arms and legs."
);

mon("zombie hulk",	'Z',	c_blue,		MS_HUGE,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_WARM)|mfb(MF_BASHES)|
	 mfb(MF_DESTROYS)|mfb(MF_POISON)|mfb(MF_ATTACKMON)|mfb(MF_LEATHER)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  1, 50,  5,130, 9,  4,  8,  0,  0,  12, 80,260,  0,
	&mdeath::normal,	&mattack::none, "\
A zombie that has somehow grown to the size of\n\
6 men, with arms as wide as a trash can."
);

mon("fungal zombie",	'Z',	c_ltgray,	MS_MEDIUM,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_STUMBLES)|
	 mfb(MF_WARM)|mfb(MF_BASHES)|mfb(MF_POISON)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  2,  6,  5, 45,  6,  1,  6,  0,  0,  0, 20, 40, 50,
	&mdeath::normal,	&mattack::fungus, "\
A disease zombie. Fungus sprouts from its\n\
mouth and eyes, and thick gray mold grows all\n\
over its translucent flesh."
);

mon("boomer",		'Z',	c_pink,		MS_LARGE,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_STUMBLES)|mfb(MF_WARM)|
	 mfb(MF_BASHES)|mfb(MF_POISON)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  6,  5,  5, 55,  7,  2,  4,  0,  1,  0, 35, 40,  20,
	&mdeath::boomer,	&mattack::boomer, "\
A bloated zombie sagging with fat. It emits a\n\
horrible odor, and putrid, pink sludge drips\n\
from its mouth."
);

mon("fungal boomer",	'B',	c_ltgray,	MS_LARGE,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_STUMBLES)|mfb(MF_WARM)|
	 mfb(MF_BASHES)|mfb(MF_POISON)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  1,  7,  5, 40,  5,  2,  6,  0,  0,  0, 20, 20, 30,
	&mdeath::fungus,	&mattack::fungus, "\
A bloated zombie that is coated with slimy\n\
gray mold. Its flesh is translucent and gray,\n\
and it dribbles a gray sludge from its mouth."
);

mon("skeleton",		'Z',	c_white,	MS_MEDIUM,	STONE,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_HARDTOSHOOT)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  7,  8,  5, 90, 10,  1,  5,  3,  2, 15,  0, 40, 0,
	&mdeath::normal,	&mattack::none, "\
A skeleton picked clean of all but a few\n\
rotten scraps of flesh, somehow still in\n\
motion."
);

mon("zombie necromancer",'Z',	c_dkgray,	MS_MEDIUM,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_WARM)|mfb(MF_BASHES)|
	 mfb(MF_POISON)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  3, 13,  5,100,  4,  2,  3,  0,  4,  0, 50,140, 4,
	&mdeath::normal,	&mattack::resurrect, "\
A zombie with jet black skin and glowing red\n\
eyes.  As you look at it, you're gripped by a\n\
feeling of dread and terror."
);

mon("zombie scientist", 'Z',	c_ltgray,	MS_MEDIUM,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_WARM)|mfb(MF_BASHES)|
	 mfb(MF_POISON)|mfb(MF_ACIDPROOF)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 20,  3,  5, 75,  7,  1,  3,  0,  1,  0, 50, 35, 20,
	&mdeath::normal,	&mattack::science, "\
A zombie wearing a tattered lab coat and\n\
some sort of utility belt.  It looks weaker\n\
than most zombies, but more resourceful too."
);

// PLANTS & FUNGI
mon("triffid",		'F',	c_ltgreen,	MS_MEDIUM,	VEGGY,
	(mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_BASHES)|mfb(MF_NOHEAD)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
 	 24, 16,  4, 75,  9,  2,  4,  5,  0, 12,  0, 80,  0,
	&mdeath::normal,	&mattack::none, "\
A plant that grows as high as your head,\n\
with one thick, bark-coated stalk\n\
supporting a flower-like head with a sharp\n\
sting within."
);

mon("young triffid",	'f',	c_ltgreen,	MS_SMALL,	VEGGY,
	(mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_NOHEAD)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 15,  2,  1, 65,  7,  1,  4,  3,  0,  0,  0, 40,  0,
	&mdeath::normal,	&mattack::none, "\
A small triffid, only a few feet tall. It\n\
has not yet developed bark, but its sting\n\
is still sharp and deadly."
);

mon("queen triffid",	'F',	c_red,		MS_LARGE,	VEGGY,
	(mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_NOHEAD)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  3, 22,  5, 85, 14,  2,  7,  8,  0, 14,  0,280, 2,
	&mdeath::normal,	&mattack::growplants, "\
A very large triffid, with a particularly\n\
vicious sting and thick bark.  As it\n\
moves, plant matter drops off its body\n\
and immediately takes root."
);

mon("creeper hub",	'V',	c_dkgray,	MS_MEDIUM,	VEGGY,
	(mfb(MF_NOHEAD)|mfb(MF_IMMOBILE)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  0, 16,  5,100,  0,  0,  0,  0,  0,  8,  0,100, 2,
	&mdeath::kill_vines,	&mattack::grow_vine, "\
A thick stalk, rooted to the ground.\n\
It rapidly sprouts thorny vines in all\n\
directions."
);

mon("creeping vine",	'v',	c_green,	MS_TINY,	VEGGY,
	(mfb(MF_NOHEAD)|mfb(MF_HARDTOSHOOT)|mfb(MF_PLASTIC)|mfb(MF_IMMOBILE)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  0,  4,  5, 75,  0,  0,  0,  0,  0,  2,  0, 20, 2,
	&mdeath::vine_cut,	&mattack::vine, "\
A thorny vine.  It twists wildly as\n\
it grows, spreading rapidly."
);

mon("biollante",	'F',	c_magenta,	MS_LARGE,	VEGGY,
	(mfb(MF_NOHEAD)|mfb(MF_IMMOBILE)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  0, 20,  5,100,  0,  0,  0,  0,  0,  0,-80,120, 2,
	&mdeath::normal,	&mattack::spit_sap, "\
A thick stalk topped with a purple\n\
flower.  The flower's petals are closed,\n\
and pulsate ominously."
);

mon("triffid heart",	'T',	c_red,		MS_HUGE,	VEGGY,
	(mfb(MF_NOHEAD)|mfb(MF_IMMOBILE)|mfb(MF_QUEEN)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
          0, 45,  5,100,  0,  0,  0,  0,  0, 14,  0,300, 5,
	&mdeath::triffid_heart,	&mattack::triffid_heartbeat, "\
A knot of roots that looks bizarrely like a\n\
heart.  It beats slowly with sap, powering\n\
the root walls around it."
);

mon("fungaloid",	'F',	c_ltgray,	MS_MEDIUM,	VEGGY,
	(mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_POISON)|mfb(MF_NOHEAD)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 12, 12,  4, 45,  8,  3,  3,  0,  0,  0,  0, 80, 200,
	&mdeath::fungus,	&mattack::fungus, "\
A pale white fungus, one meaty gray stalk\n\
supporting a bloom at the top. A few\n\
tendrils extend from the base, allowing\n\
mobility and a weak attack."
);

// This is a "dormant" fungaloid that doesn't waste CPU cycles ;)
mon("fungaloid",	'F',	c_ltgray,	MS_MEDIUM,	VEGGY,
	(mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_POISON)|mfb(MF_NOHEAD)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  0,  0,  4,  1,  8,  3,  5,  0,  0,  0,  0,  1, 0,
	&mdeath::fungusawake,	&mattack::none, "\
A pale white fungus, one meaty gray stalk\n\
supporting a bloom at the top. A few\n\
tendrils extend from the base, allowing\n\
mobility and a weak attack."
);

mon("young fungaloid",	'f',	c_ltgray,	MS_SMALL,	VEGGY,
	(mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_POISON)|mfb(MF_NOHEAD)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  6,  6,  4, 65,  8,  1,  4,  6,  0,  4,  0, 70, 0,
	&mdeath::normal,	&mattack::none, "\
A fungal tendril just a couple feet tall.  Its\n\
exterior is hardened into a leathery bark and\n\
covered in thorns; it also moves faster than\n\
full-grown fungaloids."
);

mon("spore",		'o',	c_ltgray,	MS_TINY,	VEGGY,
	(mfb(MF_STUMBLES)|mfb(MF_POISON)|mfb(MF_NOHEAD)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  0,  1, -5,100,  0,  0,  0,  0,  6,  0,  0,  5, 50,
	&mdeath::disintegrate,	&mattack::plant, "\
A wispy spore, about the size of a fist,\n\
wafting on the breeze."
);

mon("fungal spire",	'T',	c_ltgray,	MS_HUGE,	STONE,
	mfb(MF_NOHEAD)|mfb(MF_POISON)|mfb(MF_IMMOBILE)|mfb(MF_QUEEN),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  0, 40,  5,100,  0,  0,  0,  0,  0, 10,  0,300, 5,
	&mdeath::fungus,	&mattack::fungus_sprout, "\
An enormous fungal spire, towering 30 feet\n\
above the ground.  It pulsates slowly,\n\
continuously growing new defenses."
);

mon("fungal wall",	'F',	c_dkgray,	MS_HUGE,	VEGGY,
	mfb(MF_NOHEAD)|mfb(MF_POISON)|mfb(MF_IMMOBILE),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  0,  5,  0,100,  0,  0,  0,  0,  0, 10,  0, 60, 8,
	&mdeath::disintegrate,	&mattack::fungus, "\
A veritable wall of fungus, grown as a\n\
natural defense by the fungal spire. It\n\
looks very tough, and spews spores at an\n\
alarming rate."
);

// BLOBS & SLIMES &c
mon("blob",		'O',	c_dkgray,	MS_MEDIUM,	LIQUID,
	(mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_GOODHEARING)|mfb(MF_NOHEAD)|
	 mfb(MF_POISON)|mfb(MF_ACIDPROOF)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 10, 19,  5, 85,  9,  2,  4,  0,  0,  0,  0, 85, 30,
	&mdeath::blobsplit,	&mattack::formblob, "\
A black blob of viscous goo that oozes\n\
across the ground like a mass of living\n\
oil."
);

mon("small blob",	'o',	c_dkgray,	MS_SMALL,	LIQUID,
	(mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_GOODHEARING)|mfb(MF_NOHEAD)|
	 mfb(MF_POISON)|mfb(MF_ACIDPROOF)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  1,  2,  5, 50,  6,  1,  4,  0,  0,  0,  0, 50, 0,
	&mdeath::blobsplit,	&mattack::none, "\
A small blob of viscous goo that oozes\n\
across the ground like a mass of living\n\
oil."
);

// CHUDS & SUBWAY DWELLERS
mon("C.H.U.D.",		'S',	c_ltgray,	MS_MEDIUM,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_WARM)|mfb(MF_BASHES)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 50,  8,  4,110, 10,  1,  5,  0,  3,  0, 25, 60, 0,
	&mdeath::normal,	&mattack::none,	"\
Cannibalistic Humanoid Underground Dweller.\n\
A human, turned pale and mad from years in\n\
the subways."
);

mon("one-eyed mutant",	'S',	c_ltred,	MS_MEDIUM,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_WARM)|mfb(MF_BASHES)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  5, 18,  3,130, 20,  2,  4,  0,  5,  0, 40, 80, 0,
	&mdeath::normal,	&mattack::none,	"\
A relatively humanoid mutant with purple\n\
hair and a grapefruit-sized bloodshot eye."
);

mon("crawler mutant",	'S',	c_red,		MS_LARGE,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_WARM)|
	 mfb(MF_BASHES)|mfb(MF_POISON)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  2, 16,  5, 80, 10,  2,  6,  0,  1,  8,  0,180, 0,
	&mdeath::normal,	&mattack::none,	"\
Two or three humans fused together somehow,\n\
slowly dragging their thick-hided, hideous\n\
body across the ground."
);

mon("sewer fish",	's',	c_ltgreen,	MS_SMALL,	FLESH,
	(mfb(MF_SEES)|mfb(MF_SMELLS)|mfb(MF_WARM)|mfb(MF_AQUATIC)|
	 mfb(MF_ANIMAL)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 30, 13,  5,120, 17,  1,  3,  3,  6,  0,  0, 20, 0,
	&mdeath::normal,	&mattack::none,	"\
A large green fish, it's mouth lined with\n\
three rows of razor-sharp teeth."
);

mon("sewer snake",	's',	c_yellow,	MS_SMALL,	FLESH,
	(mfb(MF_SEES)|mfb(MF_SMELLS)|mfb(MF_WARM)|mfb(MF_VENOM)|mfb(MF_SWIMS)|
	 mfb(MF_ANIMAL)|mfb(MF_LEATHER)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 15,  2,  5, 60, 12,  1,  2,  5,  1,  0,  0, 40, 0,
	&mdeath::normal,	&mattack::none,	"\
A large snake, turned pale yellow from its\n\
underground life."
);

mon("sewer rat",	's',	c_dkgray,	MS_SMALL,	FLESH,
	(mfb(MF_SEES)|mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_WARM)|mfb(MF_SWIMS)|
	 mfb(MF_ANIMAL)|mfb(MF_FUR)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 18,  3,  4,105, 10,  1,  2,  1,  2,  0,  0, 30, 0,
	&mdeath::normal,	&mattack::none, "\
A large, mangey rat with red eyes.  It\n\
scampers quickly across the ground, squeaking\n\
hungrily."
);

// SWAMP CREATURES
mon("giant mosquito",	'y',	c_ltgray,	MS_SMALL,	FLESH,
	(mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_STUMBLES)|mfb(MF_VENOM)|
	 mfb(MF_FLIES)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 22, 12,  3,120,  8,  1,  1,  1,  5,  0,  0, 20, 0,
	&mdeath::normal,	&mattack::none, "\
An enormous mosquito, fluttering erratically,\n\
its face dominated by a long, spear-tipped\n\
proboscis."
);

mon("giant dragonfly",	'y',	c_ltgreen,	MS_SMALL,	FLESH,
	(mfb(MF_SEES)|mfb(MF_SMELLS)|mfb(MF_FLIES)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  6, 13,  4,155, 12,  1,  3,  6,  5,  6,-20, 70, 0,
	&mdeath::normal,	&mattack::none, "\
A ferocious airborne predator, flying swiftly\n\
through the air, its mouth a cluster of fangs."
);

mon("giant centipede",	'a',	c_ltgreen,	MS_MEDIUM,	FLESH,
	(mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_POISON)|mfb(MF_VENOM)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  7,  9,  4,120, 10,  1,  3,  5,  2,  8,-30, 60, 0,
	&mdeath::normal,	&mattack::none, "\
A meter-long centipede, moving swiftly on\n\
dozens of thin legs, a pair of venomous\n\
pincers attached to its head."
);

mon("giant frog",	'F',	c_green,	MS_LARGE,	FLESH,
	(mfb(MF_SEES)|mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_SWIMS)|
	 mfb(MF_LEATHER)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  5, 10,  3, 90,  8,  2,  3,  0,  2,  4,  0, 70, 5,
	&mdeath::normal,	&mattack::leap, "\
A thick-skinned green frog.  It eyes you\n\
much as you imagine it might eye an insect."
);

mon("giant slug",	'S',	c_yellow,	MS_HUGE,	FLESH,
	(mfb(MF_SEES)|mfb(MF_SMELLS)|mfb(MF_BASHES)|mfb(MF_ACIDPROOF)|
	 mfb(MF_ACIDTRAIL)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  4, 16,  4, 60,  7,  1,  5,  1,  0,  5,  0,190, 10,
	&mdeath::normal,	&mattack::acid, "\
A gigantic slug, the size of a small car.\n\
It moves slowly, dribbling acidic goo from\n\
its fang-lined mouth."
);

mon("dermatik larva",	'i',	c_white,	MS_TINY,	FLESH,
	(mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_POISON)|mfb(MF_DIGS)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  0,  4, -1, 20,  1,  1,  2,  2,  1,  0,  0, 10, 0,
	&mdeath::normal,	&mattack::none, "\
A fat, white grub the size of your foot, with\n\
a set of mandibles that look more suited for\n\
digging than fighting."
);

mon("dermatik",		'i',	c_red,		MS_TINY,	FLESH,
	(mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_STUMBLES)|mfb(MF_POISON)|
	 mfb(MF_FLIES)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  3, 18,  4,100,  5,  1,  1,  6, 7,   6,  0, 60, 50,
	&mdeath::normal,	&mattack::dermatik, "\
A wasp-like flying insect, smaller than most\n\
mutated wasps.  It does not looke very\n\
threatening, but has a large ovipositor in\n\
place of a sting."
);


// SPIDERS
mon("wolf spider",	's',	c_brown,	MS_MEDIUM,	FLESH,
	(mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_VENOM)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  1, 20,  4,110, 7,   1,  1,  8,  6,  8,-70, 40, 0,
	&mdeath::normal,	&mattack::none, "\
A large, brown spider, which moves quickly\n\
and aggresively."
);

mon("web spider",	's',	c_yellow,	MS_SMALL,	FLESH,
	(mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_VENOM)|mfb(MF_WEBWALK)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  1, 16,  3,120,  5,  1,  1,  7,  5,  7,-70, 35, 0,
	&mdeath::normal,	&mattack::none, "\
A yellow spider the size of a dog.  It lives\n\
in webs, waiting for prey to become\n\
entangled before pouncing and biting."
);

mon("jumping spider",	's',	c_white,	MS_SMALL,	FLESH,
	(mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_VENOM)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  2, 14,  4,100,  7,  1,  1,  4,  8,  3,-60, 30, 2,
	&mdeath::normal,	&mattack::leap, "\
A small, almost cute-looking spider.  It\n\
leaps so quickly that it almost appears to\n\
instantaneously move from one place to\n\
another."
);

mon("trap door spider",	's',	c_blue,		MS_MEDIUM,	FLESH,
	(mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_VENOM)|mfb(MF_WEBWALK)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  1, 20,  5,110,  5,  1,  2,  7,  3,  8,-80, 70, 0,
	&mdeath::normal,	&mattack::none, "\
A large spider with a bulbous thorax.  It\n\
creates a subterranean nest and lies in\n\
wait for prey to fall in and become trapped\n\
in its webs."
);

mon("black widow",	's',	c_dkgray,	MS_SMALL,	FLESH,
	(mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_BADVENOM)|mfb(MF_WEBWALK)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  1, 20,  3, 90,  6,  1,  1,  6,  3,  3,-50, 40, 0,
	&mdeath::normal,	&mattack::none, "\
A spider with a characteristic red\n\
hourglass on its black carapace.  It is\n\
known for its highly toxic venom."
);

// UNEARTHED HORRORS
mon("dark wyrm",	'S',	c_blue,		MS_LARGE,	FLESH,
	(mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_GOODHEARING)|mfb(MF_DESTROYS)|
	 mfb(MF_POISON)|mfb(MF_SUNDEATH)|mfb(MF_ACIDPROOF)|mfb(MF_ACIDTRAIL)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  1, 20,  5, 90,  8,  2,  6,  4,  4,  0,  0, 80, 0,
	&mdeath::normal,	&mattack::none, "\
A huge, black worm, its flesh glistening\n\
with an acidic, blue slime.  It has a gaping\n\
round mouth lined with dagger-like teeth."
);

mon("Amigara horror",	'H',	c_white,	MS_LARGE,	FLESH,
	(mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_SEES)|mfb(MF_STUMBLES)|
	 mfb(MF_HARDTOSHOOT)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  1, 30,  5, 70, 10,  2,  4,  0,  2,  0,  0,250, 0,
	&mdeath::amigara,	&mattack::fear_paralyze, "\
A spindly body, standing at least 15 feet\n\
tall.  It looks vaguely human, but its face is\n\
grotesquely stretched out, and its limbs are\n\
distorted to the point of being tentacles."
);

// This "dog" is, in fact, a disguised Thing
mon("dog",		'd',	c_white,	MS_SMALL,	FLESH,
	(mfb(MF_SEES)|mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_ANIMAL)|mfb(MF_WARM)|
	 mfb(MF_FUR)|mfb(MF_FRIENDLY_SPECIAL)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  3,  5,  1,150, 12,  2,  3,  3,  3,  0,  0, 25, 40,
	&mdeath::thing,		&mattack::dogthing, "\
A medium-sized domesticated dog, gone feral."
);

mon("tentacle dog",	'd',	c_dkgray,	MS_MEDIUM,	FLESH,
	(mfb(MF_SEES)|mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_BASHES)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  1, 14,  5,120, 12,  2,  4,  0,  3,  0,  0,120, 5,
	&mdeath::thing,		&mattack::tentacle, "\
A dog's body with a mass of ropy, black\n\
tentacles extending from its head."
);

mon("Thing",		'H',	c_dkgray,	MS_LARGE,	FLESH,
	(mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_NOHEAD)|mfb(MF_BASHES)|
	 mfb(MF_SWIMS)|mfb(MF_ATTACKMON)|mfb(MF_PLASTIC)|mfb(MF_ACIDPROOF)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  1, 25,  5,135, 14,  2,  4,  0,  5,  0,  0,160, 5,
	&mdeath::melt,		&mattack::tentacle, "\
An amorphous black creature which seems to\n\
sprout tentacles rapidly."
);

mon("human snail",	'h',	c_green,	MS_LARGE,	FLESH,
	(mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_POISON)|mfb(MF_ACIDPROOF)|
	 mfb(MF_ACIDTRAIL)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 20, 10,  3, 50,  4,  1,  5,  0,  0, 10,  0, 50, 15,
	&mdeath::normal,	&mattack::acid, "\
A large snail, with an oddly human face."
);

mon("twisted body",	'h',	c_pink,		MS_MEDIUM,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_GOODHEARING)|mfb(MF_POISON)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  5, 12,  5, 90,  5,  2,  4,  0,  6,  0,  0, 65, 0,
	&mdeath::normal,	&mattack::none, "\
A human body, but with its limbs, neck, and\n\
hair impossibly twisted."
);

mon("vortex",		'v',	c_white,	MS_SMALL,	POWDER,
	(mfb(MF_HEARS)|mfb(MF_GOODHEARING)|mfb(MF_STUMBLES)|mfb(MF_NOHEAD)|
	 mfb(MF_HARDTOSHOOT)|mfb(MF_FLIES)|mfb(MF_PLASTIC)|
	 mfb(MF_FRIENDLY_SPECIAL)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  2, 30,  5,120,  0,  0,  0,  0,  0,  0,  0, 20, 6,
	&mdeath::melt,		&mattack::vortex, "\
A twisting spot in the air, with some kind\n\
of morphing mass at its center."
);

// NETHER WORLD INHABITANTS
mon("flying polyp",	'H',	c_dkgray,	MS_HUGE,	FLESH,
	(mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_GOODHEARING)|mfb(MF_NOHEAD)|
	 mfb(MF_BASHES)|mfb(MF_FLIES)|mfb(MF_ATTACKMON)|mfb(MF_PLASTIC)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  1, 42,  5,280, 16,  3,  8,  6,  7,  8,  0,350, 0,
	&mdeath::melt,		&mattack::none, "\
An amorphous mass of twisting black flesh\n\
that flies through the air swiftly."
);

mon("hunting horror",	'h',	c_dkgray,	MS_SMALL,	MNULL,
	(mfb(MF_SEES)|mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_NOHEAD)|
	 mfb(MF_HARDTOSHOOT)|mfb(MF_FLIES)|mfb(MF_PLASTIC)|mfb(MF_SUNDEATH)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 10, 28,  4,180, 15,  3,  4,  0,  6,  0,  0, 80, 0,
	&mdeath::melt,		&mattack::none, "\
A ropy, worm-like creature that flies on\n\
bat-like wings. Its form continually\n\
shifts and changes, twitching and\n\
writhing."
);

mon("Mi-go",		'H',	c_pink,		MS_MEDIUM,	FLESH,
	(mfb(MF_SEES)|mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_WARM)|
	 mfb(MF_BASHES)|mfb(MF_POISON)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  5, 26,  3,120, 14,  5,  3, 10,  7, 12,  0,110, 0,
	&mdeath::normal,	&mattack::none, "\
A pinkish, fungoid crustacean-like\n\
creature with numerous pairs of clawed\n\
appendages and a head covered with waving\n\
antennae."
);

mon("yugg",		'H',	c_white,	MS_HUGE,	FLESH,
	(mfb(MF_SEES)|mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_BASHES)|
	 mfb(MF_DESTROYS)|mfb(MF_POISON)|mfb(MF_VENOM)|mfb(MF_DIGS)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  3, 32,  4, 80, 12,  3,  5,  8,  1,  0,  0,320, 20,
	&mdeath::normal,	&mattack::gene_sting, "\
An enormous white flatworm writhing\n\
beneath the earth. Poking from the\n\
ground is a bulbous head dominated by a\n\
pink mouth, lined with rows of fangs."
);

mon("gelatinous blob", 'O',	c_ltgray,	MS_LARGE,	LIQUID,
	(mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_PLASTIC)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  5, 20,  2, 40,  8,  2,  3,  0,  0,  0,  0,200, 4,
	&mdeath::melt,		&mattack::formblob, "\
A shapeless blob the size of a cow.  It\n\
oozes slowly across the ground, small\n\
chunks falling off of its sides."
);

mon("flaming eye",	'E',	c_red,		MS_MEDIUM,	FLESH,
	(mfb(MF_SEES)|mfb(MF_WARM)|mfb(MF_FLIES)|mfb(MF_FIREY)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  5, 27,  4, 90,  0,  0,  0,  0,  1,  0,  0,300, 12,
	&mdeath::normal,	&mattack::stare, "\
An eyeball the size of an easy chair and\n\
covered in rolling blue flames. It floats\n\
through the air."
);

mon("kreck",		'h',	c_ltred,	MS_SMALL,	FLESH,
	(mfb(MF_SEES)|mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_WARM)|mfb(MF_BASHES)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  9,  5,  5,135,  6,  2,  2,  1,  5,  5,  0, 35, 0,
	&mdeath::melt,		&mattack::none, "\
A small humanoid, the size of a dog, with\n\
twisted red flesh and a distended neck. It\n\
scampers across the ground, panting and\n\
grunting."
);

mon("blank body",	'h',	c_white,	MS_MEDIUM,	FLESH,
	(mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_WARM)|mfb(MF_ANIMAL)|
	 mfb(MF_SUNDEATH)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  3,  5,  3, 80,  9,  1,  4,  0,  1,  0,  0,100, 10,
	&mdeath::normal,	&mattack::shriek, "\
This looks like a human body, but its\n\
flesh is snow-white and its face has no\n\
features save for a perfectly round\n\
mouth."
);

mon("Gozu",		'G',	c_white,	MS_MEDIUM,	FLESH,
	(mfb(MF_SEES)|mfb(MF_SMELLS)|mfb(MF_HEARS)|mfb(MF_WARM)|
	 mfb(MF_BASHES)|mfb(MF_ANIMAL)|mfb(MF_FUR)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 1,  20,  1, 80, 12,  2,  5,  0,  5,  0,  0,400, 20,
	&mdeath::normal,	&mattack::fear_paralyze, "\
A beast with the body of a slightly-overweight\n\
man and the head of a cow.  It walks slowly,\n\
milky white drool dripping from its mouth,\n\
wearing only a pair of white underwear."
);


// ROBOTS
mon("eyebot",		'r',	c_ltblue,	MS_SMALL,	STEEL,
	(mfb(MF_SEES)|mfb(MF_FLIES)|mfb(MF_ELECTRONIC)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 20,  2,  2,120, 0,  0,  0,  0,  3, 10, 70,  20, 30,
	&mdeath::normal,	&mattack::photograph, "\
A roughly spherical robot that hovers about\n\
five feet of the ground.  Its front side is\n\
dominated by a huge eye and a flash bulb.\n\
Frequently used for reconaissance."
);

mon("manhack",		'r',	c_green,	MS_TINY,	STEEL,
	(mfb(MF_SEES)|mfb(MF_FLIES)|mfb(MF_NOHEAD)|mfb(MF_ELECTRONIC)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 18,  7,  5,130, 12,  1,  1,  8,  2,  0, 10,  5, 0,
	&mdeath::normal,	&mattack::none, "\
A fist-sized robot that flies swiftly through\n\
the air.  It's covered with whirring blades\n\
and has one small, glowing red eye."
);

mon("skitterbot",	'r',	c_ltred,	MS_SMALL,	STEEL,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_GOODHEARING)|mfb(MF_ELECTRONIC)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	 10, 13,  4,105,  0,  0,  0,  0,  0, 12, 60, 40, 5,
	&mdeath::normal,	&mattack::tazer, "\
A robot with an insectoid design, about\n\
the size of a small dog.  It skitters\n\
quickly across the ground, two electric\n\
prods at the ready."
);

mon("secubot",		'R',	c_dkgray,	MS_SMALL,	STEEL,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_BASHES)|mfb(MF_ATTACKMON)|
	 mfb(MF_ELECTRONIC)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  7, 19,  5, 70,  0,  0,  0,  0,  0, 14, 80, 80, 8,
	&mdeath::explode,	&mattack::smg, "\
A boxy robot about four feet high.  It moves\n\
slowly on a set of treads, and is armed with\n\
a large machine gun type weapon.  It is\n\
heavily armored."
);

mon("copbot",		'R',	c_dkgray,	MS_MEDIUM,	STEEL,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_BASHES)|mfb(MF_ATTACKMON)|
	 mfb(MF_ELECTRONIC)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  0, 12,  3,100,  4,  3,  2,  0,  8,  8, 80, 80, 3,
	&mdeath::normal,	&mattack::copbot, "\
A blue-painted robot that moves quickly on a\n\
set of three omniwheels.  It has a nightstick\n\
readied, and appears to be well-armored."
);

mon("molebot",		'R',	c_brown,	MS_MEDIUM,	STEEL,
	(mfb(MF_HEARS)|mfb(MF_GOODHEARING)|mfb(MF_DIGS)|mfb(MF_ELECTRONIC)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  2, 17,  5, 40, 13,  1,  4, 10,  0, 14, 82, 80, 0,
	&mdeath::normal,	&mattack::none,	"\
A snake-shaped robot that tunnels through the\n\
ground slowly.  When it emerges from the\n\
ground it can attack with its large, spike-\n\
covered head."
);

mon("tripod robot",	'R',	c_white,	MS_LARGE,	STEEL,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_GOODHEARING)|mfb(MF_BASHES)|
	 mfb(MF_ELECTRONIC)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  5, 26,  5, 90, 15,  2,  4,  7,  0,  8, 82, 80, 10,
	&mdeath::normal,	&mattack::flamethrower, "\
A 8-foot-tall robot that walks on three long\n\
legs.  It has a pair of spiked tentacles, as\n\
well as a flamethrower mounted on its head."
);

mon("chicken walker",	'R',	c_red,		MS_LARGE,	STEEL,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_BASHES)|mfb(MF_ELECTRONIC)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  3, 32,  5,115, 0,  0,  0,  0,  0,  14, 85, 90, 5,
	&mdeath::explode,	&mattack::smg, "\
A 10-foot-tall, heavily-armored robot that\n\
walks on a pair of legs with the knees\n\
facing backwards.  It's armed with a\n\
nasty-looking machine gun."
);

mon("tankbot",		'R',	c_blue,		MS_HUGE,	STEEL,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_GOODHEARING)|mfb(MF_NOHEAD)|
	 mfb(MF_BASHES)|mfb(MF_DESTROYS)|mfb(MF_ATTACKMON)|mfb(MF_ELECTRONIC)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  1, 52,  5,100, 0,  0,  0,  0,  0,  20, 92,240, 4,
	&mdeath::normal,	&mattack::multi_robot, "\
This fearsome robot is essentially an\n\
autonomous tank.  It moves surprisingly fast\n\
on its treads, and is armed with a variety of\n\
deadly weapons."
);

mon("turret",		't',	c_ltgray,	MS_SMALL,	STEEL,
	(mfb(MF_SEES)|mfb(MF_NOHEAD)|mfb(MF_ELECTRONIC)|mfb(MF_IMMOBILE)|
	 mfb(MF_FRIENDLY_SPECIAL)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  0, 14,  5,100,  0,  0,  0,  0,  0, 16, 88, 30, 1,
	&mdeath::explode,	&mattack::smg, "\
A small, round turret which extends from\n\
the floor.  Two SMG barrels swivel 360\n\
degrees."
);

// HALLUCINATIONS
mon("zombie",		'Z',	c_ltgreen,	MS_MEDIUM,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_SMELLS)|mfb(MF_STUMBLES)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  0,  0,  5, 65,  3,  0,  0,  0,  0,  0,  0,  1,  20,
	&mdeath::disappear,	&mattack::disappear, "\
A human body, stumbling slowly forward on\n\
uncertain legs, possessed with an\n\
unstoppable rage."
);

mon("giant bee",	'a',	c_yellow,	MS_MEDIUM,	FLESH,
	(mfb(MF_SMELLS)|mfb(MF_FLIES)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  0,  0,  5,180,  2,  0,  0,  0,  0,  0,  0,  1,  20,
	&mdeath::disappear,	&mattack::disappear, "\
A honey bee the size of a small dog. It\n\
buzzes angrily through the air, dagger-\n\
sized sting pointed forward."
);

mon("giant ant",	'a',	c_brown,	MS_MEDIUM,	FLESH,
	(mfb(MF_SMELLS)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  0,  0,  5,100,  3,  0,  0,  0,  0,  0,  0,  1,  20,
	&mdeath::disappear,	&mattack::disappear, "\
A red ant the size of a crocodile. It is\n\
covered in chitinous armor, and has a\n\
pair of vicious mandibles."
);

mon("your mother",	'@',	c_white,	MS_MEDIUM,	FLESH,
	(mfb(MF_SEES)|mfb(MF_HEARS)|mfb(MF_SMELLS)),
//	frq dif agr spd msk mdi m## cut dge arm itm  HP special freq
	  0,  0,  5,100,  3,  0,  0,  0,  0,  0,  0,  1,  20,
	&mdeath::guilt,		&mattack::disappear, "\
Mom?"
);
}

#include "itype.h"
#include "game.h"
#include "setvector.h"
#include <fstream>

// Armor colors
#define C_SHOES  c_blue
#define C_PANTS  c_brown
#define C_BODY   c_yellow
#define C_TORSO  c_ltred
#define C_ARMS   c_blue
#define C_GLOVES c_ltblue
#define C_MOUTH  c_white
#define C_EYES   c_cyan
#define C_HAT    c_dkgray
#define C_STORE  c_green
#define C_DECOR  c_ltgreen

// Special function for setting melee techniques
#define TECH(id, t) itypes[id]->techniques = t

std::vector<std::string> unreal_itype_ids;
std::vector<std::string> martial_arts_itype_ids;
std::vector<std::string> artifact_itype_ids;
std::vector<std::string> standard_itype_ids;
std::vector<std::string> pseudo_itype_ids;

// GENERAL GUIDELINES
// When adding a new item, you MUST REMEMBER to insert it in the itype_id enum
//  at the top of itype.h!
//  Additionally, you should check mapitemsdef.cpp and insert the new item in
//  any appropriate lists.
void game::init_itypes ()
{
// First, the null object.  NOT REALLY AN OBJECT AT ALL.  More of a concept.
 itypes["null"]=
  new itype("null", 0, 0, "none", "", '#', c_white, MNULL, MNULL, PNULL, 0, 0, 0, 0, 0, 0);
// Corpse - a special item
 itypes["corpse"]=
  new itype("corpse", 0, 0, "corpse", "A dead body.", '%', c_white, MNULL, MNULL, PNULL, 0, 0,
            0, 0, 1, 0);
// Fire - only appears in crafting recipes
 itypes["fire"]=
  new itype("fire", 0, 0, "nearby fire",
            "Some fire - if you are reading this it's a bug!",
            '$', c_red, MNULL, MNULL, PNULL, 0, 0, 0, 0, 0, 0);
// Integrated toolset - ditto
 itypes["toolset"]=
  new itype("toolset", 0, 0, "integrated toolset",
            "A fake item. If you are reading this it's a bug!",
            '$', c_red, MNULL, MNULL, PNULL, 0, 0, 0, 0, 0, 0);
// For smoking crack or meth
 itypes["apparatus"]=
  new itype("apparatus", 0, 0, "something to smoke that from, and a lighter",
            "A fake item. If you are reading this it's a bug!",
            '$', c_red, MNULL, MNULL, PNULL, 0, 0, 0, 0, 0, 0);

// Drinks
// Stim should be -8 to 8.
// quench MAY be less than zero--salt water and liquor make you thirstier.
// Thirst goes up by 1 every 5 minutes; so a quench of 12 lasts for 1 hour

// Any foods with a nutrition of lower than 5 will never prompt a 'You are full, force yourself to eat that?' message

#define DRINK(id, name,rarity,price,color,container,quench,nutr,spoils,stim,\
healthy,addict,charges,fun,use_func,addict_func,des, item_flags) \
	itypes[id] = new it_comest(id,rarity,price,name,des,'~',\
color,MNULL,LIQUID,2,1,0,0,0,item_flags,quench,nutr,spoils,stim,healthy,addict,charges,\
fun,container,"null",use_func,addict_func,"DRINK");

//     NAME		RAR PRC	COLOR     CONTAINER
DRINK("water","water",		90, 50,	c_ltcyan, "bottle_plastic",
//	QUE NUT SPO STM HTH ADD CHG FUN use_func	addiction type
	40,  0,  0,  0,  0,  0,  1,  0,&iuse::none,	ADD_NULL, "\
Water, the stuff of life, the best thirst-quencher available.", 0);

//     NAME		RAR PRC	COLOR     CONTAINER
DRINK("water_clean","clean water",	90, 50,	c_ltcyan, "bottle_plastic",
//	QUE NUT SPO STM HTH ADD CHG FUN use_func	addiction type
	50,  0,  0,  0,  0,  0,  1,  0,&iuse::none,	ADD_NULL, "\
Fresh, clean water. Truly the best thing to quench your thirst.", 0);

DRINK("sewage","sewage sample",	 5,  5, c_ltgreen, "bottle_plastic",
	 5,  0,  0,  0,-10,  0,  1,-20,&iuse::sewage,	ADD_NULL, "\
A sample of sewage from a treatment plant. Gross.", 0);

DRINK("salt_water","salt water",	20,  5,	c_ltcyan, "bottle_plastic",
	-30, 0,  0,  0,  1,  0,  1, -1,&iuse::none,	ADD_NULL, "\
Water with salt added. Not good for drinking.", 0);

DRINK("oj","orange juice",	50, 38,	c_yellow, "bottle_plastic",
	35,  4,120,  0,  2,  0,  1,  3,&iuse::none,	ADD_NULL, "\
Freshly-squeezed from real oranges! Tasty and nutritious.", 0);

DRINK("apple_cider","apple cider",	50, 38, c_brown,  "bottle_plastic",
	35,  4,144,  0,  3,  0,  1,  2,&iuse::none,	ADD_NULL, "\
Pressed from fresh apples. Tasty and nutritious.", 0);

DRINK("energy_drink","energy drink",	55, 45,	c_magenta,"can_drink",
	15,  1,  0,  8, -2,  2,  1,  5,&iuse::caff,	ADD_CAFFEINE, "\
Popular among those who need to stay up late working.", 0);

//     NAME		RAR PRC	COLOR     CONTAINER
DRINK("cola", "cola",		70, 35,	c_brown,  "can_drink",
//	QUE NUT SPO STM HTH ADD CHG FUN use_func	addiction type
	18,  3,  0,  6, -1,  2,  1,  5,&iuse::caff,	ADD_CAFFEINE, "\
Things go better with cola. Sugar water with caffeine added.", 0);

DRINK("rootbeer","root beer",	65, 30,	c_brown,  "can_drink",
	18,  3,  0,  1, -1,  0,  1,  3,&iuse::none,	ADD_NULL, "\
Like cola, but without caffeine. Still not that healthy.", 0);

DRINK("milk","milk",		50, 35,	c_white,  "bottle_glass",
	25,  8,  8,  0,  1,  0,  1,  0,&iuse::none,	ADD_NULL, "\
Baby cow food, appropriated for adult humans. Spoils rapidly.", 0);

DRINK("V8","V8",		15, 35,	c_red,    "can_drink",
	 6, 28,240,  0,  1,  0,  1,  0,&iuse::none,	ADD_NULL, "\
Contains up to 8 vegetables! Nutritious and tasty.", 0);

//     NAME		RAR PRC	COLOR     CONTAINER
DRINK("broth","broth",		15, 35, c_yellow, "can_food",
//	QUE NUT SPO STM HTH ADD CHG FUN use_func	addiction type
	10, 15,160,  0,  0,  0,  1,  1,&iuse::none,	ADD_NULL, "\
Vegetable stock. Tasty and fairly nutritious.", mfb(IF_EATEN_HOT));

DRINK("soup_veggy","vegetable soup",		15, 60, c_red,    "can_food",
	10, 60,120,  0,  2,  0,  1,  2,&iuse::none,	ADD_NULL, "\
A nutritious and delicious hearty vegetable soup.", mfb(IF_EATEN_HOT));

DRINK("soup_meat","meat soup",		15, 60, c_red,    "can_food",
	10, 60,120,  0,  2,  0,  1,  2,&iuse::none,	ADD_NULL, "\
A nutritious and delicious hearty meat soup.", mfb(IF_EATEN_HOT));
itypes["soup_meat"]->m1 = FLESH;

DRINK("soup_human","sap soup",		15, 60, c_red,    "can_food",
	10, 60,120,  0,  2,  0,  1,  2,&iuse::none,	ADD_NULL, "\
A soup made from someone who is a far better meal than person.", mfb(IF_EATEN_HOT));
itypes["soup_human"]->m1 = HFLESH;

DRINK("whiskey","whiskey",	16, 85,	c_brown,  "bottle_glass",
	-12, 4,  0,-12, -2,  5, 7, 15,&iuse::alcohol,	ADD_ALCOHOL, "\
Made from, by, and for real Southern colonels!", 0);

//     NAME		RAR PRC	COLOR     CONTAINER
DRINK("vodka","vodka",		20, 78,	c_ltcyan, "bottle_glass",
//	QUE NUT SPO STM HTH ADD CHG FUN use_func	addiction type
	-10, 2,  0,-12, -2,  5, 7, 15,&iuse::alcohol,	ADD_ALCOHOL, "\
In Soviet Russia, vodka drinks you!", 0);

DRINK("gin","gin",		20, 78,	c_ltcyan, "bottle_glass",
	-10, 2,  0,-12, -2,  5, 7, 15,&iuse::alcohol,	ADD_ALCOHOL, "\
Smells faintly of elderberries, but mostly booze.", 0);

DRINK("rum","rum",		14, 85,	c_ltcyan, "bottle_glass",
	-12, 2,  0,-10, -2,  5, 7, 15,&iuse::alcohol,	ADD_ALCOHOL, "\
Drinking this might make you feel like a pirate. Or not.", 0);

DRINK("tequila","tequila",	12, 88,	c_brown,  "bottle_glass",
	-12, 2,  0,-12, -2,  6, 7, 18,&iuse::alcohol,	ADD_ALCOHOL, "\
Don't eat the worm! Wait, there's no worm in this bottle.", 0);

DRINK("triple_sec","triple sec",	12, 55,	c_brown,  "bottle_glass",
	-8, 2,  0,-10, -2,  4, 7, 10,&iuse::alcohol,	ADD_ALCOHOL, "\
An orange flavored liquor used in many mixed drinks.", 0);

DRINK("long_island","long island iced tea",	8, 100,	c_brown,  "bottle_glass",
	-10, 2,  0,-10, -2,  5, 6, 20,&iuse::alcohol,	ADD_ALCOHOL, "\
A blend of incredibly strong-flavored liquors that somehow tastes\n\
like none of them.", 0);

DRINK("drink_screwdriver","Screwdriver", 8, 100, c_yellow, "bottle_glass",
   25, 6, 0, -12, 1, 4, 1, 20, &iuse::alcohol, ADD_ALCOHOL, "\
The surreptitious drunkard mechanic's drink of choice.", 0);

DRINK("drink_wild_apple","Wild Apple", 8, 100, c_brown, "bottle_glass",
   25, 6, 0, -12, 1, 4, 1, 20, &iuse::alcohol, ADD_ALCOHOL, "\
Like apple cider, only with vodka.", 0);

DRINK("beer","beer",           60, 35, c_brown,  "can_drink",
         16, 4,  0, -4, -1,  2,  1, 10, &iuse::alcohol_weak,   ADD_ALCOHOL, "\
Best served cold, in a glass, and with a lime - but you're not that lucky.", 0);

DRINK("bleach","bleach",		20, 18,	c_white,  "jug_plastic",
	-96, 0,  0,  0, -8,  0,  1,-30,&iuse::blech,	ADD_NULL, "\
Don't drink it. Mixing it with ammonia produces toxic gas.", 0);

//     NAME		RAR PRC	COLOR     CONTAINER
DRINK("ammonia","ammonia",	24, 30,	c_yellow, "jug_plastic",
//	QUE NUT SPO STM HTH ADD CHG FUN use_func	addiction type
	-96, 0,  0,  0, -2,  0,  1,-30,&iuse::blech,	ADD_NULL, "\
Don't drink it. Mixing it with bleach produces toxic gas.", 0);

DRINK("mutagen","mutagen",	 8,1000,c_magenta,"flask_glass",
	  0, 0,  0,  0, -2,  0,  1,  0,&iuse::mutagen_3,ADD_NULL, "\
A rare substance of uncertain origins. Causes you to mutate.", 0);

DRINK("purifier","purifier",	12, 6000,c_pink,  "flask_glass",
	  0, 0,  0,  0,  1,  0,  1,  0,&iuse::purifier,	ADD_NULL, "\
A rare stem-cell treatment that causes mutations and other genetic defects\n\
to fade away.", 0);

DRINK("tea","tea",		1, 50,	c_green, "bottle_plastic",
//	QUE NUT SPO STM HTH ADD CHG FUN use_func	addiction type
	40,  3,  0,  0,  0,  0,  1, 6,&iuse::none,	ADD_NULL, "\
Tea, the beverage of gentlemen everywhere.", mfb(IF_EATEN_HOT));

DRINK("coffee","coffee",		1, 50,	c_brown, "bottle_plastic",
	40,  3,  0,  12,  0,  0,  1, 6,&iuse::caff,	ADD_CAFFEINE, "\
Coffee. The morning ritual of the pre-apocalypse world.", mfb(IF_EATEN_HOT));

//     NAME		RAR PRC	COLOR     CONTAINER
DRINK("blood","blood",		 20,  0, c_red, "flask_glass",
//	QUE NUT SPO STM HTH ADD CHG FUN use_func	addiction type
	  5,  5,  0,  0, -8,  0,  1,-50,&iuse::none,	ADD_NULL, "\
Blood, possibly that of a human. Disgusting!", 0);

#define FOOD(id, name, rarity,price,color,mat1,container,volume,weight,quench,\
nutr,spoils,stim,healthy,addict,charges,fun,use_func,addict_func,des, item_flags) \
itypes[id]=new it_comest(id,rarity,price,name,des,'%',\
color,mat1,SOLID,volume,weight,0,0,0,item_flags, quench,nutr,spoils,stim,healthy,addict,charges,\
fun,container,"null",use_func,addict_func,"FOOD");
// FOOD

FOOD("bone", "bone",            50, 50, c_white,    FLESH, "null",
    1,  1,  0, 4,  0,   0, -1,  0, 1, 0,    &iuse::none, ADD_NULL, "\
A bone from some creature or other, could be eaten or used to make some\n\
stuff, like needles.",0);

FOOD("plant_sac", "fluid sac",            50, 50, c_white,    FLESH, "null",
    1,  1,  0, 4,  0,   0, -1,  0, 1, 0,    &iuse::none, ADD_NULL, "\
A fluid bladder from a plant based lifeform, not very nutritious, but\n\
fine to eat anyway.",0);

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("meat", "chunk of meat",	50, 50,	c_red,		FLESH,  "null",
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func    addiction type
    1,  2,  0, 20, 24,  0, -1,  0,  1,-10,	&iuse::none, ADD_NULL, "\
Freshly butchered meat. You could eat it raw, but cooking it is better.", 0);


FOOD("veggy", "plant marrow",	30, 60,	c_green,	VEGGY,	"null",
    1,  2,  0, 20, 80,  0,  1,  0,  1,  0,	&iuse::none, ADD_NULL, "\
A nutrient rich chunk of plant matter, could be eaten raw or cooked.", 0);

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("human_flesh", "human flesh",	50, 50,	c_red,		HFLESH,  "null",
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func    addiction type
    1,  2,  0, 20, 24,  0, -1,  0,  1, 0,	&iuse::none, ADD_NULL, "\
Freshly butchered from a human body.", 0);

FOOD("veggy_wild", "wild vegetables",	30, 60,	c_green,	VEGGY,	"null",
    1,  2,  0, 20, 80,  0,  1,  0,  1,  -10,	&iuse::none, ADD_NULL, "\
An assortment of edible-looking wild plants.  Most are quite bitter-tasting.", 0);

FOOD("meat_tainted", "tainted meat",	60,  4,	c_red,		FLESH,	"null",
    1,  2,  0, 20,  4,  0,  0,  0,  1,-10,	&iuse::poison, ADD_NULL, "\
Meat that's obviously unhealthy. You could eat it, but it will poison you.", 0);

FOOD("veggy_tainted", "tainted veggy",	35,  5,	c_green,	VEGGY,	"null",
    1,  2,  0, 20, 10,  0,  1,  0,  1,  0,	&iuse::poison, ADD_NULL, "\
Vegetable that looks poisonous. You could eat it, but it will poison you.", 0);

FOOD("meat_cooked", "cooked meat",	 0, 75, c_red,		FLESH,	"null",
    1,  2,  0, 50, 24,  0,  0,  0,  1,  8,	&iuse::none,	ADD_NULL, "\
Freshly cooked meat. Very nutritious.", mfb(IF_EATEN_HOT));

FOOD("human_cooked", "cooked creep",	 0, 75, c_red,		HFLESH,	"null",
    1,  2,  0, 50, 24,  0,  0,  0,  1,  8,	&iuse::none,	ADD_NULL, "\
A freshly cooked slice of some unpleasant person. Tastes great.", mfb(IF_EATEN_HOT));

FOOD("veggy_cooked", "cooked plant marrow",	 0, 70, c_green,	VEGGY,	"null",
    1,  2,  0, 40, 50,  0,  1,  0,  1,  0,	&iuse::none,	ADD_NULL, "\
A freshly cooked chunk of plant matter, tasty and nutritious.", mfb(IF_EATEN_HOT));

FOOD("veggy_wild_cooked", "cooked wild vegetables",	0, 70,	c_green,	VEGGY,	"null",
    1,  2,  0, 40, 50,  0,  1,  0,  1,  0,	&iuse::none, ADD_NULL, "\
Cooked wild edible plants.  An interesting mix of flavours.", mfb(IF_EATEN_HOT));

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("apple", "apple",		70, 16,	c_red,		VEGGY,  "null",
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func    addiction type
    1,  1,  3, 16,160,  0,  4,  0,  1,  3,	&iuse::none, ADD_NULL, "\
An apple a day keeps the doctor away.", 0);

FOOD("orange", "orange",		65, 18,	c_yellow,	VEGGY,	"null",
    1,  1,  8, 14, 96,  0,  0,  0,  1,  3,	&iuse::none, ADD_NULL, "\
Sweet citrus fruit. Also comes in juice form.", 0);

FOOD("lemon", "lemon",		50, 12, c_yellow,	VEGGY,	"null",
    1,  1,  5,  5,180,  0,  0,  0,  1, -4,	&iuse::none, ADD_NULL, "\
Very sour citrus. Can be eaten if you really want.", 0);

FOOD("chips", "potato chips",	65, 12,	c_magenta,	VEGGY,	"bag_plastic",
    2,  1, -2,  8,  0,  0,  0,  0,  3,  6,	&iuse::none, ADD_NULL, "\
Betcha can't eat just one.", 0);

FOOD("chips2", "potato chips",	65, 12,	c_magenta,	VEGGY,	"bag_plastic",
    2,  1, -2,  8,  0,  0,  0,  0,  3,  3,	&iuse::none, ADD_NULL, "\
A bag of cheap, own-brand chips.", 0);

FOOD("chips3", "potato chips",	65, 12,	c_magenta,	VEGGY,	"bag_plastic",
    2,  1, -2,  8,  0,  0,  0,  0,  3,  12,	&iuse::none, ADD_NULL, "\
Oh man, you love these chips! Score!", 0);

FOOD("pretzels", "pretzels",	55, 13,	c_brown,	VEGGY,	"bag_plastic",
    2,  1, -2,  9,  0,  0,  0,  0,  3,  2,	&iuse::none, ADD_NULL, "\
A salty treat of a snack.", 0);


FOOD("chocolate", "chocolate bar",	50, 20,	c_brown,	VEGGY,	"wrapper",
    1,  1,  0,  8,  0,  1,  0,  0,  1,  8,	&iuse::none, ADD_NULL, "\
Chocolate isn't very healthy, but it does make a delicious treat.", 0);

FOOD("jerky", "beef jerky",	55, 24,	c_red,		FLESH,  "bag_plastic",
    1,  1, -3, 12,  0,  0, -1,  0,  3,  4,	&iuse::none, ADD_NULL, "\
Salty dried meat that never goes bad, but will make you thirsty.", 0);

FOOD("jerky_human", "jerk jerky", 55, 24, c_red, HFLESH, "null",
    1,  1, -3, 12,  0,  0, -1,  0,  3,  4, &iuse::none, ADD_NULL, "\
Salty dried human flesh that never goes bad, but will make you thirsty.", 0);

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("sandwich_t", "meat sandwich", 30, 60,	c_ltgray,	FLESH,	"wrapper",
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func    addiction type
    1,  2,  0, 50, 36,  0,  0,  0,  1,  5,	&iuse::none, ADD_NULL, "\
Bread and turkey, that's it.", mfb(IF_EATEN_HOT));

FOOD("candy", "candy",		80, 14,	c_magenta,	VEGGY,	"bag_plastic",
    0, 0,  -1,  2,  0,  1, -2,  0,  3,  4,	&iuse::none, ADD_NULL, "\
A big bag of peanut butter cups... your favorite!", 0);

FOOD("mushroom", "mushroom",         4, 14,	c_brown,	VEGGY,	"null",
    1,  0,  0, 14,  0,  0,  1,  0,  1,  0,	&iuse::none, ADD_NULL, "\
Mushrooms are tasty, but be careful. Some can poison you, while others are\n\
hallucinogenic.", mfb(IF_EATEN_HOT));

FOOD("mushroom_poison", "mushroom",	 3, 14,	c_brown,	VEGGY,	"null",
    1,  0,  0, 10,  0,  0,  0,  0,  1,  0,	&iuse::poison, ADD_NULL, "\
Mushrooms are tasty, but be careful. Some can poison you, while others are\n\
hallucinogenic.", mfb(IF_EATEN_HOT));

FOOD("mushroom_magic", "mushroom",	 1, 14,	c_brown,	VEGGY,	"null",
    1,  0,  0, 10,  0, -4,  0,  0,  1,  6,	&iuse::hallu, ADD_NULL, "\
Mushrooms are tasty, but be careful. Some can poison you, while others are\n\
hallucinogenic.", mfb(IF_EATEN_HOT));

FOOD("blueberries", "blueberries",	 3, 20,	c_blue,		VEGGY,	"null",
    1,  1,  2, 16, 60,  0,  0,  0,  1,  0,	&iuse::none, ADD_NULL, "\
They're blue, but that doesn't mean they're sad.", 0);

FOOD("strawberries", "strawberries",	 2, 10,	c_red,		VEGGY,	"null",
    1,  1,  3, 18, 60,  0,  0,  0,  1,  0,	&iuse::none, ADD_NULL, "\
Tasty juicy berry, often found growing wild in fields.", 0);

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("tomato", "tomato",		 9, 25,	c_red,		VEGGY,  "null",
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func    addiction type
    1,  1,  3, 18, 90,  0,  0,  0,  1,  0,	&iuse::none, ADD_NULL, "\
Juicy red tomato. It gained popularity in Italy after being brought back\n\
from the New World.", 0);

FOOD("broccoli", "broccoli",	 9, 40,	c_green,	VEGGY,	"null",
    2,  2,  0, 30,160,  0,  1,  0,  1,  0,	&iuse::none, ADD_NULL, "\
It's a bit tough, but quite delicious.", mfb(IF_EATEN_HOT));

FOOD("zucchini", "zucchini",	 7, 30,	c_ltgreen,	VEGGY,	"null",
    2,  1,  0, 20,120,  0,  1,  0,  1,  0,	&iuse::none, ADD_NULL, "\
A tasty summer squash.", mfb(IF_EATEN_HOT));

FOOD("corn", "corn",	 7, 30,	c_ltgreen,	VEGGY,	"null",
    2,  1,  0, 20,120,  0,  1,  0,  1,  0,	&iuse::none, ADD_NULL, "\
Delicious golden kernels.", mfb(IF_EATEN_HOT));

FOOD("frozen_dinner", "frozen dinner",	50, 80,	c_magenta,	FLESH,	"box_small",
    5,  4, -2, 60, 60,  0, -2,  0,  1, -3,	&iuse::none, ADD_NULL, "\
Now with ONE POUND of meat and ONE POUND of carbs! Not as appetizing or\n\
nutritious as it would be if heated up.", 0);

FOOD("cooked_dinner", "cooked TV dinner", 0,  0, c_magenta,	FLESH,  "null",
    5,  4, -2, 72, 12,  0, -2,  0,  1,  5,	&iuse::none,	ADD_NULL, "\
Now with ONE POUND of meat and ONE POUND of carbs! Nice and heated up. It's\n\
tastier and more filling, but will also spoil quickly.", mfb(IF_EATEN_HOT));

FOOD("spaghetti_raw", "raw spaghetti",	40, 12,	c_yellow,	VEGGY,	"box_small",
    6,  2,  0,  6,  0,  0,  0,  0,  1, -8,	&iuse::none, ADD_NULL, "\
It could be eaten raw if you're desperate, but is much better cooked.", 0);

FOOD("spaghetti_cooked", "cooked spaghetti", 0, 28,	c_yellow,	VEGGY,	"box_small",
   10,  3,  0, 60, 20,  0,  0,  0,  1,  2,	&iuse::none, ADD_NULL, "\
Fresh wet noodles. Very tasty.", mfb(IF_EATEN_HOT));

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("macaroni_raw", "raw macaroni",	40, 15,	c_yellow,	VEGGY,	"box_small",
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func    addiction type
    3,  1,  0,  3,  0,  0,  0,  0,  1, -8,	&iuse::none, ADD_NULL, "\
It could be eaten raw if you're desperate, but is much better cooked.", 0);

FOOD("macaroni_cooked", "mac & cheese",	 0, 38,	c_yellow,	VEGGY,	"box_small",
    4,  2,  0, 60, 10,  0, -1,  0,  1,  3,	&iuse::none, ADD_NULL, "\
When the cheese starts flowing, Kraft gets your noodle going.", mfb(IF_EATEN_HOT));

FOOD("ravioli", "ravioli",		25, 75,	c_cyan,		FLESH,	"can_food",
    1,  2,  0, 48,  0,  0,  0,  0,  1,  0,	&iuse::none, ADD_NULL, "\
Meat encased in little dough satchels. Tastes fine raw.", mfb(IF_EATEN_HOT));

FOOD("sauce_red", "red sauce",	20, 24,	c_red,		VEGGY,	"can_food",
    2,  3,  0, 20,  0,  0,  0,  0,  1,  1,	&iuse::none, ADD_NULL, "\
Tomato sauce, yum yum.", mfb(IF_EATEN_HOT));

FOOD("sauce_pesto", "pesto",		15, 20,	c_ltgreen,	VEGGY,	"can_food",
    2,  3,  0, 18,  0,  0,  1,  0,  1,  4,	&iuse::none, ADD_NULL, "\
Olive oil, basil, garlic, pine nuts. Simple and delicious.", 0);

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("can_beans", "beans",		40, 55,	c_cyan,		VEGGY,	"can_food",
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func    addiction type
    1,  2,  0, 40,  0,  0,  0,  0,  1,  0,	&iuse::none, ADD_NULL, "\
Canned beans. A staple for hobos.", mfb(IF_EATEN_HOT));

FOOD("can_corn", "corn",		35, 40,	c_cyan,		VEGGY,	"can_food",
    1,  2,  5, 30,  0,  0,  0,  0,  1, -2,	&iuse::none, ADD_NULL, "\
Canned corn in water. Eat up!", mfb(IF_EATEN_HOT));

FOOD("can_spam", "SPAM",		30, 50,	c_cyan,		FLESH,	"can_food",
    1,  2, -3, 48,  0,  0,  0,  0,  1, -8,	&iuse::none, ADD_NULL, "\
Yuck, not very tasty, but it is quite filling.", mfb(IF_EATEN_HOT));

FOOD("can_pineapple", "pineapple",	30, 50,	c_cyan,		VEGGY,	"can_food",
    1,  2,  5, 26,  0,  0,  1,  0,  1,  7,	&iuse::none, ADD_NULL, "\
Canned pineapple rings in water. Quite tasty.", 0);

FOOD("can_coconut", "coconut milk",	10, 45,	c_cyan,		VEGGY,	"can_food",
    1,  2,  5, 30,  0,  0,  0,  0,  1,  0,	&iuse::none, ADD_NULL, "\
A dense, sweet creamy sauce, often used in curries.", 0);

FOOD("can_sardine", "sardines",	14, 25,	c_cyan,		FLESH,	"can_food",
    1,  1, -8, 14,  0,  0,  0,  0,  1,  0,	&iuse::none, ADD_NULL, "\
Salty little fish. They'll make you thirsty.", mfb(IF_EATEN_HOT));

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("can_tuna", "tuna fish",	35, 35,	c_cyan,		FLESH,	"can_food",
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func    addiction type
    1,  2,  0, 24,  0,  0,  0,  0,  1,  0,	&iuse::none, ADD_NULL, "\
Now with 95 percent less dolphins!", 0);

FOOD("can_catfood", "cat food",	20,  8,	c_cyan,		FLESH,	"can_food",
    1,  2,  0, 10,  0,  0, -1,  0,  1,-24,	&iuse::none, ADD_NULL, "\
Blech, so gross. Save it for when you're about to die of starvation.", 0);

FOOD("honeycomb", "honey comb",	10, 35,	c_yellow,	VEGGY,	"null",
    1,  1,  0, 20,  0,  0, -2,  0,  1,  9,	&iuse::honeycomb, ADD_NULL, "\
A large chunk of wax, filled with honey. Very tasty.", 0);

FOOD("wax", "wax",     	10, 35,	c_yellow,	VEGGY,	"null",
    1,  1,  0, 4,  0,  0, -2,  0,  1,  -5,	&iuse::none, ADD_NULL, "\
A large chunk of beeswax.\n\
Not very tasty or nourishing, but ok in an emergency.", 0);

FOOD("royal_jelly", "royal jelly",	 8,200,	c_magenta,	VEGGY,	"null",
    1,  1,  0, 10,  0,  0,  3,  0,  1,  7,	&iuse::royal_jelly, ADD_NULL, "\
A large chunk of wax, filled with dense, dark honey.  Useful for curing all\n\
sorts of afflictions.", 0);

FOOD("fetus", "misshapen fetus",	 1,150,	c_magenta,	HFLESH,	"null",
    4,  4,  0,  8,  0,  0, -8,  0,  1,-60,	&iuse::mutagen, ADD_NULL, "\
A deformed human fetus, eating this would be very nasty, and cause\n\
your DNA to mutate.", 0);

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("arm", "mutated arm",		 4,250,	c_brown,	HFLESH,	"null",
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func       addiction type
    8, 14,  0, 12,  0,  0, -8,  0,  1, -20,	&iuse::mutagen, ADD_NULL, "\
A misshapen human arm, eating this would be pretty disgusting\n\
and cause your DNA to mutate.", 0);

FOOD("leg", "mutated leg",		 4,250,	c_brown,	HFLESH,	"null",
   12, 24,  0, 16,  0,  0, -8,  0,  1, -20,	&iuse::mutagen, ADD_NULL, "\
A malformed human leg, this would be gross to eat, and cause\n\
mutations.", 0);

FOOD("ant_egg", "ant egg",		 5, 80,	c_white,	FLESH,	"null",
    4,  2, 10, 100, 0,  0, -1,  0,  1, -10,	&iuse::none,	ADD_NULL, "\
A large ant egg, the size of a softball. Extremely nutritious, but gross.", 0);

FOOD("marloss_berry", "marloss berry",	 2,600, c_pink,		VEGGY,	"null",
    1,  1, 20, 40,  0,  0,-10,  0,  1, 30,	&iuse::marloss,	ADD_NULL, "\
This looks like a blueberry the size of your fist, but pinkish in color. It\n\
has a strong but delicious aroma, but is clearly either mutated or of alien\n\
origin.", 0);

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("flour", "flour",		20, 25, c_white,	POWDER, "box_small",
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func       addiction type
    2,  4,  0,  1,  0,  0,  0,  0,  8, -5,	&iuse::none,	ADD_NULL, "\
This white flour is useful for baking.", 0);

FOOD("sugar", "sugar",		20, 25, c_white,	POWDER, "box_small",
    2,  3,  0,  3,  0,  4, -3,  0,  8,  1,	&iuse::none,	ADD_NULL, "\
Sweet, sweet sugar. Bad for your teeth and surprisingly not very tasty\n\
on its own.", 0);

FOOD("salt", "salt",		20, 18, c_white,	POWDER, "box_small",
    2,  2,-10,  0,  0,  0,  0,  0,  8, -8,	&iuse::none,	ADD_NULL, "\
Yuck, you surely wouldn't want to eat this. It's good for preserving meat\n\
and cooking with, though.", 0);

FOOD("potato_raw", "raw potato",	10, 10, c_brown,	VEGGY,  "null",
    1,  1,  0,  8,240,  0, -2,  0,  1, -3,	&iuse::none,	ADD_NULL, "\
Mildly toxic and not very tasty raw. When cooked, it is delicious.", 0);

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("potato_baked", "baked potato",	 0, 30, c_brown,	VEGGY,  "null",
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func       addiction type
    1,  1,  0, 20, 48,  0,  1,  0,  1,  3,	&iuse::none,	ADD_NULL, "\
A delicious baked potato. Got any sour cream?", mfb(IF_EATEN_HOT));

FOOD("bread", "bread",		14, 50, c_brown,	VEGGY,  "bag_plastic",
    4,  1,  0, 20,240,  0,  1,  0,  4,  1,	&iuse::none,	ADD_NULL, "\
Healthy and filling.", mfb(IF_EATEN_HOT));

FOOD("pie", "fruit pie",	20, 80, c_yellow,	VEGGY,  "box_small",
    6,  3,  5, 16, 72,  2,  1,  0,  6,  3,	&iuse::none,	ADD_NULL, "\
A delicious baked pie with a sweet fruit filling.", mfb(IF_EATEN_HOT));

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("pizza", "pizza",		 8, 80, c_ltred,	VEGGY,	"box_small",
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func       addiction type
    8,  4,  0, 18, 48,  0,  0,  0,  8,  6,	&iuse::none,	ADD_NULL, "\
A vegetarian pizza, with delicious tomato sauce and a fluffy crust.  Its\n\
smell brings back great memories.", mfb(IF_EATEN_HOT));

FOOD("mre_beef", "MRE - beef",		50,100, c_green,	FLESH,	"null",
    2,  1,  0, 50,  0,  0,  1,  0,  1, -4,	&iuse::none,	ADD_NULL, "\
Meal Ready to Eat. A military ration. Though not very tasty, it is very\n\
filling and will not spoil.", mfb(IF_EATEN_HOT));

FOOD("mre_veggy", "MRE - vegetable",		50,100, c_green,	VEGGY,	"null",
    2,  1,  0, 40,  0,  0,  1,  0,  1, -4,	&iuse::none,	ADD_NULL, "\
Meal Ready to Eat.  A military ration.  Though not very tasty, it is very\n\
filling and will not spoil.", mfb(IF_EATEN_HOT));

FOOD("tea_raw", "tea leaves",	10, 13,	c_green,	VEGGY,	"bag_plastic",
    2,  1, 0,  2,  0,  0,  0,  0,  5, -1,	&iuse::none, ADD_NULL, "\
Dried leaves of a tropical plant. You cam boil them into tea, or you\n\
can just eat them raw. They aren't too filling though.", 0);

FOOD("coffee_raw", "coffee powder",	15, 13,	c_brown,	VEGGY,	"bag_plastic",
    2,  1, 0,  0,  0,  8,  0,  0,  4, -5,	&iuse::caff, ADD_CAFFEINE, "\
Ground coffee beans. You can boil it into a mediocre stimulant,\n\
or swallow it raw for a lesser stimulative boost.", 0);

FOOD("jihelucake", "cake",            0, 0, c_white, VEGGY, "null",
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func       addiction type
    2,  1,   0, 25, 0,  0,  0,  0,  1,  20, &iuse::none, ADD_NULL, "\
Delicious sponge cake with buttercream icing, it says happy birthday on it.", 0);

FOOD("meat_canned", "canned meat",	 0, 25, c_red,		FLESH,	"bottle_glass",
    1,  2,  0, 50, 40,  0,  0,  0,  1,  2,	&iuse::none,	ADD_NULL, "\
Low-sodium preserved meat.  It was boiled and canned.\n\
Contains all of the nutrition, but little of the savor of cooked meat.\n",0 );

FOOD("veggy_canned", "canned veggy",	 0, 150, c_green,		VEGGY,	"bottle_glass",
    1,  2,  0, 40, 60,  0,  1,  0,  1,  0,	&iuse::none,	ADD_NULL, "\
This mushy pile of vegetable matter was boiled and canned in an earlier life.\n\
Better eat it before it oozes through your fingers.", 0);

FOOD("apple_canned", "canned apple slices",	 0, 32, c_red,		VEGGY,	"bottle_glass",
    1,  1,  3, 16, 180,  0,  2,  0,  1,  1,	&iuse::none,	ADD_NULL, "\
Sealed glass jar containing preserved apples.  Bland, mushy and losing color.", 0);

FOOD("human_canned", "canned cad",	 0, 25, c_red,		HFLESH,	"bottle_glass",
    1,  2,  0, 50, 40,  0,  0,  0,  1,  2,	&iuse::none,	ADD_NULL, "\
Low-sodium preserved human meat.  It was boiled and canned.\n\
Contains all of the nutrition, but little of the savor of cooked meat.\n",0 );

// MEDS
#define MED(id, name,rarity,price,color,tool,mat,stim,healthy,addict,\
charges,fun,use_func,addict_func,des) \
itypes[id]=new it_comest(id,rarity,price,name,des,'!',\
color,mat,SOLID,1,1,0,0,0,0,0,0,0,stim,healthy,addict,charges,\
fun,"null",tool,use_func,addict_func,"MED");

//  NAME		RAR PRC	COLOR		TOOL
MED("bandages", "bandages",		50, 60,	c_white,	"null",
//	MATERIAL STM HTH ADD CHG FUN use_func		addiction type
	COTTON,   0,  0,  0,  3,  0,&iuse::bandage,	ADD_NULL, "\
Simple cloth bandages. Used for healing small amounts of damage.");

MED("1st_aid", "first aid",	35,350,	c_red,		"null",
	PLASTIC,  0,  0,  0,  2,  0,&iuse::firstaid,	ADD_NULL, "\
A full medical kit, with bandages, anti-biotics, and rapid healing agents.\n\
Used for healing large amounts of damage.");

MED("vitamins", "vitamins",		75, 45,	c_cyan,		"null",
	PLASTIC,  0,  3,  0, 20,  0,&iuse::none,	ADD_NULL, "\
Take frequently to improve your immune system.");

MED("aspirin", "aspirin",		85, 30,	c_cyan,		"null",
	PLASTIC,  0, -1,  0, 20,  0,&iuse::pkill_1,	ADD_NULL, "\
Low-grade painkiller. Best taken in pairs.");

MED("caffeine", "caffeine pills",	25, 60,	c_cyan,		"null",
	PLASTIC,  8,  0,  3, 10,  0,&iuse::caff,	ADD_CAFFEINE, "\
No-doz pills. Useful for staying up all night.");

//  NAME		RAR PRC	COLOR		TOOL
MED("pills_sleep", "sleeping pills",	15, 50,	c_cyan,		"null",
//	MATERIAL STM HTH ADD CHG FUN use_func		addiction type
	PLASTIC, -8,  0, 40, 10,  0,&iuse::sleep,	ADD_SLEEP, "\
Prescription sleep aids. Will make you very tired.");

MED("iodine", "iodine tablets",	 5,140, c_yellow,	"null",
	PLASTIC,  0, -1,  0, 10,  0,&iuse::iodine,	ADD_NULL, "\
Iodine tablets are used for recovering from irradiation. They are not\n\
spectacularly effective, but are better than nothing.");

MED("dayquil", "Dayquil",		70, 75,	c_yellow,	"null",
	PLASTIC,  0,  1,  0, 10,  0,&iuse::flumed,	ADD_NULL, "\
Daytime flu medication. Will halt all flu symptoms for a while.");

MED("nyquil", "Nyquil",		70, 85,	c_blue,		"null",
	PLASTIC, -7,  1, 20, 10,  0,&iuse::flusleep,	ADD_SLEEP, "\
Nighttime flu medication. Will halt all flu symptoms for a while, plus make\n\
you sleepy.");

MED("inhaler", "inhaler",		14,200,	c_ltblue,	"null",
	PLASTIC,  1,  0,  0, 20,  0,&iuse::inhaler,	ADD_NULL, "\
Vital medicine for those with asthma. Those without asthma can use it for a\n\
minor stimulant boost.");

//  NAME		RAR PRC	COLOR		TOOL
MED("codeine", "codeine",		15,400,	c_cyan,		"null",
//	MATERIAL STM HTH ADD CHG FUN use_func		addiction type
	PLASTIC, -2,  0, 10, 10, 10,&iuse::pkill_2,	ADD_PKILLER, "\
A weak opiate, prescribed for light to moderate pain.");

MED("oxycodone", "oxycodone",	 7,900,	c_cyan,		"null",
	PLASTIC, -4, -1, 16, 10, 18,&iuse::pkill_3,	ADD_PKILLER, "\
A strong opiate, prescribed for moderate to intense pain.");

MED("tramadol", "tramadol",		11,300,	c_cyan,		"null",
	PLASTIC,  0,  0,  6, 10,  6,&iuse::pkill_l,	ADD_PKILLER, "\
A long-lasting opiate, prescribed for moderate pain. Its painkiller effects\n\
last for several hours, but are not as strong as oxycodone.");

MED("xanax", "Xanax",		10,600,	c_cyan,		"null",
	PLASTIC, -4,  0,  0, 10, 20,&iuse::xanax,	ADD_NULL, "\
Anti-anxiety medication. It will reduce your stimulant level steadily, and\n\
will temporarily cancel the effects of anxiety, like the Hoarder trait.");

//  NAME		RAR PRC	COLOR		TOOL
MED("adderall", "Adderall",		10,450,	c_cyan,		"null",
//	MATERIAL STM HTH ADD CHG FUN use_func		addiction type
	PLASTIC, 24,  0, 10, 10, 10,&iuse::none,	ADD_SPEED, "\
A strong stimulant prescribed for ADD. It will greatly increase your\n\
stimulant level, but is quite addictive.");

MED("thorazine", "Thorazine",	 7,500,	c_cyan,		"null",
	PLASTIC,  0,  0,  0, 10,  0,&iuse::thorazine,	ADD_NULL, "\
Anti-psychotic medication. Used to control the symptoms of schizophrenia and\n\
similar ailments. Also popular as a way to come down from a bad trip.");

MED("prozac", "Prozac",		10,650,	c_cyan,		"null",
	PLASTIC, -4,  0,  0, 15,  0,&iuse::prozac,	ADD_NULL, "\
A strong anti-depressant. Useful if your morale level is very low.");

MED("cig", "cigarettes",	90,120,	c_dkgray,	"null",
	VEGGY,    1, -1, 40, 10,  5,&iuse::cig,		ADD_CIG, "\
These will boost your dexterity, intelligence, and perception for a short\n\
time. They are quite addictive.");

//  NAME		RAR PRC	COLOR
MED("weed", "marijuana",	20,250,	c_green,	"null",
//	MATERIAL STM HTH ADD CHG FUN use_func		addiction type
	VEGGY,   -8,  0,  0,  5, 18,&iuse::weed,	ADD_NULL, "\
Really useful only for relaxing. Will reduce your attributes and reflexes.");

MED("coke", "cocaine",		 8,420,	c_white,	"null",
	POWDER,  20, -2, 30,  8, 25,&iuse::coke,	ADD_COKE, "\
A strong, illegal stimulant. Highly addictive.");

MED("meth", "methamphetamine",	 2,800, c_ltcyan,	"null",
	POWDER,  10, -4, 50,  6, 30,&iuse::meth,	ADD_SPEED, "\
A very strong illegal stimulant. Extremely addictive and bad for you, but\n\
also extremely effective in boosting your alertness.");

//  NAME		RAR PRC	COLOR
MED("heroin", "heroin",		 1,1000,c_brown,	"syringe",
//	MATERIAL STM HTH ADD CHG FUN use_func		addiction type
	POWDER, -10, -3, 60,  4, 45,&iuse::pkill_4,	ADD_PKILLER, "\
A very strong illegal opiate. Unless you have an opiate tolerance, avoid\n\
heroin, as it will be too strong for you.");

MED("cigar", "cigars",		 5,120,	c_dkgray,	"null",
	VEGGY,    1, -1, 40, 10, 15,&iuse::cig,		ADD_CIG, "\
A gentleman's vice. Cigars are what separates a gentleman from a savage.");

MED("antibiotics", "antibiotics",	25,900, c_pink,		"null",
	PLASTIC,   0, -2,  0, 15,  0,&iuse::antibiotic,	ADD_NULL, "\
Medication designed to stop the spread of, and kill, bacterial infections.");

MED("poppy_sleep", "Poppy Sleep",	25,900, c_pink,		"null",
	PLASTIC,   0, -2,  0, 5,  0,&iuse::sleep,	ADD_NULL, "\
Sleeping pills made by refining mutated poppy seeds.");

MED("poppy_pain", "Poppy Painkillers",25,900, c_pink,		"null",
	PLASTIC,   0, -2,  0, 10,  0,&iuse::pkill_2,	ADD_NULL, "\
Painkillers made by refining mutated poppy seeds..");

MED("crack", "crack",		 8,420,	c_white,	"apparatus",
	POWDER,  40, -2, 80,  4, 50,&iuse::crack,	ADD_CRACK, "\
Refined cocaine, incredibly addictive.");

/*MED("grack", "Grack Cocaine",      8,420, c_white,        "apparatus",
        POWDER,  200, -2, 80,  4, 50,&iuse::grack,       ADD_CRACK, "\
Grack Cocaine, the strongest substance known to the multiverse\n\
this potent substance is refined from the sweat of the legendary\n\
gracken");
*/

// MELEE WEAPONS
// Only use secondary material if it will have a major impact.
// dam is a somewhat rigid bonus--anything above 30, tops, is ridiculous
// cut is even MORE rigid, and should be kept lower still
// to_hit (affects chances of hitting) should be kept small, -5 to +5
// Note that do-nothing objects (e.g. superglue) belong here too!
#define MELEE(id, name,rarity,price,sym,color,mat1,mat2,volume,wgt,dam,cut,to_hit,\
              flags, des)\
itypes[id]=new itype(id,rarity,price,name,des,sym,\
color,mat1,mat2,SOLID,volume,wgt,dam,cut,to_hit,flags);

//    NAME		RAR PRC SYM  COLOR	MAT1	MAT2
MELEE("wrapper", "paper wrapper",	50,  1, ',', c_ltgray,	PAPER,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  0, -8,  0, -2, 0, "\
Just a piece of butcher's paper. Good for starting fires.");

//    NAME		RAR PRC SYM  COLOR	MAT1	MAT2
MELEE("withered", "withered plant",	70,  1, 't', c_ltgray,	PAPER,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  0, -8,  0, -2, 0, "\
A dead plant. Good for starting fires.");

MELEE("syringe", "syringe",	 8, 25, ',', c_ltcyan,	PLASTIC,MNULL,
	 1,  0, -4,  6, -2, mfb(IF_SPEAR), "\
A medical syringe. Used for administering heroin and other drugs.");

MELEE("fur", "fur pelt",	 0, 10, ',', c_brown,	FUR,	LEATHER,
	 1,  1, -8,  0,  0, 0, "\
A small bolt of fur from an animal. Can be made into warm clothing.");

MELEE("leather", "leather patch",	 0, 20, ',', c_red,	LEATHER, FLESH,
	 2,  1, -2,  0, -1, 0, "\
A smallish patch of leather, could be used to make tough clothing.");

//    NAME		RAR PRC SYM  COLOR	MAT1	MAT2
MELEE("superglue", "superglue",	30, 18, ',', c_white,	PLASTIC,MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  0, -2,  0, -2, 0, "\
A tube of strong glue. Used in many crafting recipes.");

MELEE("id_science", "science ID card", 2,600, ',', c_pink,	PLASTIC,MNULL,
	 0,  0, -8,  1, -3, 0, "\
This ID card once belonged to a scientist of some sort. It has a magnetic\n\
stripe on the back; perhaps it can be used on a control panel.");

MELEE("id_military", "military ID card",3,1200,',', c_pink,	PLASTIC,MNULL,
	 0,  0, -8,  1, -3, 0, "\
This ID card once belonged to a military officer with high-level clearance.\n\
It has a magnetic stripe on the back; perhaps it can be used on a control\n\
panel.");

MELEE("electrohack", "electrohack",	 3,400, ',', c_green,	PLASTIC,STEEL,
	 2,  2,  5,  0,  1, 0, "\
This device has many ports attached, allowing to to connect to almost any\n\
control panel or other electronic machine (but not computers). With a little\n\
skill, it can be used to crack passwords and more.");

MELEE("string_6", "string - 6 in",	 2,  5, ',', c_ltgray,	COTTON,	MNULL,
	 0,  0,-20,  0,  1, 0, "\
A small piece of cotton string.");

MELEE("string_36", "string - 3 ft",	40, 30, ',', c_ltgray,	COTTON,	MNULL,
	 1,  0, -5,  0,  1, 0, "\
A long piece of cotton string. Use scissors on it to cut it into smaller\n\
pieces.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("rope_6", "rope - 6 ft",	 4, 45, ',', c_yellow,	WOOD,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 2,  4,  1,  0,  1, mfb(IF_WRAP), "\
A short piece of nylon rope. Too small to be of much use.");

MELEE("rope_30", "rope - 30 ft",	35,100, ',', c_yellow,	WOOD,	MNULL,
	10, 20,  1,  0, -10, 0, "\
A long nylon rope. Useful for keeping yourself safe from falls.");

MELEE("chain", "steel chain",	20, 80, '/', c_cyan,	STEEL,	MNULL,
	 4,  8, 12,  0,  2, mfb(IF_WRAP), "\
A heavy steel chain. Useful as a weapon, or for crafting. It has a chance\n\
to wrap around your target, allowing for a bonus unarmed attack.");
TECH("chain", mfb(TEC_GRAB) );

MELEE("processor", "processor board",15,120, ',', c_ltcyan,	IRON,	PLASTIC,
	 1,  0, -3,  0, -1, 0, "\
A central processor unit, useful in advanced electronics crafting.");

MELEE("RAM", "RAM",		22, 90, ',', c_ltcyan,	IRON,	PLASTIC,
	 1,  0, -5,  0, -1, 0, "\
A stick of memory. Useful in advanced electronics crafting.");

MELEE("power_supply", "power converter",16,170, ',', c_ltcyan,	IRON,	PLASTIC,
	 4,  2,  5,  0, -1, 0, "\
A power supply unit. Useful in lots of electronics recipes.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("amplifier", "amplifier circuit",8,200,',', c_ltcyan,	IRON,	PLASTIC,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  0, -5,  0, -1, 0, "\
A circuit designed to amplify the strength of a signal. Useful in lots of\n\
electronics recipes.");

MELEE("transponder", "transponder circuit",5,240,',',c_ltcyan,	IRON,	PLASTIC,
	 1,  0, -5,  0, -1, 0, "\
A circuit designed to repeat a signal. Useful for crafting communications\n\
equipment.");

MELEE("receiver", "signal receiver",10,135, ',', c_ltcyan,	IRON,	PLASTIC,
	 1,  0, -4,  0, -1, 0, "\
A module designed to receive many forms of signals. Useful for crafting\n\
communications equipment.");

MELEE("burnt_out_bionic", "burnt out bionic",10,135, ',', c_ltred,	STEEL,	PLASTIC,
	 1,  0, -4,  0, -1, 0, "\
Once a valuable bionic implants, it's not held up well under repeated\n\
use. This object has been destroyed by excessive electric current and\n\
is now useless.");

MELEE("antenna", "antenna",	18, 80, ',', c_ltcyan,	STEEL,	MNULL,
	 1,  0, -6,  0,  2, 0, "\
A simple thin aluminum shaft. Useful in lots of electronics recipes.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("steel_chunk", "chunk of steel", 30, 10, ',', c_ltblue,	STEEL,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 2,  6, 12,  0, -2, 0, "\
A misshapen chunk of steel. Makes a decent weapon in a pinch, and is also\n\
useful for some crafting recipes.");

//    NAME      RAR PRC SYM COLOR   MAT1    MAT2
MELEE("steel_lump", "lump of steel", 30, 20, ',', c_ltblue,  STEEL,  MNULL,
//  VOL WGT DAM CUT HIT FLAGS
     2,  80, 18,  0, -4, 0, "\
A misshapen heavy piece of steel. Useful for some crafting recipes.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("scrap", "scrap metal", 30, 10, ',', c_ltblue,	STEEL,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  1,  0,  0, -2, 0, "\
An assortment of small bits of metal and scrap\n\
useful in all kinds of crafting");

MELEE("hose", "rubber hose",	15, 80, ',', c_green,	PLASTIC,MNULL,
	 3,  2,  4,  0,  3, mfb(IF_WRAP), "\
A flexible rubber hose. Useful for crafting.");

MELEE("glass_sheet", "sheet of glass",	 5,135, ']', c_ltcyan,	GLASS,	MNULL,
	50, 20, 16,  0, -5, 0, "\
A large sheet of glass. Easily shattered. Useful for re-paning windows.");

MELEE("manhole_cover", "manhole cover",	 1, 20, ']', c_dkgray,	IRON,	MNULL,
	45,250, 20,  0,-10, 0, "\
A heavy iron disc that typically covers a ladder into the sewers. Lifting\n\
it from the manhole is impossible without a crowbar.");

MELEE("stick", "heavy stick",	95,  0, '/', c_brown,	WOOD,	MNULL,
	 6, 10, 12,  0,  3, 0, "\
A sturdy, heavy stick. Makes a decent melee weapon, and can be cut into two\n\
by fours for crafting.");

MELEE("broom", "broom",		30, 24, '/', c_blue,	PLASTIC,MNULL,
	10,  8,  6,  0,  1, 0, "\
A long-handled broom. Makes a terrible weapon unless you're chasing cats.");
TECH("broom", mfb(TEC_WBLOCK_1) );

MELEE("hammer_sledge", "sledge hammer",	 6, 120,'/', c_brown,	WOOD,	IRON,
	18, 38, 40,  0,  0, 0, "\
A large, heavy hammer. Makes a good melee weapon for the very strong, but is\n\
nearly useless in the hands of the weak.");
TECH("hammer_sledge", mfb(TEC_BRUTAL)|mfb(TEC_WIDE) );

MELEE("hatchet", "hatchet",	10,  95,';', c_ltgray,	IRON,	WOOD,
	 6,  7, 12, 12,  1, 0, "\
A one-handed hatchet. Makes a great melee weapon, and is useful both for\n\
cutting wood, and for use as a hammer.");

MELEE("nailboard", "nail board",	 5,  80,'/', c_ltred,	WOOD,	MNULL,
	 6,  6, 16,  6,  1, mfb(IF_STAB), "\
A long piece of wood with several nails through one end; essentially a simple\n\
mace. Makes a great melee weapon.");
TECH("nailboard", mfb(TEC_WBLOCK_1) );

MELEE("nailbat", "nail bat",	60, 160,'/', c_ltred,	WOOD,	MNULL,
	12, 10, 28,  6,  3, mfb(IF_STAB), "\
A baseball bat with several nails driven through it, an excellent melee weapon.");
TECH("nailbat", mfb(TEC_WBLOCK_1) );

MELEE("pot", "pot",		25,  45,')', c_ltgray,	IRON,	MNULL,
	 8,  6,  9,  0,  1, 0, "\
Useful for boiling water when cooking spaghetti and more.");

MELEE("pan", "frying pan",	25,  50,')', c_ltgray,	IRON,	MNULL,
	 6,  6, 14,  0,  2, 0, "\
A cast-iron pan. Makes a decent melee weapon, and is used for cooking.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("knife_butter", "butter knife",	90,  15,';', c_ltcyan,	STEEL, 	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  2,  2,  1, -2, 0, "\
A dull knife, absolutely worthless in combat.");

MELEE("2x4", "two by four", 	60,  80,'/', c_ltred,	WOOD,	MNULL,
	 6,  6, 14,  0,  1, 0, "\
A plank of wood. Makes a decent melee weapon, and can be used to board up\n\
doors and windows if you have a hammer and nails.");
TECH("2x4", mfb(TEC_WBLOCK_1) );

MELEE("muffler", "muffler",	30,  30,'/', c_ltgray,	IRON,	MNULL,
	20, 20, 19,  0, -3, 0, "\
A muffler from a car. Very unwieldy as a weapon. Useful in a few crafting\n\
recipes.");
TECH("muffler", mfb(TEC_WBLOCK_2) );

MELEE("pipe", "pipe",		20,  75,'/', c_dkgray,	STEEL,	MNULL,
	 4, 10, 13,  0,  3, 0, "\
A steel pipe, makes a good melee weapon. Useful in a few crafting recipes.");
TECH("pipe", mfb(TEC_WBLOCK_1) );

MELEE("bat", "baseball bat",	60, 160,'/', c_ltred,	WOOD,	MNULL,
	12, 10, 28,  0,  3, 0, "\
A sturdy wood bat. Makes a great melee weapon.");
TECH("bat", mfb(TEC_WBLOCK_1) );

MELEE("bat_metal", "aluminium bat",	60, 160,'/', c_ltred,	STEEL,	MNULL,
	10, 6, 24,  0,  3, 0, "\
An aluminium baseball bat, smaller and lighter than a wooden bat\n\
and a little less damaging as a result.");
TECH("bat_metal", mfb(TEC_WBLOCK_1) );

MELEE("pointy_stick", "pointy stick",	 5,  40,'/', c_ltred,	WOOD,	MNULL,
	 5,  3,  4, 10,  1, mfb(IF_SPEAR), "\
A simple wood pole with one end sharpened.");
TECH("pointy_stick", mfb(TEC_WBLOCK_1) | mfb(TEC_RAPID) );

MELEE("spear_wood", "wood spear",	 5,  40,'/', c_ltred,	WOOD,	MNULL,
	 5,  3,  4, 18,  1, mfb(IF_SPEAR), "\
A stout pole with an improvised grip and a fire-hardened point.");
TECH("spear_wood", mfb(TEC_WBLOCK_1) | mfb(TEC_RAPID) );

MELEE("baton", "expandable baton",8, 175,'/', c_blue,	STEEL,	MNULL,
	 1,  4, 12,  0,  2, 0, "\
A telescoping baton that collapses for easy storage. Makes an excellent\n\
melee weapon.");
TECH("baton", mfb(TEC_WBLOCK_1) );

MELEE("bee_sting", "bee sting",	 5,  70,',', c_dkgray,	FLESH,	MNULL,
	 1,  0,  0, 18, -1, mfb(IF_SPEAR), "\
A six-inch stinger from a giant bee. Makes a good melee weapon.");
TECH("bee_sting", mfb(TEC_PRECISE) );

MELEE("wasp_sting", "wasp sting",	 5,  90,',', c_dkgray,	FLESH,	MNULL,
	 1,  0,  0, 22, -1, mfb(IF_SPEAR), "\
A six-inch stinger from a giant wasp. Makes a good melee weapon.");
TECH("wasp_sting", mfb(TEC_PRECISE) );

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("chitin_piece", "chunk of chitin",10,  15,',', c_red,	FLESH,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  0,  1,  0, -2, 0, "\
A piece of an insect's exoskeleton. It is light and very durable.");

MELEE("biollante_bud", "biollante bud",   1, 400,',', c_magenta,	VEGGY,	MNULL,
	 1,  0, -8,  0, -3, 0, "\
An unopened biollante flower, brilliant purple in color. It may still have\n\
its sap-producing organ intact.");

MELEE("canister_empty", "empty canister",  5,  20,'*', c_ltgray,	STEEL,	MNULL,
	 1,  1,  2,  0, -1, 0, "\
An empty canister, which may have once held tear gas or other substances.");

MELEE("gold", "gold bar",	10,3000,'/', c_yellow,	STEEL,	MNULL,
	 2, 60, 14,  0, -1, 0, "\
A large bar of gold. Before the apocalypse, this would've been worth a small\n\
fortune; now its value is greatly diminished.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("coal", "coal pallet",	20, 600,'/', c_dkgray,	STONE,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 72,100, 8,  0, -5, 0, "\
A large block of semi-processed coal.");

MELEE("petrified_eye", "petrified eye",   1,2000,'*', c_dkgray,	STONE,	MNULL,
	 2,  8, 10,  0, -1, 0, "\
A fist-sized eyeball with a cross-shaped pupil. It seems to be made of\n\
stone, but doesn't look like it was carved.");

MELEE("spiral_stone", "spiral stone",   20, 200,'*', c_pink,	STONE,	MNULL,
	 1,  3, 14,  0, -1, 0, "\
A rock the size of your fist. It is covered with intricate spirals; it is\n\
impossible to tell whether they are carved, naturally formed, or some kind of\n\
fossil.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("cane", "walking cane",   10, 160,'/', c_ltred,	WOOD,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	  8,  7, 10,  0,  2, 0, "\
Handicapped or not, you always walk in style. Consisting of a metal\n\
headpiece and a wooden body, this makes a great bashing weapon in a pinch.");
TECH("cane", mfb(TEC_WBLOCK_1) );

MELEE("binoculars", "binoculars",	20, 300,';', c_ltgray,	PLASTIC,GLASS,
	  2,  3,  6,  0, -1, 0, "\
A tool useful for seeing long distances. Simply carrying this item in your\n\
inventory will double the distance that is mapped around you during your\n\
travels.");

MELEE("usb_drive", "USB drive",	 5, 100,',', c_white,	PLASTIC,MNULL,
	  0,  0,  0,  0,  0, 0, "\
A USB thumb drive. Useful for holding software.");

MELEE("mace", "mace",		20,1000,'/',c_dkgray,	IRON,	WOOD,
	10, 18, 36,  0,  1, 0, "\
A medieval weapon consisting of a wood handle with a heavy iron end. It\n\
is heavy and slow, but its crushing damage is devastating.");
TECH("mace", mfb(TEC_SWEEP) );

MELEE("morningstar", "morningstar",	10,1200,'/',c_dkgray, 	IRON,	WOOD,
	11, 20, 32,  4,  1, mfb(IF_SPEAR), "\
A medieval weapon consisting of a wood handle with a heavy, spiked iron\n\
ball on the end. It deals devastating crushing damage, with a small\n\
amount of piercing to boot.");
TECH("morningstar", mfb(TEC_SWEEP) );

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("pool_cue", "pool cue",	 4, 80,'/', c_red,	WOOD,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	14,  5, 12,  0,  3, 0, "\
A hard-wood stick designed for hitting colorful balls around a felt\n\
table. Truly, the coolest of sports.");
TECH("pool_cue", mfb(TEC_WBLOCK_1) );

MELEE("pool_ball", "pool ball",	40, 30,'*', c_blue,	STONE,	MNULL,
	 1,  3, 12,  0, -3, 0, "\
A colorful, hard ball. Essentially a rock.");

MELEE("candlestick", "candlestick",	20,100,'/', c_yellow,	SILVER,	MNULL,
	 1,  5, 12,  0,  1,  0, "\
A gold candlestick.");

MELEE("spike", "spike",           0, 0,';',  c_cyan,     STEEL,  MNULL,
	 2,  2,  2, 10, -2, mfb(IF_STAB),"\
A large and slightly misshapen spike, could do some damage\n\
mounted on a vehicle.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("blade", "blade",	 5, 280,'/', c_blue,	IRON,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 8, 10,  6, 10, -2, 0, "\
A large, relatively sharp blade. Could be used to make\n\
bladed weaponry, or attached to a car.");

MELEE("wire", "wire",   50, 200,';', c_blue,    STEEL,  MNULL,
         4,  2,  0,  0, -2, 0, "\
A length of thin, relatively stiff, steel wire. Like the\n\
the sort you find in wire fences.");

MELEE("wire_barbed", "barbed wire",   20, 200,';', c_blue,    STEEL,  MNULL,
         4,  2,  0,  0, -2, 0, "\
A length of stiff wire, covered in sharp barbs.");

MELEE("rebar", "rebar",		20,  75,'/', c_ltred,	IRON,	MNULL,
	 6, 10, 13,  0,  2, 0, "\
A length of rebar, makes a nice melee weapon, and could be\n\
handy in constructing tougher walls and such.");

MELEE("log", "log",                    20,  100,'/', c_brown,  WOOD,   MNULL,
         40, 20, 10, 0, -10, 0, "\
A large log, cut from a tree. (a)ctivate a wood ax or wood\n\
saw to cut it into planks");

MELEE("splinter", "splintered wood", 	60,  80,'/', c_ltred,	WOOD,	MNULL,
	 6,  6, 9,  0,  1, 0, "\
A splintered piece of wood, useless as anything but kindling");

MELEE("skewer", "skewer",                 10,  10,',', c_brown,   WOOD,   MNULL,
         0,  0, 0,  0,  -10, 0, "\
A thin wooden skewer. Squirrel on a stick, anyone?");

MELEE("crackpipe", "crack pipe",             37,  35, ',',c_ltcyan,  GLASS,  MNULL,
         1,  1, 0,  0,  -10, 0, "\
A fine glass pipe, with a bulb on the end, used for partaking of\n\
certain illicit substances.");

MELEE("torch_done", "burnt out torch",	95,  0, '/', c_brown,	WOOD,	MNULL,
	 6, 10, 12,  0,  3, 0, "\
A torch that has consumed all its fuel; it can be recrafted\n\
into another torch");

MELEE("spring", "spring", 50, 10, ',', c_ltgray,  STEEL,  MNULL,
         3,  0, -1,  0,  0, 0, "\
A large, heavy-duty spring. Expands with significant force\n\
when compressed.");

MELEE("lawnmower", "lawnmower", 25, 100, ';', c_red, STEEL,  IRON,
         25, 40, -3, 10, 0, 0, "\
A motorized pushmower that seems to be broken. You could\n\
take it apart if you had a wrench.");

MELEE("sheet", "sheet",           0, 100, ';', c_dkgray, COTTON, MNULL,
         20, 2, 0, 0,    -1, 0, "\
A large fabric sheet, could be used as a curtain or bedsheets;\n\
or cut up for a bunch of rags.");

//  NAME        RAR PRC SYM  COLOR  MAT1    MAT
MELEE("broketent", "damaged tent",17, 65, ';', c_green,	IRON,	MNULL,
	 10,  20,  4,  0, -3, 0, "\
A small tent, just big enough to fit a person comfortably.\n\
This tent is broken and cannot be deployed");

MELEE("element", "heating element", 20, 10, ',', c_cyan,   IRON,   MNULL,
         0,   1,   0,  0,  0, 0, "\
A heating element, like the ones used in hotplates or kettles.");

MELEE("television", "television",      40, 0,  ';', c_dkgray,   PLASTIC, GLASS,
        10,  12,  5, 0, -5, 0, "\
A large cathode ray tube television, full of delicious\n\
electronics.");

MELEE("pilot_light", "pilot light", 20, 10, ',', c_cyan,   IRON,   PLASTIC,
         0,   1,   0,  0,  0, 0, "\
A pilot light from a gas-burning device, this particular one\n\
is a simple piezo electric igniter.");

MELEE("toaster", "toaster", 50, 10, ',', c_cyan, IRON, PLASTIC,
         2,   1,   0,  0,  0, 0, "\
A small two slice toaster, not much use as anything but spare parts");

MELEE("microwave", "microwave", 50, 10, ',', c_cyan, IRON, PLASTIC,
         8,   5,   0,  0,  0, 0, "\
A home microwave, has probably seen its share of baked beans.\n\
Good for scrap parts.");

MELEE("laptop", "laptop computer", 50, 10, ',', c_cyan, IRON, PLASTIC,
         3,   2,   0,  0,  0, 0, "\
A broken laptop, basically a paperweight now");

MELEE("fan", "desk fan", 50, 10, ',', c_cyan, IRON, PLASTIC,
         4,   1,   0,  0,  0, 0, "\
A small fan, used to propel air around a room.");

MELEE("ceramic_plate", "ceramic plate", 50, 10, ',', c_cyan, GLASS, MNULL,
         1,   1,   1,  0,  0, 0, "\
A ceramic dinner plate, you could probably play frisbee with it");

MELEE("ceramic_bowl", "ceramic bowl", 50, 10, ',', c_cyan, GLASS, MNULL,
         1,   1,   1,  0,  0, 0, "\
A shallow dessert bowl, not a lot of use for it really.");

MELEE("ceramic_cup", "ceramic cup", 50, 10, ',', c_cyan, GLASS, MNULL,
         1,   1,   1,  0,  0, 0, "\
A ceramic teacup, pinky out!");

MELEE("glass_plate", "glass plate", 50, 10, ',', c_cyan, GLASS, MNULL,
         1,   1,   1,  0,  0, 0, "\
A glass dinner plate, you could probably play frisbee with it");

MELEE("glass_bowl", "glass bowl", 50, 10, ',', c_cyan, GLASS, MNULL,
         1,   1,   1,  0,  0, 0, "\
A glass dessert bowl, not a lot of use for it really.");

MELEE("glass", "glass", 50, 10, ',', c_cyan, GLASS, MNULL,
         1,   1,   1,  0,  0, 0, "\
A tall glass, just begging for a frosty one!");

MELEE("tin_plate", "tin plate", 50, 10, ',', c_cyan, STEEL, MNULL,
         1,   0,   0,  0,  0, 0, "\
A tin dinner plate, you could probably play frisbee with it");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("fork", "fork",	90,  15,';', c_ltcyan,	STEEL, 	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  2,  2,  1, -2, 0, "\
A fork, if you stab something with it you eat it right away\n\
Wait.. nevermind.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("spork", "spork",	90,  15,';', c_ltcyan,	STEEL, 	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  2,  2,  1, -2, 0, "\
Foons are for scrubs, real men use sporks.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("foon", "foon",	90,  15,';', c_ltcyan,	STEEL, 	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  2,  2,  1, -2, 0, "\
Clearly the superior instrument. Sporks are just imitators.");

MELEE("rag_bloody", "blood soaked rag",    1, 0,  ',', c_red, COTTON,   MNULL,
         0, 0, 0, 0, 0, 0, "\
A large rag, drenched in blood. It could be cleaned with\n\
boiling water.");

MELEE("clock", "clock",               60, 0, ';', c_ltcyan, PLASTIC, IRON,
         1, 2, 0, 0, 0, 0, "\
A small mechanical clock, it's stopped at 10:10.");

MELEE("clockworks", "clockworks",          30, 0, ';', c_ltcyan, IRON, MNULL,
         1, 1, 0, 0, 0, 0, "\
A small assortment of gears and other clockwork gubbins.");

MELEE("javelin", "wooden javelin",	 5,  40,'/', c_ltred,	WOOD,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 5,  3,  6,  22,  2, mfb(IF_SPEAR), "\
A wooden spear, honed to a sharper point and fire hardened\n\
for toughness. The grip area has also be carved and covered\n\
for better grip.");
TECH("javelin", mfb(TEC_WBLOCK_1) | mfb(TEC_RAPID) );

MELEE("rock_pot", "stone pot", 0, 0, ';', c_dkgray, STONE, MNULL,
     9, 3,  4, 0, -1, 0, "\
A large stone, roughly hollowed out into a pot.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("frame", "steel frame",  20, 55, ']', c_cyan,  STEEL,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    60,  240,  20,  0,  -5, 0, "\
A large frame made of steel. Useful for crafting.");
TECH("frame", mfb(TEC_DEF_DISARM) );

#define VAR_VEH_PART(id, name,rarity,price,sym,color,mat1,mat2,volume,wgt,dam,cut,to_hit,\
              flags, bigmin, bigmax, bigaspect, des)\
itypes[id]=new it_var_veh_part(id,rarity,price,name,des,sym,\
color,mat1,mat2,volume,wgt,dam,cut,to_hit,flags, bigmin, bigmax, bigaspect)

//"wheel", "wheel_wide", "wheel_bicycle", "wheel_motorbike", "wheel_small",
//           NAME     RAR PRC  SYM COLOR        MAT1    MAT2
VAR_VEH_PART("wheel", "wheel", 10, 100, ']', c_dkgray,  STEEL,   PLASTIC,
//  VOL WGT DAM CUT HIT FLAGS BIGNESS_MIN BIGNESS_MAX  BIGNESS_ASPECT
    40,  140, 12,  0,  -1, 0,       13,         20,  BIGNESS_WHEEL_DIAMETER,  "\
A car wheel");
//           NAME         RAR PRC  SYM COLOR        MAT1    MAT2
VAR_VEH_PART("wheel_wide", "wide wheel", 4, 340, ']', c_dkgray,  STEEL,   PLASTIC,
//  VOL WGT  DAM CUT HIT FLAGS BIGNESS_MIN BIGNESS_MAX   ASPECT
    70,  260, 17,  0,  -1, 0,       17,         36,  BIGNESS_WHEEL_DIAMETER,  "\
A wide wheel. \\o/ This wide.");
//           NAME            RAR  PRC  SYM COLOR        MAT1    MAT2
VAR_VEH_PART("wheel_bicycle", "bicycle wheel", 18, 40,  ']', c_dkgray,  STEEL,   PLASTIC,
//  VOL WGT  DAM CUT HIT FLAGS BIGNESS_MIN BIGNESS_MAX    ASPECT
    28,  45,  8,  0,  -1, 0,       9,         18,  BIGNESS_WHEEL_DIAMETER,  "\
A bicycle wheel");
//           NAME              RAR  PRC   SYM COLOR        MAT1    MAT2
VAR_VEH_PART("wheel_motorbike", "motorbike wheel", 13, 140,  ']', c_dkgray,  STEEL,   PLASTIC,
//  VOL WGT  DAM CUT HIT FLAGS BIGNESS_MIN BIGNESS_MAX    ASPECT
    33,  85,  10,  0,  -1, 0,       9,         14,  BIGNESS_WHEEL_DIAMETER,  "\
A motorbike wheel");
//           NAME              RAR  PRC   SYM COLOR        MAT1    MAT2
VAR_VEH_PART("wheel_small", "small wheel",    5, 140,  ']', c_dkgray,  STEEL,   PLASTIC,
//  VOL WGT  DAM CUT HIT FLAGS BIGNESS_MIN BIGNESS_MAX    ASPECT
    9,  42,  10,  0,  -1, 0,       6,         14,   BIGNESS_WHEEL_DIAMETER,  "\
A pretty small wheel. Probably from one of those segway things.\
It is not very menacing.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("seat", "seat",  8, 250, '0', c_red,  PLASTIC,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    30,  80,  4,  0,  -4, 0, "\
A soft car seat covered with leather.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("vehicle_controls", "vehicle controls",  3, 400, '$', c_ltcyan,  PLASTIC,   STEEL,
//  VOL WGT DAM CUT HIT FLAGS
    12,  30,  2,  0,  -4, 0, "\
A set of various vehicle controls. Useful for crafting.");

//                                 NAME           RAR PRC SYM COLOR        MAT1    MAT2
VAR_VEH_PART("1cyl_combustion", "1-cylinder engine",  3, 100, ':', c_ltcyan,  IRON,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS 0BIGNESS_MIN BIGNESS_MAX   ASPECT
    6,  70,  4,  0,  -1, 0,       28,         75,   BIGNESS_ENGINE_DISPLACEMENT, "\
A single-cylinder 4-stroke combustion engine.");

//                              NAME           RAR PRC SYM COLOR        MAT1    MAT2
VAR_VEH_PART("v2_combustion", "V-twin engine",  2, 100, ':', c_ltcyan,  IRON,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS BIGNESS_MIN BIGNESS_MAX ASPECT
    6,  70,  4,  0,  -1, 0,       65,        260, BIGNESS_ENGINE_DISPLACEMENT, "\
A 2-cylinder 4-stroke combustion engine.");

//                                NAME           RAR PRC SYM COLOR        MAT1    MAT2
VAR_VEH_PART("i4_combustion", "Inline-4 engine",  6, 150, ':', c_ltcyan,  IRON,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS BIGNESS_MIN BIGNESS_MAX ASPECT
    6,  160,  8,  0,  -2, 0,       220,       350, BIGNESS_ENGINE_DISPLACEMENT, "\
A small, yet powerful 4-cylinder combustion engine.");

//                          NAME           RAR PRC SYM COLOR        MAT1    MAT2
VAR_VEH_PART("v6_combustion", "V6 engine",  3, 180, ':', c_ltcyan,  IRON,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS BIGNESS_MIN BIGNESS_MAX ASPECT
    14,  400,  12,  0,  -3, 0,    250,        520, BIGNESS_ENGINE_DISPLACEMENT, "\
A powerful 6-cylinder combustion engine.");

//                          NAME           RAR PRC SYM COLOR        MAT1    MAT2
VAR_VEH_PART("v8_combustion", "V8 engine",  2, 250, ':', c_ltcyan,  IRON,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS BIGNESS_MIN BIGNESS_MAX ASPECT
    25,  600,  15,  0,  -5, 0,    380,     700, BIGNESS_ENGINE_DISPLACEMENT, "\
A large and very powerful 8-cylinder combustion engine.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("motor", "electric motor",  2,120, ',', c_ltcyan,  IRON,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    6,  80,  4,  0,  0, 0, "\
A powerful electric motor. Useful for crafting.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("motor_large", "large electric motor",  1,220, ':', c_ltcyan,  IRON,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    15,  650,  9,  0,  -3, 0, "\
A large and very powerful electric motor. Useful for crafting.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("plasma_engine", "plasma engine",  1, 900, ':', c_ltcyan,  STEEL,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    12,  350,  7,  0,  -2, 0, "\
High technology engine, working on hydrgen fuel.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("foot_crank", "foot crank",  10, 90, ':', c_ltgray,  IRON,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    2,  10,  10,  0,  -1, 0, "\
The pedal and gear assembly from a bicycle.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("metal_tank", "metal tank",  10, 40, '}', c_ltcyan,  STEEL,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    18,  25,  3,  0,  -2, 0, "\
A metal tank for holding liquids. Useful for crafting.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("storage_battery", "storage battery",  6, 80, ':', c_ltcyan,  IRON,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    8,  220,  6,  0,  -2, 0, "\
A large storage battery. Useful for crafting.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("minireactor", "minireactor",  1, 900, ':', c_ltcyan,  STEEL,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    6,  250,  11,  0,  -4, 0, "\
A small portable plutonium reactor. Handle with great care!");

//      NAME          RAR PRC SYM COLOR        MAT1    MAT2
MELEE("solar_panel", "solar panel",  3, 900, ']', c_yellow,  GLASS,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    12,  4,  1,  0,  -4, 0, "\
Electronic device that can convert solar radiation into electric\n\
power. Useful for crafting.");

MELEE("solar_cell", "solar cell", 5, 50, ';', c_yellow, GLASS, MNULL,
      1, 0,  1,  0,  -4, 0, "\
A small electronic device that can convert solar radiation into\n\
electric power. Useful for crafting.");

MELEE("sheet_metal", "sheet metal",  30, 60, ']', c_ltcyan,  STEEL,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    4,  20,  5,  0,  -2, 0, "\
A thin sheet of metal.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("steel_plate", "steel plating",  30, 120, ']', c_ltcyan,  STEEL,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    12,  600,  6,  0,  -1, 0, "\
A piece of armor plating made of steel.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("alloy_plate", "superalloy plating",  10, 185, ']', c_ltcyan,  STEEL,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    12,  350,  6,  0,  -1, 0, "\
A piece of armor plating made of sturdy superalloy.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("spiked_plate", "spiked plating",  15, 185, ']', c_ltcyan,  STEEL,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    14,  600,  6,  3,  -1, 0, "\
A piece of armor plating made of steel. It is covered by menacing\n\
spikes.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("hard_plate", "hard plating",  30, 160, ']', c_ltcyan,  STEEL,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    12,  1800,  6,  0,  -1, 0, "\
A piece of very thick armor plating made of steel.");

MELEE("kitchen_unit", "RV kitchen unit", 20, 400, '&', c_ltcyan, STEEL, MNULL,
    80, 900, 0, 0, -2, 0, "\
A vehicle mountable electric range and sink unit with integrated\n\
tool storage for cooking utensils.");

// ARMOR
#define ARMOR(id, name, rarity,price,color,mat1,mat2,volume,wgt,dam,to_hit,\
encumber,dmg_resist,cut_resist,env,warmth,storage,covers,des,item_flags)\
itypes[id]=new it_armor(id, rarity,price,name,des,'[',\
color,mat1,mat2,volume,wgt,dam,0,to_hit,item_flags,covers,encumber,dmg_resist,cut_resist,\
env,warmth,storage)

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("socks", "socks",	70, 100,C_SHOES,	COTTON,	MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    1,  1, -5,  0,  0,  0,  0,  0,  10,  0,	mfb(bp_feet), "\
Socks. Put 'em on your feet.", 0);

ARMOR("socks_wool", "wool socks",		30, 120,C_SHOES,	WOOL,	MNULL,
    2,  1, -5,  0,  0,  0,  0,  0,  20,  0,	mfb(bp_feet), "\
Warm socks made of wool.", 0);

ARMOR("sneakers", "sneakers",	80, 100,C_SHOES,	LEATHER,	MNULL,
    5,  4, -2,  0,  0,  0,  2,  0,  20,  0,	mfb(bp_feet), "\
Guaranteed to make you run faster and jump higher!", mfb(IF_VARSIZE));

ARMOR("boots", "boots",		70, 120,C_SHOES,	LEATHER,	MNULL,
    7,  6,  1, -1,  1,  1,  4,  2,  50,  0,	mfb(bp_feet), "\
Tough leather boots. Very durable.", mfb(IF_VARSIZE));

ARMOR("boots_fur", "fur boots",		70, 120,C_SHOES,	LEATHER,	FUR,
    7,  6,  1, -1,  1,  1,  4,  2,  70,  0,	mfb(bp_feet), "\
Boots lined with fur for warmth.", mfb(IF_VARSIZE));

ARMOR("boots_steel", "steeltoed boots",50, 135,C_SHOES,	LEATHER,	STEEL,
    7,  9,  4, -1,  1,  4,  4,  3,  50,  0,	mfb(bp_feet), "\
Leather boots with a steel toe. Extremely durable.", mfb(IF_VARSIZE));

ARMOR("boots_winter", "winter boots",	60, 140,C_SHOES,	WOOL,	PLASTIC,
    8,  7,  0, -1,  2,  0,  2,  1,  90,  0,	mfb(bp_feet), "\
Cumbersome boots designed for warmth.", mfb(IF_VARSIZE));

ARMOR("mocassins", "mocassins",	 5,  80,C_SHOES,	FUR,	LEATHER,
    2,  1, -3,  0,  0,  0,  1,  0,  40,  0,	mfb(bp_feet), "\
Simple shoes made from animal pelts.", 0);

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("flip_flops", "flip-flops",	35,  25,C_SHOES,	PLASTIC,	MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    1,  1, -4, -2,  3,  0,  0,  0,  0,  0,	mfb(bp_feet), "\
Simple sandals. Very difficult to run in.", mfb(IF_VARSIZE));

ARMOR("dress_shoes", "dress shoes",	50,  45,C_SHOES,	LEATHER,	MNULL,
    5,  3,  1,  1,  1,  0,  3,  0,  10,  0,	mfb(bp_feet), "\
Fancy patent leather shoes. Not designed for running in.", mfb(IF_VARSIZE));

ARMOR("heels", "heels",		50,  50,C_SHOES,	LEATHER,	MNULL,
    4,  2,  6, -2,  4,  0,  0,  0,  0,  0,	mfb(bp_feet), "\
A pair of high heels. Difficult to even walk in.", mfb(IF_VARSIZE));

ARMOR("boots_chitin", "chitinous boots",50, 135,C_SHOES,	LEATHER,	STEEL,
    7,  9,  4, -1,  1,  4,  4,  3,  50,  0,	mfb(bp_feet), "\
Boots made from the exoskeletons of insects. Light and durable.", 0);

ARMOR("shorts", "shorts",		70, 180,C_PANTS,	COTTON,		MNULL,
    4,  2, -4,  1,  0,  0,  1,  0,  0,  4,	mfb(bp_legs), "\
A pair of khaki shorts.", mfb(IF_VARSIZE));

ARMOR("shorts_cargo", "cargo shorts",		50, 180,C_PANTS,	COTTON,		MNULL,
    4,  2, -4,  1,  0,  0,  1,  0,  0,  8,	mfb(bp_legs), "\
A pair of shorts lined with pockets, offering decent storage.", mfb(IF_VARSIZE));

ARMOR("jeans", "jeans",		90, 180,C_PANTS,	COTTON,		MNULL,
    5,  4, -4,  1,  1,  0,  1,  0,  10,  2,	mfb(bp_legs), "\
A pair of blue jeans with two deep pockets.", mfb(IF_VARSIZE));

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("pants", "pants",		75, 185,C_PANTS,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    5,  5, -4,  1,  1,  0,  1,  0,  20,  4,	mfb(bp_legs), "\
A pair of khaki pants. Slightly warmer than jeans.", mfb(IF_VARSIZE));

ARMOR("pants_leather", "leather pants",	60, 210,C_PANTS,	LEATHER,	MNULL,
    6,  8, -2,  1,  2,  1,  7,  0,  50,  2,	mfb(bp_legs), "\
A pair of black leather pants. Very tough, but cumbersome and without much\n\
storage.", mfb(IF_VARSIZE));

ARMOR("pants_cargo", "cargo pants",	70, 280,C_PANTS,	COTTON,		MNULL,
    6,  6, -3,  0,  1,  0,  2,  0,  20, 12,	mfb(bp_legs), "\
A pair of pants lined with pockets, offering lots of storage.", mfb(IF_VARSIZE));

ARMOR("pants_army", "army pants",	30, 315,C_PANTS,	COTTON,		MNULL,
    6,  7, -2,  0,  1,  0,  3,  0,  40, 14,	mfb(bp_legs), "\
A tough pair of pants lined with pockets. Favored by the military.", mfb(IF_VARSIZE));

ARMOR("pants_ski", "ski pants",	60, 300,C_PANTS,	COTTON,		MNULL,
    10,  6, -3,  0,  2,  2,  0,  3,  80, 4,	mfb(bp_legs), "\
A pair of pants meant for alpine skiing.", mfb(IF_VARSIZE));

ARMOR("pants_fur", "fur pants",	60, 300,C_PANTS,	COTTON,		FUR,
    10,  6, -3,  1,  2,  2,  0,  3,  80, 4,	mfb(bp_legs), "\
A hefty pair of fur-lined pants.", mfb(IF_VARSIZE));

ARMOR("long_underpants", "long underwear",	40, 200,C_PANTS,	COTTON,		MNULL,
    4,  2, -3,  0,  0,  0,  0,  0,  30,  0,	mfb(bp_legs), "\
A pair of long underwear that help to maintain body temperature.", mfb(IF_VARSIZE));

ARMOR("skirt", "skirt",		75, 120,C_PANTS,	COTTON,		MNULL,
    2,  2, -5,  0, -1,  0,  0,  0,  0,  1,	mfb(bp_legs), "\
A short, breezy cotton skirt. Easy to move in, but only has a single small\n\
pocket.", mfb(IF_VARSIZE));

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("jumpsuit", "jumpsuit",	20, 200,C_BODY,		COTTON,		PLASTIC,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    6,  6, -3, -3,  0,  0,  1,  0,  10, 8,	mfb(bp_legs)|mfb(bp_torso), "\
A thin, short-sleeved jumpsuit; similar to those\n\
worn by prisoners. Provides decent storage and is\n\
not very encumbering.", mfb(IF_VARSIZE));

ARMOR("wolfsuit", "wolf suit", 4, 200, C_BODY,  COTTON,     MNULL,
    18,  19, -3, -3, 1,  3,  7,  2,  50,  4,
    mfb(bp_legs)|mfb(bp_torso)|mfb(bp_arms)|mfb(bp_hands)|mfb(bp_head)|mfb(bp_feet)|mfb(bp_mouth)|mfb(bp_eyes), "\
A full body fursuit in the form of an anthropomorphic wolf. It is quite encumbering\n\
and has little storage but is very warm.", 0);

ARMOR("dress", "dress",		70, 180,C_BODY,		COTTON,		MNULL,
    8,  6, -5, -5,  3,  0,  1,  0,  20,  0,	mfb(bp_legs)|mfb(bp_torso), "\
A long cotton dress. Difficult to move in and lacks any storage space.", mfb(IF_VARSIZE));

ARMOR("armor_chitin", "chitinous armor", 1,1200,C_BODY,		FLESH,		MNULL,
   70, 10,  2, -5,  2,  8, 14,  0,  10,  0,	mfb(bp_legs)|mfb(bp_torso), "\
Leg and body armor made from the exoskeletons of insects. Light and durable.", 0);

ARMOR("suit", "suit",		60, 180,C_BODY,		COTTON,		MNULL,
   10,  7, -5, -5,  1,  0,  1,  0,  25,  10,	mfb(bp_legs)|mfb(bp_torso)|mfb(bp_arms), "\
A full-body cotton suit. Makes the apocalypse a truly gentlemanly\n\
experience.", mfb(IF_VARSIZE));

ARMOR("hazmat_suit", "hazmat suit",	10,1000,C_BODY,		PLASTIC,	MNULL,
   20, 8, -5,  -8,  4,  0,  0, 10,  20, 12,	mfb(bp_legs)|mfb(bp_torso)|mfb(bp_arms), "\
A hazardous materials suit. Though quite bulky and cumbersome, wearing it\n\
will provide excellent protection against ambient radiation.", mfb(IF_VARSIZE));

ARMOR("armor_plate", "plate mail",	 2, 700,C_BODY,		IRON,		MNULL,
   70,140,  8, -5,  5, 16, 20,  0,  20,  0,	mfb(bp_torso)|mfb(bp_legs)|mfb(bp_arms), "\
An extremely heavy ornamental suit of armor.", mfb(IF_VARSIZE));

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("tshirt", "t shirt",	80,  80,C_TORSO,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    3,  2, -5,  0,  1,  0,  0,  0,  10,  0,	mfb(bp_torso), "\
A short-sleeved cotton shirt.", mfb(IF_VARSIZE));

ARMOR("polo_shirt", "polo shirt",	65,  95,C_TORSO,	COTTON,		MNULL,
    3,  2, -5,  0,  1,  0,  1,  0,  20,  0,	mfb(bp_torso), "\
A short-sleeved cotton shirt, slightly thicker than a t-shirt.", mfb(IF_VARSIZE));

ARMOR("dress_shirt", "dress shirt",	60, 115,C_TORSO,	COTTON,		MNULL,
    3,  3, -5,  0,  1,  0,  1,  0,  10,  1,	mfb(bp_torso)|mfb(bp_arms), "\
A white button-down shirt with long sleeves. Looks professional!", mfb(IF_VARSIZE));

ARMOR("tank_top", "tank top",	50,  75,C_TORSO,	COTTON,		MNULL,
    1,  1, -5,  0,  0,  0,  0,  0,  0,  0,	mfb(bp_torso), "\
A sleeveless cotton shirt. Very easy to move in.", mfb(IF_VARSIZE));

ARMOR("sweatshirt", "sweatshirt",	75, 110,C_TORSO,	COTTON,		MNULL,
    9,  5, -5,  0,  1,  1,  2,  0,  30,  0,	mfb(bp_torso)|mfb(bp_arms), "\
A thick cotton shirt. Provides warmth and a bit of padding.", mfb(IF_VARSIZE));

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("sweater", "sweater",	75, 105,C_TORSO,	WOOL,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    8,  5, -5,  0,  1,  1,  0,  0,  40,  0,	mfb(bp_torso)|mfb(bp_arms), "\
A wool shirt. Provides warmth.", mfb(IF_VARSIZE));

ARMOR("hoodie", "hoodie",		65, 130,C_TORSO,	COTTON,		MNULL,
   10,  5, -5,  0,  1,  1,  1,  0,  30,  9,	mfb(bp_torso)|mfb(bp_arms), "\
A sweatshirt with a hood and a \"kangaroo pocket\" in front for storage.", mfb(IF_VARSIZE));

ARMOR("under_armor", "under armor", 20, 200,C_TORSO,	COTTON,		MNULL,
   2,  2, -5,  0, 0,  0,  0,  0,  20,  0,	mfb(bp_torso), "\
Sports wear that clings to your chest to maintain body temperature.", mfb(IF_VARSIZE));

ARMOR("jacket_light", "light jacket",	50, 105,C_TORSO,	COTTON,		MNULL,
    6,  4, -5,  0,  1,  0,  2,  0,  20,  4,	mfb(bp_torso)|mfb(bp_arms), "\
A thin cotton jacket. Good for brisk weather.", mfb(IF_VARSIZE));

ARMOR("jacket_jean", "jean jacket",	35, 120,C_TORSO,	COTTON,		MNULL,
    7,  5, -3,  0,  1,  0,  4,  0,  20,  3,	mfb(bp_torso)|mfb(bp_arms), "\
A jacket made from denim. Provides decent protection from cuts.", mfb(IF_VARSIZE));

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("blazer", "blazer",		35, 120,C_TORSO,	WOOL,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   10,  6, -4,  0,  2,  0,  3,  0,  30,  2,	mfb(bp_torso)|mfb(bp_arms), "\
A professional-looking wool blazer. Quite cumbersome.", mfb(IF_VARSIZE));

ARMOR("jacket_leather", "leather jacket",	30, 150,C_TORSO,	LEATHER,	MNULL,
   14, 14, -2,  1,  2,  1,  9,  1,  40,  4,	mfb(bp_torso)|mfb(bp_arms), "\
A jacket made from thick leather. Cumbersome, but offers excellent\n\
protection from cuts.", mfb(IF_VARSIZE));

ARMOR("kevlar", "kevlar vest",	30, 800,C_TORSO,	KEVLAR,		MNULL,
   24, 24,  6, -3,  2,  4, 22,  0,  20,  4,	mfb(bp_torso), "\
A heavy bulletproof vest. The best protection from cuts and bullets.", mfb(IF_VARSIZE));

ARMOR("coat_rain", "rain coat",	50, 100,C_TORSO,	COTTON,	PLASTIC,
    9,  8, -4,  0,  2,  0,  3,  1,  20,  7,	mfb(bp_torso)|mfb(bp_arms), "\
A plastic coat with two very large pockets. Provides protection from rain.", mfb(IF_VARSIZE));

ARMOR("poncho", "wool poncho",	15, 120,C_TORSO,	WOOL,		MNULL,
    7,  3, -5, -1,  0,  1,  2,  1,  35,  0,	mfb(bp_torso), "\
A simple wool garment worn over the torso. Provides a bit of protection.", mfb(IF_VARSIZE));

//     NAME		RARE	COLOR		MAT1		MAT2
ARMOR("trenchcoat", "trenchcoat",	25, 225,C_TORSO,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   10,  6, -5, -1,  1,  0,  1,  1,  20, 24,	mfb(bp_torso)|mfb(bp_arms), "\
A thin cotton trenchcoat, lined with pockets. Great for storage.", mfb(IF_VARSIZE));

//     NAME		RARE	COLOR		MAT1		MAT2
ARMOR("trenchcoat_leather", "leather trenchcoat",	25, 225,C_TORSO,	LEATHER,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   16,  10, -5, -1,  2,  1,  9,  1,  50, 24,	mfb(bp_torso)|mfb(bp_arms), "\
A thick leather trenchcoat, lined with pockets. Great for storage.", mfb(IF_VARSIZE));

ARMOR("trenchcoat_fur", "fur trenchcoat",	25, 225,C_TORSO,	FUR,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   16,  10, -5, -1,  2,  1,  9,  1,  50, 24,	mfb(bp_torso)|mfb(bp_arms), "\
A thick fur trenchcoat, lined with pockets. Great for storage.", mfb(IF_VARSIZE));

ARMOR("coat_winter", "winter coat",	50, 160,C_TORSO,	COTTON,		MNULL,
   12,  6, -5, -2,  3,  3,  1,  1,  70, 12,	mfb(bp_torso)|mfb(bp_arms), "\
A padded coat with deep pockets. Very warm.", mfb(IF_VARSIZE));

ARMOR("coat_fur", "fur coat",	 5, 550,C_TORSO,	FUR,		LEATHER,
   18, 12, -5, -5,  2,  4,  2,  2, 80,  4,	mfb(bp_torso)|mfb(bp_arms), "\
A fur coat with a couple small pockets. Extremely warm.", mfb(IF_VARSIZE));

ARMOR("peacoat", "peacoat",	30, 180,C_TORSO,	COTTON,		MNULL,
   16, 10, -4, -3,  2,  1,  2,  0,  70, 10,	mfb(bp_torso)|mfb(bp_arms), "\
A heavy cotton coat. Cumbersome, but warm and with deep pockets.", mfb(IF_VARSIZE));

ARMOR("vest", "utility vest",	15, 200,C_TORSO,	COTTON,		MNULL,
    4,  3, -3,  0,  0,  0,  1,  0,  5, 14,	mfb(bp_torso), "\
A light vest covered in pockets and straps for storage.", 0);

ARMOR("beltrig", "belt rig",	10, 200,C_TORSO,	COTTON,		MNULL,
    4,  4, -3,  0,  0,  0,  1,  0,  5, 18,	mfb(bp_torso), "\
A light vest covered in webbing, pockets and straps.\n\
This variety is favoured by the military.", 0);

ARMOR("coat_lab", "lab coat",	20, 155,C_TORSO,	COTTON,		MNULL,
   11,  7, -3, -2,  1,  1,  2,  0,  10, 14,	mfb(bp_torso)|mfb(bp_arms), "\
A long white coat with several large pockets.", 0);

// arm guards
//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("armguard_soft", "soft arm sleeves",	40,  65,C_ARMS,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    0,  0, -5,  1,  0,  1,  1,  1,  30,  0,	mfb(bp_arms), "\
A pair of soft neoprene arm sleeves, often used in contact sports.", 0);

ARMOR("armguard_hard", "hard arm guards",	20,  130,C_ARMS,	COTTON,		PLASTIC,
    1,  0, -5,  1,  1,  2,  2,  1,  20,  0,	mfb(bp_arms), "\
A pair of neoprene arm sleeves covered with molded plastic sheaths.", 0);

ARMOR("armguard_chitin", "chitin arm guards",	10,  200,C_ARMS,	FLESH,		MNULL,
    2,  0, -5,  1,  1,  3,  3,  2,  10,  0,	mfb(bp_arms), "\
A pair of arm guards made from the exoskeletons of insects. Light and durable.", 0);

ARMOR("armguard_metal", "metal arm guards",	10,  400,C_ARMS,	IRON,		MNULL,
    1,  1, -5,  1,  1,  4,  4,  1,  0,  0,	mfb(bp_arms), "\
A pair of arm guards hammered out from metal. Very stylish.", 0);

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("gloves_liner", "glove liners",	25,  100,C_GLOVES,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    0,  0, -5,  1,  0,  0,  0,  0,  10,  0,	mfb(bp_hands), "\
A pair of thin cotton gloves. Often used as a liner beneath other gloves.", mfb(IF_VARSIZE));

ARMOR("gloves_light", "light gloves",	35,  65,C_GLOVES,	COTTON,		MNULL,
    1,  0, -5,  1,  1,  0,  0,  0,  30,  0,	mfb(bp_hands), "\
A pair of cotton gloves.", 0);

ARMOR("mittens", "mittens",	30,  40,C_GLOVES,	WOOL,		MNULL,
    2,  0, -5,  1,  8,  0,  1,  0,  90,  0,	mfb(bp_hands), "\
A pair of warm mittens. They are extremely cumbersome.", 0);

ARMOR("gloves_fur", "fur gloves",	30,  40,C_GLOVES,	FUR,		MNULL,
    2,  0, -5,  1,  3,  0,  1,  0,  70,  0,	mfb(bp_hands), "\
A pair of warm fur gloves. They are somewhat cumbersome.", 0);

ARMOR("gloves_wool", "wool gloves",	33,  50,C_GLOVES,	WOOL,		MNULL,
    1,  0, -5,  1,  3,  0,  1,  0,  60,  0,	mfb(bp_hands), "\
A thick pair of wool gloves. Cumbersome but warm.", 0);

ARMOR("gloves_winter", "winter gloves",	40,  65,C_GLOVES,	COTTON,		MNULL,
    2,  0, -5,  1,  5,  1,  1,  0,  70,  0,	mfb(bp_hands), "\
A pair of padded gloves. Cumbersome but warm.", 0);

ARMOR("gloves_leather", "leather gloves",	45,  85,C_GLOVES,	LEATHER,	MNULL,
    1,  1, -3,  2,  1,  0,  3,  0,  40,  0,	mfb(bp_hands), "\
A thin pair of leather gloves. Good for doing manual labor.", 0);

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("gloves_fingerless", "fingerless gloves",20,90,C_GLOVES,	LEATHER,	MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    1,  0, -3,  2,  0,  0,  2,  0,  5,  0,	mfb(bp_hands), "\
A pair of leather gloves with no fingers, allowing greater manual dexterity.", 0);

ARMOR("gloves_rubber", "rubber gloves",	20,  30,C_GLOVES,	PLASTIC,	MNULL,
    1,  1, -3,  2,  3,  0,  1,  2,  5,  0,	mfb(bp_hands), "\
A pair of rubber gloves, often used while cleaning with caustic materials.", 0);

ARMOR("gloves_medical", "medical gloves",	70,  10,C_GLOVES,	PLASTIC,	MNULL,
    0,  0, -5,  1,  0,  0,  0,  1,  0,  0,	mfb(bp_hands), "\
A pair of thin latex gloves, designed to limit the spread of disease.", 0);

ARMOR("fire_gauntlets", "fire gauntlets",	 5,  95,C_GLOVES,	LEATHER,	MNULL,
    3,  5, -2,  2,  6,  1,  2,  5,  40,  0,	mfb(bp_hands), "\
A heavy pair of leather gloves, used by firefighters for heat protection.", 0);

ARMOR("gauntlets_chitin", "chitinous gauntlets", 1, 380,C_GLOVES,		FLESH,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   4,   1,  2, -2,  1,  5, 7,   4,  20,  0,	mfb(bp_hands), "\
Gauntlets made from the exoskeletons of insects. Very light and durable.", 0);

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("mask_dust", "dust mask",	65,  20,C_MOUTH,	COTTON,		IRON,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    0,  0, -5, -3,  1,  0,  0,  2,  10,  0,	mfb(bp_mouth), "\
A simple piece of cotton that straps over the mouth. Provides a small amount\n\
of protection from air-borne illness and dust.", 0);

ARMOR("bandana", "bandana",	35,  28,C_MOUTH,	COTTON, 	MNULL,
    1,  0, -4, -1,  0,  0,  0,  1,  30,  0,	mfb(bp_mouth), "\
A cotton bandana, worn over the mouth for warmth and minor protection from\n\
dust and other contaminants.", 0);

ARMOR("scarf", "scarf",		45,  40,C_MOUTH,	WOOL,   	MNULL,
    2,  3, -5, -3,  1,  1,  0,  2,  60,  0,	mfb(bp_mouth), "\
A long wool scarf, worn over the mouth for warmth.", 0);

ARMOR("scarf_fur", "scarf",		45,  40,C_MOUTH,	FUR,   	MNULL,
    2,  3, -5, -3,  1,  1,  0,  2,  60,  0,	mfb(bp_mouth), "\
A long wool scarf, worn over the mouth for warmth.", 0);

ARMOR("mask_filter", "filter mask",	30,  80,C_MOUTH,	PLASTIC,	MNULL,
    3,  6,  1,  1,  2,  1,  1,  7,  20,  0,	mfb(bp_mouth), "\
A mask that straps over your mouth and nose and filters air. Protects from\n\
smoke, dust, and other contaminants quite well.", 0);

ARMOR("mask_gas", "gas mask",	10, 240,C_MOUTH,	PLASTIC,	MNULL,
    6,  8,  0, -3,  4,  1,  2, 16,  40,  0,	mfb(bp_mouth)|mfb(bp_eyes), "\
A full gas mask that covers the face and eyes. Provides excellent protection\n\
from smoke, teargas, and other contaminants.", 0);

// Eyewear - Encumberment is its effect on your eyesight.
// Environment is the defense to your eyes from noxious fumes etc.


//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("glasses_eye", "eyeglasses",	90, 150,C_EYES,		GLASS,		PLASTIC,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    1,  0, -3, -2,  0,  0,  1,  1,  0,  0,	mfb(bp_eyes), "\
A pair of glasses for the near-sighted. Useless for anyone else.", 0);

ARMOR("glasses_reading", "reading glasses",90,  80,C_EYES,		GLASS,		PLASTIC,
    1,  0, -3, -2,  0,  0,  1,  1,  0,  0,	mfb(bp_eyes), "\
A pair of glasses for the far-sighted. Useless for anyone else.", 0);

ARMOR("glasses_safety", "safety glasses", 40, 100,C_EYES,		PLASTIC,	MNULL,
    1,  0, -5, -2,  0,  2,  4,  1,  0,  0,	mfb(bp_eyes), "\
A pair of plastic glasses, used in workshops, sports, chemistry labs, and\n\
many other places. Provides great protection from damage.", 0);

ARMOR("goggles_swim", "swim goggles",	50, 110,C_EYES,		PLASTIC,	MNULL,
    1,  0, -5, -2,  2,  1,  2,  4,  10,  0,	mfb(bp_eyes), "\
A small pair of goggles. Distorts vision above water, but allows you to see\n\
much further under water.", 0);

ARMOR("goggles_ski", "ski goggles",	30, 175,C_EYES,		PLASTIC,	MNULL,
    2,  1, -4, -2,  1,  1,  2,  6,  60,  0,	mfb(bp_eyes), "\
A large pair of goggles that completely seal off your eyes. Excellent\n\
protection from environmental dangers.", 0);

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("goggles_welding", "welding goggles", 70, 240,C_EYES,		GLASS,  	STEEL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    2,  4, -1, -3,  6,  2,  5,  6,  10,  0,	mfb(bp_eyes), "\
A dark pair of goggles. They make seeing very difficult, but protect you\n\
from bright flashes.", 0);

ARMOR("goggles_nv", "light amp goggles",1,920,C_EYES,		STEEL,		GLASS,
    3,  6,  1, -2,  2,  2,  3,  6,  20,  0,	mfb(bp_eyes), "\
A pair of goggles that amplify ambient light, allowing you to see in the\n\
dark.  You must be carrying a powered-on unified power supply, or UPS, to use\n\
them.", 0);

ARMOR("glasses_monocle", "monocle",	 2, 200,C_EYES,		GLASS,		PLASTIC,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    1,  0, -3, -2,  0,  0,  1,  1,  0,  0,	mfb(bp_eyes), "\
An essential article of the gentleman's apparel. Also negates near-sight.", 0);

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("sunglasses", "sunglasses",	90, 150,C_EYES,		GLASS,		PLASTIC,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    1,  0, -3, -2,  0,  0,  1,  1,  0,  0,	mfb(bp_eyes), "\
A pair of sunglasses, good for keeping the glare out of your eyes.", 0);

// Headwear encumberment should ONLY be 0 if it's ok to wear with another
// Headwear environmental protection (ENV) drops through to eyes

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("hat_ball", "baseball cap",	30,  35,C_HAT,		COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    2,  1, -5,  0,  0,  0,  0,  2,  10,  0,	mfb(bp_head), "\
A Red Sox cap. It provides a little bit of warmth.", 0);

ARMOR("hat_boonie", "boonie hat",	10,  55,C_HAT,		COTTON,	MNULL,
    2,  1, -5,  0,  0,  0,  1,  2,  20,  0,	mfb(bp_head), "\
Also called a \"bucket hat.\" Often used in the military.", 0);

ARMOR("hat_cotton", "cotton hat",	45,  40,C_HAT,		COTTON,		MNULL,
    2,  1, -5,  0,  0,  0,  0,  0,  30,  0,	mfb(bp_head), "\
A snug-fitting cotton hat. Quite warm.", 0);

ARMOR("hat_knit", "knit hat",	25,  50,C_HAT,		WOOL,		MNULL,
    2,  1, -5,  0,  0,  1,  0,  0,  40,  0,	mfb(bp_head), "\
A snug-fitting wool hat. Very warm.", 0);

ARMOR("hat_hunting", "hunting cap",	20,  80,C_HAT,		WOOL,		MNULL,
    3,  2, -5,  0,  0,  0,  1,  2,  60,  0,	mfb(bp_head), "\
A red plaid hunting cap with ear flaps. Notably warm.", 0);

ARMOR("hat_fur", "fur hat",	15, 120,C_HAT,		FUR,		LEATHER,
    4,  2, -5,  0,  1,  2,  2,  0,  80,  0,	mfb(bp_head), "\
A hat made from the pelts of animals. Extremely warm.", 0);

ARMOR("balclava", "balaclava",	15, 100,C_HAT,		COTTON,		MNULL,
    4,  2, -5,  0,  0,  0,  0,  0,  30,  0,	mfb(bp_head)|mfb(bp_mouth), "\
A warm covering that protects the head and face from cold.", 0);

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("hat_hard", "hard hat",	50, 125,C_HAT,		PLASTIC,	MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    8,  4,  6,  0,  1,  4,  5,  0,  10,  0,	mfb(bp_head), "\
A hard plastic hat worn in constructions sites. Excellent protection from\n\
cuts and percussion.", 0);
TECH("hat_hard", mfb(TEC_WBLOCK_1) );

ARMOR("helmet_bike", "bike helmet",	35, 140,C_HAT,		PLASTIC,	MNULL,
   12,  2,  4,  0,  1,  8,  2,  0,  20,  0,	mfb(bp_head), "\
A thick foam helmet. Designed to protect against concussion.", 0);
TECH("hat_hard", mfb(TEC_WBLOCK_1) );

ARMOR("helmet_skid", "skid lid",	30, 190,C_HAT,		PLASTIC,	IRON,
   10,  5,  8,  0,  2,  6, 16,  0,  10,  0,	mfb(bp_head), "\
A small metal helmet that covers the head and protects against cuts and\n\
percussion.", 0);
TECH("helmet_skid", mfb(TEC_WBLOCK_1) );

ARMOR("helmet_ball", "baseball helmet",45, 195,C_HAT,		PLASTIC,	IRON,
   14,  6,  7, -1,  2, 10, 10,  1,  15,  0,	mfb(bp_head), "\
A hard plastic helmet that covers the head and ears. Designed to protect\n\
against a baseball to the head.", 0);
TECH("helmet_ball", mfb(TEC_WBLOCK_1) );

ARMOR("helmet_army", "army helmet",	40, 480,C_HAT,		PLASTIC,	IRON,
   16,  8, 10, -1,  2, 12, 28,  0,  25,  0,	mfb(bp_head), "\
A heavy helmet that provides excellent protection from all sorts of damage.", 0);
TECH("helmet_army", mfb(TEC_WBLOCK_1) );

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("helmet_riot", "riot helmet",	25, 420,C_HAT,		PLASTIC,	IRON,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   20,  7,  8, -1,  2,  6, 28,  2,  20,  0,	mfb(bp_head)|mfb(bp_eyes)|
						mfb(bp_mouth), "\
A helmet with a plastic shield that covers your entire face.", 0);
TECH("helmet_riot", mfb(TEC_WBLOCK_1) );

ARMOR("helmet_motor", "motorcycle helmet",40,325,C_HAT,		PLASTIC,	IRON,
   24,  8,  7, -1,  3,  8, 20,  1,  30,  0,	mfb(bp_head)|mfb(bp_mouth), "\
A helmet with covers for your head and chin, leaving space in-between for you\n\
to wear goggles.", 0);
TECH("helmet_motor", mfb(TEC_WBLOCK_1) );

ARMOR("helmet_chitin", "chitinous helmet", 1, 380,C_HAT,		FLESH,		MNULL,
   22,  1,  2, -2,  1, 10, 14,  4,  20,  0,	mfb(bp_head)|mfb(bp_eyes)|
						mfb(bp_mouth), "\
A helmet made from the exoskeletons of insects. Covers the entire head; very\n\
light and durable.", 0);

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("helmet_plate", "great helm",	  1,400,C_HAT,		IRON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    20, 15, 10,  0,  4, 10, 15,  1,  10,  0,	mfb(bp_head)|mfb(bp_eyes)|
						mfb(bp_mouth), "\
A medieval helmet that provides excellent protection to the entire head, at\n\
the cost of great encumbrance.", 0);
TECH("helmet_plate", mfb(TEC_WBLOCK_1) );

ARMOR("tophat", "top hat",	10,  55,C_HAT,		COTTON,	MNULL,
    2,  1, -5,  0,  0,  0,  1,  1,  10,  0,	mfb(bp_head), "\
The only hat for a gentleman. Look exquisite while laughing in the face\n\
of danger!", 0);

ARMOR("backpack", "backpack",	38, 210,C_STORE,	COTTON,	PLASTIC,
   10,  2, -4,  0,  1,  0,  0,  0,  0, 40,	mfb(bp_torso), "\
A small backpack, good storage for a little encumbrance.", 0);

ARMOR("rucksack", "military rucksack",	20, 210,C_STORE,	PLASTIC,	MNULL,
   14,  3, -4,  0,  2,  0,  0,  0,  0, 80,	mfb(bp_torso), "\
A huge military rucksack, provides a lot of storage.", 0);

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("purse", "purse",		40,  75,C_STORE,	LEATHER,	MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   10,  3,  2,  2,  1,  0,  0,  0,  0, 20,	mfb(bp_torso), "\
A bit cumbersome to wear, but provides some storage", 0);

ARMOR("mbag", "messenger bag",	20, 110,C_STORE,	COTTON,	PLASTIC,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    8,  2,  1,  1,  0,  0,  0,  0,  0, 20,	mfb(bp_torso), "\
Light and easy to wear, but doesn't offer much storage.", 0);

ARMOR("fanny", "fanny pack", 	10, 100,C_STORE,	COTTON,	PLASTIC,
    3,  1,  1,  2,  0,  0,  0,  0,  0,  6,	0, "\
Provides a bit of extra storage without encumbering you at all.", 0);

ARMOR("holster", "holster",	 8,  90,C_STORE,	LEATHER,	MNULL,
    2,  2,  2, -1,  0,  0,  0,  0,  0,  3,	0, "\
Provides a bit of extra storage without encumbering you at all.", 0);

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("bootstrap", "bootstrap",	 3,  80,C_STORE, 	LEATHER,	MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    1,  1, -1, -1,  0,  0,  0,  0,  1,  2,	mfb(bp_legs), "\
A small holster worn on the ankle.", 0);

ARMOR("ragpouch", "pouch",	20, 110,C_STORE,	COTTON,	MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    5,  2,  1,  1,  1,  0,  0,  0,  0, 12,	mfb(bp_torso), "\
A makeshift bag, cobbled together from rags. Really gets in the way, but\n\
provides a decent amount of storage.", 0);

ARMOR("leather_pouch", "leather pouch",	20, 110,C_STORE,	LEATHER,	MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    5,  2,  1,  1,  0,  0,  0,  0,  0, 12,	mfb(bp_torso), "\
A bag stitched together from leather scraps. Doesn't hold an awful lot\n\
but is easy to wear.", 0);

ARMOR("ring", "gold ring",	12, 600,C_DECOR,	SILVER,		MNULL,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,	0,	"\
A flashy gold ring. You can wear it if you like, but it won't provide\n\
any effects.", 0);

ARMOR("necklace", "silver necklace",14, 500,C_DECOR,	SILVER,		MNULL,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,	0,	"\
A nice silver necklace. You can wear it if you like, but it won't provide\n\
any effects.", 0);

// Over the body clothing

//     NAME		RARE	COLOR		MAT1		MAT2
ARMOR("blanket", "blanket",	20, 225,C_TORSO,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   10,  3, -5, -1,  5,  0,  1,  1,  60, 0,
   mfb(bp_torso)|mfb(bp_arms)|mfb(bp_hands)|mfb(bp_legs)|mfb(bp_feet), "\
Hiding under here will not protect you from the monsters.", mfb(IF_VARSIZE));

ARMOR("fur_blanket", "fur blanket",	20, 225,C_TORSO,	COTTON,		FUR,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   10,  10, -5, -1,  5,  0,  1,  1,  80, 0,
   mfb(bp_torso)|mfb(bp_arms)|mfb(bp_hands)|mfb(bp_legs)|mfb(bp_feet), "\
A heavy fur blanket that covers most of your body.", mfb(IF_VARSIZE));

ARMOR("emer_blanket", "emergency blanket",	20, 225,C_TORSO,	WOOL,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   4,  2, -5, -1,  5,  0,  1,  1,  50, 0,
   mfb(bp_torso)|mfb(bp_arms)|mfb(bp_legs), "\
A compact wool blanket that covers your most important body parts.", mfb(IF_VARSIZE));

ARMOR("sleeping_bag", "sleeping bag",	10, 225,C_TORSO,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   10,  5, -5, -1,  5,  0,  1,  1,  80, 0,
   mfb(bp_torso)|mfb(bp_head)|mfb(bp_mouth)|mfb(bp_arms)|mfb(bp_hands)|mfb(bp_legs)|mfb(bp_feet), "\
A large sleeping bag that covers you head to toe.", mfb(IF_VARSIZE));

ARMOR("sleeping_bag_fur", "fur sleeping bag",	10, 225,C_TORSO,	COTTON,		FUR,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   10,  5, -5, -1,  5,  0,  1,  1,  100, 0,
   mfb(bp_torso)|mfb(bp_head)|mfb(bp_mouth)|mfb(bp_arms)|mfb(bp_hands)|mfb(bp_legs)|mfb(bp_feet), "\
A large sleeping bag lined with fur. Who needs a tent?", mfb(IF_VARSIZE));

ARMOR("house_coat", "house coat",	25, 225,C_TORSO,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   10,  6, -5, -1,  1,  0,  1,  1,  40, 6,
   mfb(bp_torso)|mfb(bp_arms)|mfb(bp_legs), "\
Makes you wish you had running water to take a shower.", mfb(IF_VARSIZE));

ARMOR("snuggie", "snuggie",	5, 225,C_TORSO,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   10,  6, -5, -1,  2,  0,  1,  1,  50, 0,
   mfb(bp_torso)|mfb(bp_arms)|mfb(bp_legs), "\
Perfect for reading all those books you stole.", mfb(IF_VARSIZE));

ARMOR("cloak", "cloak",	5, 225,C_TORSO,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   10,  15, -5, -1,  1,  2,  6,  3,  40, 0,
   mfb(bp_torso)|mfb(bp_head)|mfb(bp_arms)|mfb(bp_legs), "\
A heavy cloak that is thrown over your body.", mfb(IF_VARSIZE));

ARMOR("cloak_fur", "cloak",	5, 225,C_TORSO,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   10,  15, -5, -1,  1,  2,  6,  3,  70, 0,
   mfb(bp_torso)|mfb(bp_head)|mfb(bp_arms)|mfb(bp_legs), "\
A heavy fur cloak that is thrown over your body.", mfb(IF_VARSIZE));

ARMOR("cloak_leather", "cloak",	5, 225,C_TORSO,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   10,  25, -5, -1,  1,  2,  12,  3,  40, 0,
   mfb(bp_torso)|mfb(bp_head)|mfb(bp_arms)|mfb(bp_legs), "\
A heavy leather cloak that is thrown over your body. Provides decent protection", mfb(IF_VARSIZE));

ARMOR("jedi_cloak", "jedi cloak",	1, 225,C_TORSO,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   10,  14, -5, -1,  0,  5,  10,  5,  50, 0,
   mfb(bp_torso)|mfb(bp_head)|mfb(bp_arms)|mfb(bp_hands)|mfb(bp_legs), "\
Stylish cloak.", mfb(IF_VARSIZE));

#define POWER_ARMOR(id, name,rarity,price,color,mat1,mat2,volume,wgt,dam,to_hit,\
encumber,dmg_resist,cut_resist,env,warmth,storage,covers,des)\
itypes[id] = new it_armor(id,rarity,price,name,des,'[',\
  color,mat1,mat2,volume,wgt,dam,0,to_hit,0,covers,encumber,dmg_resist,cut_resist,\
  env,warmth,storage,true)

POWER_ARMOR("power_armor_basic", "basic power armor", 5, 1000, C_BODY, STEEL, MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   40, 60, 1, 1, 5, 32, 50, 10, 90, 0, mfb(bp_torso)|mfb(bp_arms)|mfb(bp_hands)|mfb(bp_legs)|mfb(bp_feet), "\
A heavy suit of basic power armor, offering very good protection against attacks, but hard to move in.");

POWER_ARMOR("power_armor_helmet_basic", "basic power armor helmet", 6, 500, C_HAT, STEEL, MNULL,
   10, 18, 1, 1, 5, 32, 50, 10, 90, 0, mfb(bp_head)|mfb(bp_eyes)|mfb(bp_mouth), "\
A basic helmet, designed for use with power armor. Offers excellent protection from both attacks and environmental hazards.");

POWER_ARMOR("power_armor_frame", "power armor hauling frame", 4, 1000, C_STORE, STEEL, MNULL,
    8, 12, 1, 1, 4, 0, 0, 0, 0, 120, 0, "\
A heavy duty hauling frame designed to interface with power armor.");

// AMMUNITION
// Material should be the wrapper--even though shot is made of iron, because
//   it can survive a dip in water and be okay, its material here is PLASTIC.
// dmg is damage done, in an average hit.  Note that the average human has
//   80 health.  Headshots do 8x damage; vital hits do 2x-4x; glances do 0x-1x.
// Weight and price is per 100 rounds.
// AP is a reduction in the armor of the target.
// Accuracy is in quarter-degrees, and measures the maximum this ammo will
//   contribute to the angle of difference.  HIGH ACC IS BAD.
// Recoil is cumulative between shots.  4 recoil = 1 accuracy.
// IMPORTANT: If adding a new AT_*** ammotype, add it to the ammo_name function
//   at the end of this file.
#define AMMO(id, name,rarity,price,ammo_type,color,mat,volume,wgt,dmg,AP,range,\
accuracy,recoil,count,des,effects) \
itypes[id]=new it_ammo(id,rarity,price,name,des,'=',\
color,mat,SOLID,volume,wgt,1,0,0,effects,ammo_type,dmg,AP,accuracy,recoil,range,count);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("battery", "batteries",	50, 120,AT_BATT,	c_magenta,	IRON,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 1,  1,  0,  0,  0,  0,  0, 100, "\
A set of universal batteries. Used to charge almost any electronic device.",
0);

AMMO("thread", "thread",          40, 50, AT_THREAD,      c_magenta,      COTTON,
         1,  1,  0,  0,  0,  0,  0, 50, "\
A small quantity of thread that could be used to refill a sewing kit.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("sinew", "sinew",	50, 120,AT_THREAD,	c_red,	   FLESH,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 1,  1,  0,  0,  0,  0,  0,  10, "\
A tough sinew cut from a corpse, useable as thread.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("plant_fibre", "plant fibre",	50, 120,AT_THREAD,	c_green,	   VEGGY,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 1,  1,  1,  0,  0,  0,  0,  10, "\
Tough thin fibres, taken from a plant. Can be used as thread.",
0);
AMMO("duct_tape", "duct tape",       60, 20, AT_NULL,    c_ltgray,       PLASTIC,
         2,  2,  0,  0,  0,  0,  0, 200, "\
A roll of incredibly strong tape. Its uses are innumerable.",
0);

AMMO("cable", "copper wire",       60, 20, AT_NULL,    c_ltgray,       PLASTIC,
         2,  2,  0,  0,  0,  0,  0, 200, "\
Plastic jacketed copper cable of the type used in small electronics.",
0);

AMMO("plut_cell", "plutonium cell",	10,1500,AT_PLUT,	c_ltgreen,	STEEL,
	 1,  1,  0,  0,  0,  0,  0, 5, "\
A nuclear-powered battery. Used to charge advanced and rare electronics.",
0);

AMMO("nail", "nails",		35,  60,AT_NAIL,	c_cyan,		IRON,
         1,  8,  4,  3,  3, 40,  4, 100, "\
A box of nails, mainly useful with a hammer.",
0);

AMMO("bb", "BB",		 8,  50,AT_BB,		c_pink,		STEEL,
	 1,  6,  2,  0, 12, 20,  0, 200, "\
A box of small steel balls. They deal virtually no damage.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("arrow_wood", "wood arrow",       7,100,AT_ARROW,        c_green,        WOOD,
//	VOL WGT DMG  AP RNG ACC REC COUNT
         2, 60,  8,  1, 10, 18,  0,  10, "\
A sharpened arrow carved from wood. It's light-weight, does little damage,\n\
and is so-so on accuracy. Stands a good chance of remaining intact once\n\
fired.",
0);

AMMO("arrow_cf", "carbon fiber arrow",5,300,AT_ARROW,       c_green,        PLASTIC,
         2, 30, 12,  2, 15, 14,  0,   8, "\
High-tech carbon fiber shafts and 100 grain broadheads. Very light weight,\n\
fast, and notoriously fragile.",
0);

AMMO("bolt_wood", "wood crossbow bolt",8,100,AT_BOLT,	c_green,	WOOD,
	 1, 40, 10,  1, 10, 16,  0,  15, "\
A sharpened bolt carved from wood. It's lighter than a steel bolt, but does\n\
less damage and is less accurate. Stands a good chance of remaining intact\n\
once fired.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("bolt_steel", "steel crossbow bolt",7,400,AT_BOLT,	c_green,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 1, 90, 20,  3, 14, 12,  0,  10, "\
A sharp bolt made from steel. Deadly in skilled hands. Stands an excellent\n\
chance of remaining intact once fired.",
0);

AMMO("shot_bird", "birdshot",	 8, 500,AT_SHOT,	c_red,		PLASTIC,
	 2, 25, 18,  0,  5,  2, 18,  25, "\
Weak shotgun ammunition. Designed for hunting birds and other small game, its\n\
applications in combat are very limited.",
mfb(AMMO_COOKOFF));

AMMO("shot_00", "00 shot",		 8, 800,AT_SHOT,	c_red,		PLASTIC,
	 2, 28, 50,  0,  6,  1, 26,  25, "\
A shell filled with iron pellets. Extremely damaging, plus the spread makes\n\
it very accurate at short range. Favored by SWAT forces.",
mfb(AMMO_COOKOFF));

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("shot_slug", "shotgun slug",	 6, 900,AT_SHOT,	c_red,		PLASTIC,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 2, 34, 50,  8, 12, 10, 28,  25, "\
A heavy metal slug used with shotguns to give them the range capabilities of\n\
a rifle. Extremely damaging but rather innaccurate. Works best in a shotgun\n\
with a rifled barrel.",
mfb(AMMO_COOKOFF));

AMMO("shot_he", "explosive slug",   0,1200,AT_SHOT,	c_red,		PLASTIC,
	 2, 30, 10,  0, 12, 12, 20,   5, "\
A shotgun slug loaded with concussive explosives. While the slug itself will\n\
not do much damage to its target, it will explode on contact.",
mfb(AMMO_EXPLOSIVE));

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("22_lr", ".22 LR",		 9, 250,AT_22,		c_ltblue,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 2,  2, 11,  0, 13, 14,  6, 100, "\
One of the smallest calibers available, the .22 Long Rifle cartridge has\n\
maintained popularity for nearly two centuries. Its minimal recoil, low cost\n\
and low noise are offset by its paltry damage.",
mfb(AMMO_COOKOFF));

AMMO("22_cb", ".22 CB",		 8, 180,AT_22,		c_ltblue,	STEEL,
	 2,  2,  5,  0, 10, 16,  4, 100, "\
Conical Ball .22 is a variety of .22 ammunition with a very small propellant\n\
charge, generally with no gunpowder, resulting in a subsonic round. It is\n\
nearly silent, but is so weak as to be nearly useless.",
mfb(AMMO_COOKOFF));

AMMO("22_ratshot", ".22 rat-shot",	 2, 230,AT_22,		c_ltblue,	STEEL,
	 2,  2,  4,  0,  3,  2,  4, 100, "\
Rat-shot is extremely weak ammunition, designed for killing rats, snakes, or\n\
other small vermin while being unable to damage walls. It has an extremely\n\
short range and is unable to injure all but the smallest creatures.",
mfb(AMMO_COOKOFF));

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("9mm", "9mm",		 8, 300,AT_9MM,		c_ltblue,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 2,  7, 18,  2, 14, 16, 13,  50, "\
9 millimeter parabellum is generally regarded as the most popular handgun\n\
cartridge, used by the majority of US police forces. It is also a very\n\
popular round in sub-machine guns.",
mfb(AMMO_COOKOFF));

AMMO("9mmP", "9mm +P",		 8, 380,AT_9MM,		c_ltblue,	STEEL,
	 1,  7, 20,  4, 14, 15, 14,  25, "\
Attempts to improve the ballistics of 9mm ammunition lead to high-pressure\n\
rounds. Increased velocity resullts in superior accuracy and damage.",
mfb(AMMO_COOKOFF));

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("9mmP2", "9mm +P+",		 8, 440,AT_9MM,		c_ltblue,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 1,  7, 22, 12, 16, 14, 15,  10, "\
A step beyond the high-pressure 9mm +P round, the +P+ has an even higher\n\
internal pressure that offers a degree of armor-penetrating ability.",
mfb(AMMO_COOKOFF));

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("762_25", "7.62mm Type P",8, 300, AT_762x25,	c_ltblue,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 2,  7, 15,  4,  12, 14, 10,  200, "\
This small caliber pistol round offers quite good armor penetration at the\n\
cost of slightly less damage. It is rarely used outside of the Chinese army.",
mfb(AMMO_COOKOFF));

AMMO("38_special", ".38 Special",	 7, 400,AT_38,		c_ltblue,	STEEL,
	 2, 10, 20,  0, 14, 16, 12,  50, "\
The .38 Smith & Wesson Special enjoyed popularity among US police forces\n\
throughout the 20th century. It is most commonly used in revolvers.",
mfb(AMMO_COOKOFF));

AMMO("38_super", ".38 Super",	 7, 450,AT_38,		c_ltblue,	STEEL,
	 1,  9, 25,  4, 16, 14, 14,  25, "\
The .38 Super is a high-pressure load of the .38 Special caliber. It is a\n\
popular choice in pistol competions for its high accuracy, while its stopping\n\
power keeps it popular for self-defense.",
mfb(AMMO_COOKOFF));

AMMO("10mm", "10mm Auto",	 4, 420,AT_40,		c_blue,		STEEL,
	 2,  9, 26, 10, 14, 18, 20,  50, "\
Originally used by the FBI, the organization eventually abandoned the round\n\
due to its high recoil. Although respected for its versatility and power, it\n\
has largely been supplanted by the downgraded .40 S&W.",
mfb(AMMO_COOKOFF));

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("40sw", ".40 S&W",		 7, 450,AT_40,		c_blue,		STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 2,  9, 22,  2, 14, 15, 16,  50, "\
The .40 Smith & Wesson round was developed as an alternative to 10mm Auto for\n\
the FBI after they complained of high recoil. It is as accurate as 9mm, but\n\
has greater stopping power, leading to widespread use in law enforcement.",
mfb(AMMO_COOKOFF));

AMMO("44magnum", ".44 Magnum",	 7, 580,AT_44,		c_blue,		STEEL,
	 2, 15, 36,  2, 16, 16, 22,  50, "\
Described (in 1971) by Dirty Harry as \"the most powerful handgun in the\n\
world,\" the .44 Magnum gained widespead popularity due to its depictions in\n\
the media. In reality, its intense recoil makes it unsuitable in most cases.",
mfb(AMMO_COOKOFF));

AMMO("45_acp", ".45 ACP",		 7, 470,AT_45,		c_blue,		STEEL,
	 2, 10, 32,  2, 16, 18, 18,  50, "\
The .45 round was one of the most popular and powerful handgun rounds through\n\
the 20th century. It features very good accuracy and stopping power, but\n\
suffers from moderate recoil and poor armor penetration.",
mfb(AMMO_COOKOFF));

AMMO("45_jhp", ".45 FMJ",		 4, 480,AT_45,		c_blue,		STEEL,
	 1, 13, 26, 20, 16, 18, 18,  25, "\
Full Metal Jacket .45 rounds are designed to overcome the poor armor\n\
penetration of the standard ACP round. However, they are less likely to\n\
expand upon impact, resulting in reduced damage overall.",
mfb(AMMO_COOKOFF));

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("45_super", ".45 Super",	 5, 520,AT_45,		c_blue,		STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 1, 11, 34,  8, 18, 16, 20,  10, "\
The .45 Super round is an updated variant of .45 ACP. It is overloaded,\n\
resulting in a great increase in muzzle velocity. This translates to higher\n\
accuracy and range, a minor armor piercing capability, and greater recoil.",
mfb(AMMO_COOKOFF));

AMMO("57mm", "5.7x28mm",	 3, 500,AT_57,		c_dkgray,	STEEL,
	 3,  2, 14, 30, 14, 12,  6, 100, "\
The 5.7x28mm round is a proprietary round developed by FN Hestal for use in\n\
their P90 SMG. While it is a very small round, comparable in power to .22,\n\
it features incredible armor-piercing capabilities and very low recoil.",
mfb(AMMO_COOKOFF));

AMMO("46mm", "4.6x30mm",	 2, 520,AT_46,		c_dkgray,	STEEL,
	 3,  1, 13, 35, 14, 12,  6, 100, "\
Designed by Heckler & Koch to compete with the 5.7x28mm round, 4.6x30mm is,\n\
like the 5.7, designed to minimize weight and recoil while increasing\n\
penetration of body armor. Its low recoil makes it ideal for automatic fire.",
mfb(AMMO_COOKOFF));

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("762_m43", "7.62x39mm M43",	 6, 500,AT_762,		c_dkgray,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 3,  7, 25,  8, 30, 19, 24,  80, "\
Designed during World War II by the Soviet Union, the popularity of the AK-47\n\
and the SKS contributed to the widespread adaption of the 7.62x39mm rifle\n\
round. However, due to its lack of yaw, this round deals less damage than most."
,
mfb(AMMO_COOKOFF));

AMMO("762_m87", "7.62x39mm M67",	 7, 650,AT_762,		c_dkgray,	STEEL,
	 3,  8, 28, 10, 30, 17, 25,  80, "\
The M67 variant of the popular 7.62x39mm rifle round was designed to improve\n\
yaw. This causes the round to tumble inside a target, causing significantly\n\
more damage. It is still outdone by shattering rounds.",
mfb(AMMO_COOKOFF));

AMMO("223", ".223 Remington",	 8, 620,AT_223,		c_dkgray,	STEEL,
	 2,  2, 36,  2, 36, 13, 30,  40, "\
The .223 rifle round is a civilian variant of the 5.56 NATO round. It is\n\
designed to tumble or fragment inside a target, dealing devastating damage.\n\
The lower pressure of the .223 compared to the 5.56 results in lower accuracy."
,
mfb(AMMO_COOKOFF));

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("556", "5.56 NATO",	 6, 650,AT_223,		c_dkgray,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 2,  4, 40,  8, 38, 10, 32,  40, "\
This rifle round has enjoyed widespread use in NATO countries, thanks to its\n\
very light weight and high damage. It is designed to shatter inside a\n\
target, inflicting massive damage.",
mfb(AMMO_COOKOFF));

AMMO("556_incendiary", "5.56 incendiary",	 2, 840,AT_223,		c_dkgray,	STEEL,
	 2,  4, 28, 18, 36, 11, 32, 30, "\
A variant of the widely-used 5.56 NATO round, incendiary rounds are designed\n\
to burn hotly upon impact, piercing armor and igniting flammable substances.",
mfb(AMMO_INCENDIARY));

AMMO("270", ".270 Winchester",	 8, 600,AT_3006,	c_dkgray,	STEEL,
	 1,  7, 42,  4, 80,  6, 34,  20, "\
Based off the military .30-03 round, the .270 rifle round is compatible with\n\
most guns that fire .30-06 rounds. However, it is designed for hunting, and\n\
is less powerful than the military rounds, with nearly no armor penetration.",
mfb(AMMO_COOKOFF));

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("3006", ".30-06 AP",	 4, 650,AT_3006,	c_dkgray,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 1, 12, 50, 30, 90,  7, 36,  10, "\
The .30-06 is a very powerful rifle round designed for long-range use. Its\n\
stupendous accuracy and armor piercing capabilities make it one of the most\n\
deadly rounds available, offset only by its drastic recoil and noise.",
mfb(AMMO_COOKOFF));

AMMO("3006_incendiary", ".30-06 incendiary", 1, 780,AT_3006,	c_dkgray,	STEEL,
	  1, 12, 35, 50, 90,  8, 35,  5, "\
A variant of the powerful .30-06 sniper round, incendiary rounds are designed\n\
to burn hotly upon impact, piercing armor and igniting flammable substances.",
mfb(AMMO_INCENDIARY));

AMMO("308", ".308 Winchester",	 7, 620,AT_308,		c_dkgray,	STEEL,
	 1,  9, 36,  2, 65,  7, 33,  20, "\
The .308 Winchester is a rifle round, the commercial equivalent of the\n\
military 7.62x51mm round. Its high accuracy and phenominal damage have made\n\
it the most poplar hunting round in the world.",
mfb(AMMO_COOKOFF));

AMMO("762_51", "7.62x51mm",	 6, 680,AT_308,		c_dkgray,	STEEL,
	 1,  9, 44,  8, 75,  6, 34,  20, "\
The 7.62x51mm largely replaced the .30-06 round as the standard military\n\
rifle round. It is lighter, but offers similar velocities, resulting in\n\
greater accuracy and reduced recoil.",
mfb(AMMO_COOKOFF));

//  NAME		   RAR PRC TYPE		COLOR		MAT
AMMO("762_51_incendiary", "7.62x51mm incendiary",6, 740,AT_308,	c_dkgray,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	  1,  9, 30, 25, 75,  6, 32,  10, "\
A variant of the powerful 7.62x51mm round, incendiary rounds are designed\n\
to burn hotly upon impact, piercing armor and igniting flammable substances.",
mfb(AMMO_INCENDIARY));

AMMO("laser_pack", "fusion pack",	 2, 800,AT_FUSION,	c_ltgreen,	PLASTIC,
	 1,  2, 12, 15, 30,  4,  0,  20, "\
In the middle of the 21st Century, military powers began to look towards\n\
energy based weapons. The result was the standard fusion pack, capable of\n\
delivering bolts of superheated gas at near light speed with no recoil.",
mfb(AMMO_INCENDIARY));

AMMO("40mm_concussive", "40mm concussive",     10,400,AT_40MM,	c_ltred,	STEEL,
	  1,200,  5,  0, 40,  8, 15,  4, "\
A 40mm grenade with a concussive explosion.",
mfb(AMMO_EXPLOSIVE));

//  NAME		   RAR PRC TYPE		COLOR		MAT
AMMO("40mm_frag", "40mm frag",           8, 450,AT_40MM,	c_ltred,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	  1,220,  5,  0, 40,  8, 15,  4, "\
A 40mm grenade with a small explosion and a high number of damaging fragments.",
mfb(AMMO_FRAG));

AMMO("40mm_incendiary", "40mm incendiary",     6, 500,AT_40MM,	c_ltred,	STEEL,
	  1,200,  5,  0, 40,  8, 15,  4, "\
A 40mm grenade with a small napalm load, designed to create a burst of flame.",
mfb(AMMO_NAPALM));

AMMO("40mm_teargas", "40mm teargas",        5, 450,AT_40MM,	c_ltred,	STEEL,
	  1,210,  5,  0, 40,  8, 15,  4, "\
A 40mm grenade with a teargas load. It will burst in a cloud of highly\n\
incapacitating gas.",
mfb(AMMO_TEARGAS));

AMMO("40mm_smoke", "40mm smoke cover",    4, 350,AT_40MM,	c_ltred,	STEEL,
	  1,210,  5,  0, 40,  8, 15,  6, "\
A 40mm grenade with a smoke load. It will burst in a cloud of harmless gas,\n\
and will also leave a streak of smoke cover in its wake.",
mfb(AMMO_SMOKE)|mfb(AMMO_TRAIL));

//  NAME		   RAR PRC TYPE		COLOR		MAT
AMMO("40mm_flashbang", "40mm flashbang",      8, 400,AT_40MM,	c_ltred,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	  1,210,  5,  0, 40,  8, 15,  6, "\
A 40mm grenade with a flashbang load. It will detonate with a blast of light\n\
and sound, designed to blind, deafen, and disorient anyone nearby.",
mfb(AMMO_FLASHBANG));

AMMO("66mm_HEAT", "66mm HEAT",0, 0, AT_66MM,        c_red,  STEEL,
//	VOL WGT DMG  AP  RNG ACC REC COUNT
     1,  1,  5,  20,  80,  8, 15, 1, "\
A 60mm High Explosive Anti Tank round. They can blow through up to two feet of concrete.",
mfb(AMMO_EXPLOSIVE_BIG)|mfb(AMMO_TRAIL));

AMMO("12mm", "H&K 12mm",	 2, 500,AT_12MM,		c_red,	STEEL,
	 1,  10, 25, 12, 70,  9, 7,  20, "\
The Heckler & Koch 12mm projectiles are used in H&K railguns. It's made of a\n\
ferromagnetic metal, probably cobalt.",
mfb(AMMO_COOKOFF));

AMMO("plasma", "hydrogen",	 8, 800,AT_PLASMA,	c_green,	STEEL,
	 10,  25, 35, 14,12,  4,  0,  25, "\
A canister of hydrogen. With proper equipment, it could be heated to plasma.",
mfb(AMMO_INCENDIARY));

// The following ammo type is charger rounds and subject to change wildly
//  NAME		   RAR PRC TYPE		COLOR		MAT
AMMO("charge_shot", "charge",	     0,  0,AT_NULL,	c_red,		MNULL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	  0,  0,  5,  0, 30,  8,  0, 1, "\
A weak plasma charge.",
0);

AMMO("shot_hull", "shotgun hull",		 10, 80,AT_NULL,	c_red,		PLASTIC,
	 0, 0, 0,  0,  0, 0,  0, 200, "\
An empty hull from a shotgun round.",
0);

AMMO("9mm_casing", "9mm casing",		 10, 30,AT_NULL,		c_ltblue,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 0,  0,  0,  0, 0,  0,  0,  100, "\
An empty casing from a 9mm round.",
0);

AMMO("22_casing", ".22 casing",	 10, 40,AT_NULL,		c_ltblue,	STEEL,
	 0, 0, 0,  0, 0, 0, 0, 200, "\
An empty casing from a .22 round.",
0);

AMMO("38_casing", ".38 casing",	 10, 40,AT_NULL,		c_ltblue,	STEEL,
	 0, 0, 0,  0, 0, 0, 0, 200, "\
An empty casing from a .38 round.",
0);

AMMO("40_casing", ".40 casing",		 10, 45,AT_NULL,		c_blue,		STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 0, 0, 0,  0, 0, 0, 0, 200, "\
An empty casing from a .40 round.",
0);

AMMO("44_casing", ".44 casing",	 10, 58,AT_NULL,		c_blue,		STEEL,
	 0, 0, 0,  0, 0, 0, 0, 200, "\
An empty casing from a .44 round.",
0);

AMMO("45_casing", ".45 casing",		 10, 47,AT_NULL,		c_blue,		STEEL,
	 0, 0, 0,  0, 0, 0, 0, 200, "\
An empty casing from a .45 round.",
0);

AMMO("57mm_casing", "5.7x28mm casing",	 10, 50,AT_NULL,		c_dkgray,	STEEL,
	 0, 0, 0, 0, 0,  0, 0, 200, "\
An empty casing from a 5.7x28mm round.",
0);

AMMO("46mm_casing", "4.6x30mm casing",	 10, 52,AT_NULL,		c_dkgray,	STEEL,
	 0, 0, 0, 0, 0,  0, 0, 200, "\
An empty casing from a 4.6x30mm round.",
0);

AMMO("762_casing", "7.62x39mm casing",	 10, 50,AT_NULL,		c_dkgray,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 0, 0, 0, 0, 0, 0, 0, 200, "\
An empty casing from a 7.62x39mm round.",
0);

AMMO("223_casing", ".223 casing",	 10, 72,AT_NULL,		c_dkgray,	STEEL,
	 0,  0, 0,  0, 0, 0, 0, 200, "\
An empty casing from a .223 round.",
0);

AMMO("3006_casing", ".30-06 casing",	 10, 90,AT_NULL,	c_dkgray,	STEEL,
	 0,  0, 0, 0, 0, 0, 0, 200, "\
An empty casing from a .30-06 round.",
0);

AMMO("308_casing", ".308 casing",	 10, 92,AT_NULL,		c_dkgray,	STEEL,
	 0,  0, 0,  0,  0, 0, 0, 200, "\
An empty casing from a .308 round.",
0);

AMMO("40mm_casing", "40mm canister",	 10, 92,AT_NULL,		c_dkgray,	STEEL,
	 0,  0, 0,  0,  0, 0, 0, 20, "\
A large canister from a spent 40mm grenade.",
0);

AMMO("gunpowder", "gunpowder",	 10, 30,AT_NULL,		c_dkgray,	POWDER,
	 0,  0, 0,  0,  0, 0, 0, 200, "\
Firearm quality gunpowder.",
mfb(AMMO_COOKOFF));

AMMO("shotgun_primer", "shotgun primer",	 10, 30,AT_NULL,		c_dkgray,	STEEL,
	 0,  0, 0, 0,  0, 0, 0, 200, "\
Primer from a shotgun round.",
mfb(AMMO_COOKOFF));

AMMO("smpistol_primer", "small pistol primer",	 15, 40,AT_NULL,		c_dkgray,	STEEL,
	 0,  0, 0, 0,  0, 0, 0, 200, "\
Primer from a small caliber  round.",
mfb(AMMO_COOKOFF));

AMMO("lgpistol_primer", "large pistol primer",	 15, 60,AT_NULL,		c_dkgray,	STEEL,
	 0,  0, 0,  0,  0, 0, 0, 200, "\
Primer from a large caliber pistol round.",
mfb(AMMO_COOKOFF));

AMMO("smrifle_primer", "small rifle primer",	 10, 60,AT_NULL,		c_dkgray,	STEEL,
	 0,  0,  0, 0,  0, 0, 0, 200, "\
Primer from a small caliber rifle round.",
mfb(AMMO_COOKOFF));

AMMO("lgrifle_primer", "large rifle primer",	 10, 80,AT_NULL,		c_dkgray,	STEEL,
	 0,  0,  0, 0,  0, 0, 0, 200, "\
Primer from a large caliber rifle round.",
mfb(AMMO_COOKOFF));

AMMO("lead", "lead",	 10, 50,AT_NULL,		c_dkgray,	STEEL,
	 0,  0,  0, 0,  0, 0, 0, 200, "\
Assorted bullet materials, useful in constructing a variety of ammunition.",
0);

AMMO("incendiary", "incendiary",	 0, 100,AT_NULL,		c_dkgray,	STEEL,
	 0,  0,  0, 0,  0, 0, 0, 200, "\
Material from an incendiary round, useful in constructing incendiary\n\
ammunition.",
mfb(AMMO_COOKOFF));


// FUEL
// Fuel is just a special type of ammo; liquid
#define FUEL(id, name,rarity,price,ammo_type,color,dmg,AP,range,accuracy,recoil,\
             count,des,effects) \
itypes[id]=new it_ammo(id,rarity,price,name,des,'~',\
color,MNULL,LIQUID,1,1,0,0,0,effects,ammo_type,dmg,AP,accuracy,recoil,range,count)
FUEL("gasoline","gasoline",	0,  50,   AT_GAS,	c_ltred,
//	DMG  AP RNG ACC REC COUNT
	 5,  5,  4,  0,  0,  200, "\
Gasoline is a highly flammable liquid. When under pressure, it has the\n\
potential for violent explosion.",
mfb(AMMO_FLAME)|mfb(AMMO_STREAM));

// GUNS
// ammo_type matches one of the ammo_types above.
// dmg is ADDED to the damage of the corresponding ammo.  +/-, should be small.
// aim affects chances of hitting; low for handguns, hi for rifles, etc, small.
// Durability is rated 1-10; 10 near perfect, 1 it breaks every few shots
// Burst is the # of rounds fired, 0 if no burst ability.
// clip is how many shots we get before reloading.

#define GUN(id,name,rarity,price,color,mat1,mat2,skill,ammo,volume,wgt,melee_dam,\
to_hit,dmg,accuracy,recoil,durability,burst,clip,reload_time,des,flags) \
itypes[id]=new it_gun(id,rarity,price,name,des,'(',\
color,mat1,mat2,volume,wgt,melee_dam,0,to_hit,flags,skill,ammo,dmg,accuracy,\
recoil,durability,burst,clip,reload_time)

// GUN MODS
// Accuracy is inverted from guns; high values are a bonus, low values a penalty
// The clip modification is a percentage of the original clip size.
// The final variable is a bitfield of acceptible ammo types.  Using 0 means
//   that any ammo type is acceptable (so long as the mod works on the class of
//   gun)
#define GUNMOD(id, name, rare, value, color, mat1, mat2, volume, weight, meleedam,\
               meleecut, meleehit, acc, damage, loudness, clip, recoil, burst,\
               newtype, pistol, shotgun, smg, rifle, a_a_t, des, flags)\
  itypes[id]=new it_gunmod(id, rare, value, name, des, ':',\
                            color, mat1, mat2, volume, weight, meleedam,\
                            meleecut, meleehit, flags, acc, damage, loudness,\
                            clip, recoil, burst, newtype, a_a_t, pistol,\
                            shotgun, smg, rifle)


// BOOKS
// Try to keep colors consistant among types of books.
// TYPE is the skill type required to read, or trained via reading; see skill.h
// LEV is the skill level you can be brought to by this book; if your skill is
//  already at LEV or higher, you may enjoy the book but won't learn anything.
// REQ is the skill level required to read this book, at all. If you lack the
//  required skill level, you'll waste 10 (?) turns then quit.
// FUN is the fun had by reading;
// INT is an intelligence requirement.
// TIME is the time, in minutes (10 turns), taken to gain the fun/skill bonus.
#define BOOK(id, name,rarity,price,color,mat1,mat2,volume,wgt,melee_dam,to_hit,\
type,level,req,fun,intel,time,des) \
itypes[id]=new it_book(id,rarity,price,name,des,'?',\
color,mat1,mat2,volume,wgt,melee_dam,0,to_hit,0,type,level,req,fun,intel,time)
//	NAME			RAR PRC	COLOR		MAT1	MAT2

// CONTAINERS
// These are containers you hold in your hand--ones you wear are _armor_!
// These only have two attributes; contains, which is the volume it holds, and
//  the flags. There are only three flags, con_rigid, con_wtight and con_seals.
// con_rigid is used if the item's total volume is constant.
//  Otherwise, its volume is calculated as VOL + volume of the contents
// con_wtight is used if you can store liquids in this container
// con_seals is used if it seals--this has many implications
//  * Won't spill
//  * Can be used as an icebox
//  * Others??
#define CONT(id, name,rarity,price,color,mat1,mat2,volume,wgt,melee_dam,to_hit,\
contains,flags,des) \
itypes[id]=new it_container(id,rarity,price,name,des,\
')',color,mat1,mat2,volume,wgt,melee_dam,0,to_hit,0,contains,flags)
// NAME		RAR PRC	COLOR		MAT1	MAT2

/* TOOLS
 * MAX is the maximum number of charges help.
 * DEF is the default number of charges--items will be generated with this
 *  many charges.
 * USE is how many charges are used up when 'a'pplying the object.
 * SEC is how many turns will pass before a charge is drained if the item is
 *  active; generally only used in the "<whatever> (on)" forms
 * FUEL is the type of charge the tool uses; set to AT_NULL if the item is
 *  unable to be recharged.
 * REVERT is the item type that the tool will revert to once its charges are
 *  drained
 * FUNCTION is a function called when the tool is 'a'pplied, or called once per
 *  turn if the tool is active.  The same function can be used for both.  See
 *  iuse.h and iuse.cpp for functions.
 */
#define TOOL(id, name,rarity,price,sym,color,mat1,mat2,volume,wgt,melee_dam,\
melee_cut,to_hit,max_charge,def_charge,charge_per_use,charge_per_sec,fuel,\
revert,func,flags,des) \
itypes[id]=new it_tool(id,rarity,price,name,des,sym,\
color,mat1,mat2,SOLID,volume,wgt,melee_dam,melee_cut,to_hit,flags,max_charge,\
def_charge,charge_per_use,charge_per_sec,fuel,revert,func)

//  NAME		RAR PRC COLOR		MAT1	MAT2
GUN("nailgun", "nail gun",		12, 100,c_ltblue,	IRON,	MNULL,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP RELOAD
    "pistol",	AT_NAIL, 4, 22, 12,  1,  0, 20,  0,  8,  5, 100, 450, "\
A tool used to drive nails into wood or other material. It could also be\n\
used as a ad-hoc weapon, or to practice your handgun skill up to level 1.",
mfb(IF_MODE_BURST));

GUN("bbgun", "BB gun",		10, 100,c_ltblue,	IRON,	WOOD,
	"rifle",	AT_BB,	 8, 16,  9,  2,  0,  6, -5,  7,  0, 20, 500, "\
Popular among children. It's fairly accurate, but BBs deal nearly no damage.\n\
It could be used to practice your rifle skill up to level 1.",
0);

GUN("crossbow", "crossbow",		 2,1000,c_green,	IRON,	WOOD,
	"archery",	AT_BOLT, 6,  9, 11,  1,  0, 18,  0,  6,  0,  1, 800, "\
A slow-loading hand weapon that launches bolts. Stronger people can reload\n\
it much faster. Bolts fired from this weapon have a good chance of remaining\n\
intact for re-use.",
mfb(IF_STR_RELOAD));

//  NAME		RAR PRC COLOR		MAT1	MAT2
GUN("compbow", "compound bow",      2,1400,c_yellow,       STEEL,  PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP RELOAD
        "archery",     AT_ARROW,12, 8,  8,  1,  0, 20,  0,  6,  0,  1, 100, "\
A bow with wheels that fires high velocity arrows. Weaker people can use\n\
compound bows more easily. Arrows fired from this weapon have a good chance\n\
of remaining intact for re-use. It requires 8 strength to fire",
mfb(IF_STR8_DRAW)|mfb(IF_RELOAD_AND_SHOOT));

GUN("longbow", "longbow",           5, 800,c_yellow,       WOOD,   MNULL,
        "archery",     AT_ARROW,8, 4, 10,  0,  0, 12,  0,  6,  0,  1,  80, "\
A six-foot wooden bow that fires feathered arrows. This takes a fair amount\n\
of strength to draw. Arrows fired from this weapon have a good chance of\n\
remaining intact for re-use. It requires 10 strength to fire",
mfb(IF_STR10_DRAW)|mfb(IF_RELOAD_AND_SHOOT));

GUN("rifle_22", "pipe rifle: .22",	0,  800,c_ltblue,	IRON,	WOOD,
"rifle",	AT_22,	 9, 13, 10,  2, -2, 15,  2,  6,  0,  1, 250, "\
A home-made rifle. It is simply a pipe attached to a stock, with a hammer to\n\
strike the single round it holds.",
0);

//  NAME		RAR PRC COLOR		MAT1	MAT2
GUN("rifle_9mm", "pipe rifle: 9mm",	0,  900,c_ltblue,	IRON,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP RELOAD
	"rifle",	AT_9MM,	10, 16, 10,  2, -2, 15,  2,  6,  0,  1, 250, "\
A home-made rifle. It is simply a pipe attached to a stock, with a hammer to\n\
strike the single round it holds.",
0);

GUN("smg_9mm", "pipe SMG: 9mm",	0, 1050,c_ltblue,	IRON,	WOOD,
	"smg",		AT_9MM,  5,  8,  6, -1,  0, 30,  6,  5,  4, 10, 400, "\
A home-made machine pistol. It features a rudimentary blowback system, which\n\
allows for small bursts.",
mfb(IF_MODE_BURST));

GUN("smg_45", "pipe SMG: .45",	0, 1150,c_ltblue,	IRON,	WOOD,
	"smg",		AT_45,	 6,  9,  7, -1,  0, 30,  6,  5,  3,  8, 400, "\
A home-made machine pistol. It features a rudimentary blowback system, which\n\
allows for small bursts.",
mfb(IF_MODE_BURST));

GUN("sig_mosquito", "SIG Mosquito",	 5,1200,c_dkgray,	STEEL,	PLASTIC,
	"pistol",	AT_22,	 1,  6,  9,  1,  1, 28,  4,  8,  0, 10, 350, "\
A popular, very small .22 pistol. \"Ergonomically designed to give the best\n\
shooting experience.\" --SIG Sauer official website",
0);

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("sw_22", "S&W 22A",		 5,1250,c_dkgray,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	"pistol",	AT_22,	 1, 10,  9,  1,  1, 25,  5,  7,  0, 10, 300, "\
A popular .22 pistol. \"Ideal for competitive target shooting or recreational\n\
shooting.\" --Smith & Wesson official website",
0);

GUN("glock_19", "Glock 19",		 7,1400,c_dkgray,	STEEL,	PLASTIC,
	"pistol",	AT_9MM,	 2,  5,  8,  1,  0, 24,  6,  6,  0, 15, 300, "\
Possibly the most popular pistol in existance. The Glock 19 is often derided\n\
for its plastic contruction, but it is easy to shoot.",
0);

GUN("usp_9mm", "USP 9mm",		 6,1450,c_dkgray,	STEEL,	PLASTIC,
	"pistol",	AT_9MM,	 2,  6,  8,  1, -1, 25,  5,  9,  0, 15, 350, "\
A popular 9mm pistol, widely used among law enforcement. Extensively tested\n\
for durability, it has been found to stay accurate even after subjected to\n\
extreme abuse.",
0);

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("sw_619", "S&W 619",		 4,1450,c_dkgray,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	"pistol",	AT_38,	 2,  9,  9,  1,  2, 23,  4,  8,  0,  7, 75, "\
A seven-round .38 revolver sold by Smith & Wesson. It features a fixed rear\n\
sight and a reinforced frame.",
mfb(IF_RELOAD_ONE));

GUN("taurus_38", "Taurus Pro .38",	 4,1500,c_dkgray,	STEEL,	PLASTIC,
	"pistol",	AT_38,	 2,  6,  8,  1,  1, 22,  6,  7,  0, 10, 350, "\
A popular .38 pistol. Designed with numerous safety features and built from\n\
high-quality, durable materials.",
0);

GUN("sig_40", "SIG Pro .40",	 4,1500,c_dkgray,	STEEL,	PLASTIC,
	"pistol",	AT_40,	 2,  6,  8,  1,  1, 22,  8,  7,  0, 12, 350, "\
Originally marketed as a lightweight and compact alternative to older SIG\n\
handguns, the Pro .40 is popular among European police forces.",
0);

GUN("sw_610", "S&W 610",		 2,1460,c_dkgray,	STEEL,	WOOD,
	"pistol",	AT_40,	 2, 10, 10,  1,  2, 23,  6,  8,  0,  6, 60, "\
The Smith and Wesson 610 is a classic six-shooter revolver chambered for 10mm\n\
rounds, or for S&W's own .40 round.",
mfb(IF_RELOAD_ONE));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("ruger_redhawk", "Ruger Redhawk",	 3,1560,c_dkgray,	STEEL,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	"pistol",	AT_44,	 2, 12, 10,  1,  2, 21,  6,  8,  0,  6, 80, "\
One of the most powerful handguns in the world when it was released in 1979,\n\
the Redhawk offers very sturdy contruction, with an appearance that is\n\
reminiscent of \"Wild West\" revolvers.",
mfb(IF_RELOAD_ONE));

GUN("deagle_44", "Desert Eagle .44",	 2,1750,c_dkgray,	STEEL,	PLASTIC,
	"pistol",	AT_44,	 4, 17, 14,  1,  4, 35,  3,  7,  0, 10, 400, "\
One of the most recognizable handguns due to its popularity in movies, the\n\
\"Deagle\" is better known for its menacing appearance than its performace.\n\
It's highly innaccurate, but its heavy weight reduces recoil.",
0);

GUN("usp_45", "USP .45",		 6,1600,c_dkgray,	STEEL,	PLASTIC,
	"pistol",	AT_45,	 2,  7,  9,  1,  1, 25,  8,  9,  0, 12, 350, "\
A popular .45 pistol, widely used among law enforcement. Extensively tested\n\
for durability, it has been found to stay accurate even after subjected to\n\
extreme abuse.",
0);

GUN("m1911", "M1911",		 5,1680,c_ltgray,	STEEL,	PLASTIC,
	"pistol",	AT_45,	 3, 10, 12,  1,  6, 25,  9,  7,  0,  7, 300, "\
The M1911 was the standard-issue sidearm from the US Military for most of the\n\
20th Century. It remains one of the most popular .45 pistols today.",
0);

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("fn57", "FN Five-Seven",	 2,1550,c_ltgray,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	"pistol",	AT_57,	 2,  5,  6,  0,  2, 13,  6,  8,  0, 20, 300, "\
Designed to work with FN's proprietary 5.7x28mm round, the Five-Seven is a\n\
lightweight pistol with a very high capacity, best used against armored\n\
opponents.",
0);

GUN("hk_ucp", "H&K UCP",		 2,1500,c_ltgray,	STEEL,	PLASTIC,
	"pistol",	AT_46,	 2,  5,  6,  0,  2, 12,  6,  8,  0, 20, 300, "\
Designed to work with H&K's proprietary 4.6x30mm round, the UCP is a small\n\
pistol with a very high capacity, best used against armored opponents.",
0);

GUN("tokarev", "Tokarev TT-30",		 7,1400,c_dkgray,	STEEL,	PLASTIC,
//	SKILL		AMMO	    VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	"pistol",	AT_762x25,	 2,  12,  10,1,  0, 23,  4,  6,  0, 8, 300, "\
The Norinco manufactured Tokarev TT-30 is the standard sidearm of the\n\
Chinese military, it does not see extensive use outside of China.",
0);

GUN("shotgun_sawn", "sawn-off shotgun",	 1, 700,c_red,	IRON,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	"shotgun",	AT_SHOT, 6, 10, 14, 2,  4,  40, 15, 4,   0,   2, 100, "\
The barrels of shotguns are often sawed in half to make it more maneuverable\n\
and concealable. This has the added effect of reducing accuracy greatly.",
mfb(IF_RELOAD_ONE));

GUN("saiga_sawn", "sawn-off Saiga 12",	 1, 700,c_red,	IRON,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	"shotgun",	AT_SHOT, 6, 10, 14,  2,  4, 40, 15,  4,  0,  10, 100, "\
The Saiga-12 shotgun is designed on the same Kalishnikov pattern as the AK47\n\
rifle. It reloads with a magazine, rather than one shell at a time like most\n\
shotguns. This one has had the barrel cut short, vastly reducing accuracy\n\
but making it more portable",
mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("shotgun_s", "single barrel shotgun",1,600,c_red,IRON,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	"shotgun",	AT_SHOT,12, 20, 14,  3,  0,  6,  5,  6,  0,  1, 100, "\
An old shotgun, possibly antique. It is little more than a barrel, a wood\n\
stock, and a hammer to strike the cartridge. Its simple design keeps it both\n\
light and accurate.",
0);

GUN("shotgun_d", "double barrel shotgun",2,1050,c_red,IRON,	WOOD,
	"shotgun",	AT_SHOT,12, 26, 15,  3,  0,  7,  4,  7,  2,  2, 100, "\
An old shotgun, possibly antique. It is little more than a pair of barrels,\n\
a wood stock, and a hammer to strike the cartridges.",
mfb(IF_RELOAD_ONE)|mfb(IF_MODE_BURST));

GUN("remington_870", "Remington 870",	 9,2200,c_red,	STEEL,	PLASTIC,
	"shotgun",	AT_SHOT,16, 30, 17,  3,  5, 10,  0,  8,  3,  6, 100, "\
One of the most popular shotguns on the market, the Remington 870 is used by\n\
hunters and law enforcement agencies alike thanks to its high accuracy and\n\
muzzle velocity.",
mfb(IF_RELOAD_ONE)|mfb(IF_MODE_BURST));

GUN("mossberg_500", "Mossberg 500",	 5,2250,c_red,	STEEL,	PLASTIC,
	"shotgun",	AT_SHOT,15, 30, 17,  3,  0, 13, -2,  9,  3,  8, 80, "\
The Mossberg 500 is a popular series of pump-action shotguns, often acquired\n\
for military use. It is noted for its high durability and low recoil.",
mfb(IF_RELOAD_ONE)|mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("saiga_12", "Saiga-12",		 3,2300,c_red,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	"shotgun",	AT_SHOT,15, 36, 17,  3,  0, 17,  2,  7,  4, 10, 500, "\
The Saiga-12 shotgun is designed on the same Kalishnikov pattern as the AK47\n\
rifle. It reloads with a magazine, rather than one shell at a time like most\n\
shotguns.",
mfb(IF_MODE_BURST));

GUN("american_180", "American-180",	 2,1600,c_cyan, STEEL,	MNULL,
	"smg",		AT_22,  12, 23, 11,  0,  2, 20,  0,  6, 30,165, 500, "\
The American-180 is a submachine gun developed in the 1960's that fires .22\n\
LR, unusual for an SMG. Though the round is low-powered, the high rate of\n\
fire and large magazine makes the 180 a formidable weapon.",
mfb(IF_MODE_BURST));

GUN("uzi", "Uzi 9mm",		 8,2080,c_cyan,	STEEL,	MNULL,
	"smg",		AT_9MM,	 6, 29, 10,  1,  0, 25, -2,  7, 12, 32, 450, "\
The Uzi 9mm has enjoyed immense popularity, selling more units than any other\n\
submachine gun. It is widely used as a personal defense weapon, or as a\n\
primary weapon by elite frontline forces.",
mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("tec9", "TEC-9",		10,1750,c_cyan,	STEEL,	MNULL,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	"smg",		AT_9MM,	 5, 12,  9,  1,  3, 24,  0,  6,  8, 32, 400, "\
The TEC-9 is a machine pistol made of cheap polymers and machine stamped\n\
parts. Its rise in popularity among criminals is largely due to its\n\
intimidating looks and low price.",
mfb(IF_MODE_BURST));

GUN("calico", "Calico M960",	 6,2400,c_cyan,	STEEL,	MNULL,
	"smg",		AT_9MM,	 7, 19,  9,  1, -3, 28, -4,  6, 20, 50, 500, "\
The Calico M960 is an automatic carbine with a unique circular magazine that\n\
allows for high capacities and reduced recoil.",
mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("hk_mp5", "H&K MP5",		12,2800,c_cyan,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	"smg",		AT_9MM,	12, 26, 10,  2,  1, 18, -3,  8,  8, 30, 400, "\
The Heckler & Koch MP5 is one of the most widely-used submachine guns in the\n\
world, and has been adopted by special police forces and militaries alike.\n\
Its high degree of accuracy and low recoil are universally praised.",
mfb(IF_MODE_BURST));

GUN("mac_10", "MAC-10",		14,1800,c_cyan,	STEEL,	MNULL,
	"smg",		AT_45,	 4, 25,  8,  1, -4, 28,  0,  7, 30, 30, 450, "\
The MAC-10 is a popular machine pistol originally designed for military use.\n\
For many years they were the most inexpensive automatic weapon in the US, and\n\
enjoyed great popularity among criminals less concerned with quality firearms."
,
mfb(IF_MODE_BURST));

GUN("hk_ump45", "H&K UMP45",	12,3000,c_cyan,	STEEL,	PLASTIC,
	"smg",		AT_45,	13, 20, 11,  1,  0, 13, -3,  8,  4, 25, 450, "\
Developed as a successor to the MP5 submachine gun, the UMP45 retains the\n\
earlier model's supreme accuracy and low recoil, but in the higher .45 caliber."
,
mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("TDI", "TDI Vector",	 4,4200,c_cyan,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	"smg",		AT_45,	13, 20,  9,  0, -2, 15,-14,  7,  8, 30, 450, "\
The TDI Vector is a submachine gun with a unique, in-line design that makes\n\
recoil very managable, even in the powerful .45 caliber.",
mfb(IF_MODE_BURST));

GUN("fn_p90", "FN P90",		 7,4000,c_cyan,	STEEL,	PLASTIC,
	"smg",		AT_57,	14, 22, 10,  1,  0, 22, -8,  8, 20, 50, 500, "\
The first in a new genre of guns, termed \"personal defense weapons.\"  FN\n\
designed the P90 to use their proprietary 5.7x28mm ammunition.  It is made\n\
for firing bursts manageably.",
mfb(IF_MODE_BURST));

GUN("hk_mp7", "H&K MP7",		 5,3400,c_cyan,	STEEL,	PLASTIC,
	"smg",		AT_46,	 7, 17,	 7,  1,  0, 21,-10,  8, 20, 20, 450, "\
Designed by Heckler & Koch as a competitor to the FN P90, as well as a\n\
successor to the extremely popular H&K MP5. Using H&K's proprietary 4.6x30mm\n\
ammunition, it is designed for burst fire.",
mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("ppsh", "PPSh-41",	12,2800,c_cyan,	STEEL,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	"smg",		AT_762x25,12, 26, 10,  2,  1, 16, -1,  8,  8, 35, 400, "\
The Soviet made PPSh-41, chambered in 7.62 Tokarev provides a relatively\n\
large ammunition capacity, coupled with low recoil and decent accuracy.",
mfb(IF_MODE_BURST));
//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("marlin_9a", "Marlin 39A",	14,1600,c_brown,IRON,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP RELOAD
	"rifle",	AT_22,	 11, 26, 12,  3,  3, 10, -5,  8,  0, 19,  90, "\
The oldest and longest-produced shoulder firearm in the world. Though it\n\
fires the weak .22 round, it is highly accurate and damaging, and has\n\
essentially no recoil.",
mfb(IF_RELOAD_ONE));

GUN("ruger_1022", "Ruger 10/22",	12,1650,c_brown,IRON,	WOOD,
	"rifle",	AT_22,	11, 23, 12,  3,  0,  8, -5,  8,  0, 10, 500, "\
A popular and highly accurate .22 rifle. At the time of its introduction in\n\
1964, it was one of the first modern .22 rifles designed for quality, and not\n\
as a gun for children.",
0);

GUN("browning_blr", "Browning BLR",	 8,3500,c_brown,IRON,	WOOD,
	"rifle",	AT_3006,12, 28, 12,  3, -3,  6, -4,  7,  0,  4, 100, "\
A very popular rifle for hunting and sniping. Its low ammo capacity is\n\
offset by the very powerful .30-06 round it fires.",
mfb(IF_RELOAD_ONE));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("remington_700", "Remington 700",	14,3200,c_brown,IRON,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	"rifle",	AT_3006,12, 34, 13,  3,  7,  9, -3,  8,  0,  4, 75, "\
A very popular and durable hunting or sniping rifle. Popular among SWAT\n\
and US Marine snipers. Highly damaging, but perhaps not as accurate as the\n\
competing Browning BLR.",
mfb(IF_RELOAD_ONE));

GUN("sks", "SKS",		12,3000,c_brown,IRON,	WOOD,
	"rifle",	AT_762,	12, 34, 13,  3,  0,  5, -4,  8,  0, 10, 450, "\
Developed by the Soviets in 1945, this rifle was quickly replaced by the\n\
full-auto AK47. However, due to its superb accuracy and low recoil, this gun\n\
maintains immense popularity.",
0);

GUN("ruger_mini", "Ruger Mini-14",	12,3200,c_brown,IRON,	WOOD,
	"rifle",	AT_223,	12, 26, 12,  3,  4,  5, -4,  8,  0, 10, 500, "\
A small, lightweight semi-auto carbine designed for military use. Its superb\n\
accuracy and low recoil makes it more suitable than full-auto rifles for some\n\
situations.",
0);

GUN("savage_111f", "Savage 111F",	10,3280,c_brown,STEEL,	PLASTIC,
	"rifle",	AT_308, 12, 26, 13,  3,  6,  4,-11,  9,  0,  3, 100, "\
A very accurate rifle chambered for the powerful .308 round. Its very low\n\
ammo capacity is offset by its accuracy and near-complete lack of recoil.",
mfb(IF_RELOAD_ONE));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("hk_g3", "H&K G3",		15,5050,c_blue,	IRON,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	"rifle",	AT_308,	16, 40, 13,  2,  8, 10,  4,  8, 10, 20, 550, "\
An early battle rifle developed after the end of WWII. The G3 is designed to\n\
unload large amounts of deadly ammunition, but it is less suitable over long\n\
ranges.",
mfb(IF_MODE_BURST));

GUN("hk_g36", "H&K G36",		17,5100,c_blue,	IRON,	PLASTIC,
	"rifle",	AT_223, 15, 32, 13,  2,  6,  8,  5,  8, 15, 30, 500, "\
Designed as a replacement for the early H&K G3 battle rifle, the G36 is more\n\
accurate, and uses the much-lighter .223 round, allowing for a higher ammo\n\
capacity.",
mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("ak47", "AK-47",		16,4000,c_blue,	IRON,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	"rifle",	AT_762,	16, 38, 14,  2,  0, 11,  4,  9,  8, 30, 475, "\
One of the most recognizable assault rifles ever made, the AK-47 is renowned\n\
for its durability even under the worst conditions.",
mfb(IF_MODE_BURST));

GUN("fn_fal", "FN FAL",		16,4500,c_blue,	IRON,	WOOD,
	"rifle",	AT_308,	19, 36, 14,  2,  7, 13, -2,  8, 10, 20, 550, "\
A Belgian-designed battle rifle, the FN FAL is not very accurate for a rifle,\n\
but its high fire rate and powerful .308 ammunition have made it one of the\n\
most widely-used battle rifles in the world.",
mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("acr", "Bushmaster ACR",	 4,4200,c_blue,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	"rifle",	AT_223,	15, 27,	18,  2,  2, 10, -2,  8,  3, 30, 475, "\
This carbine was developed for military use in the early 21st century. It is\n\
damaging and accurate, though its rate of fire is a bit slower than competing\n\
.223 carbines.",
mfb(IF_MODE_BURST));

GUN("ar15", "AR-15",		 9,4000,c_blue,	STEEL,	PLASTIC,
	"rifle",	AT_223,	19, 28, 12,  2,  0,  6,  0,  7, 10, 30, 500, "\
A widely used assault rifle and the father of popular rifles such as the M16.\n\
It is light and accurate, but not very durable.",
mfb(IF_MODE_BURST));

GUN("m4a1", "M4A1",		 7,4400,c_blue,	STEEL,	PLASTIC,
	"rifle",	AT_223, 14, 24, 13,  2,  4,  7,  2,  6, 10, 30, 475, "\
A popular carbine, long used by the US military. Though accurate, small, and\n\
lightweight, it is infamous for its fragility, particularly in less-than-\n\
ideal terrain.",
mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("scar_l", "FN SCAR-L",	 6,4800,c_blue,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	"rifle",	AT_223,	15, 29, 18,  2,  1,  6, -4,  8, 10, 30, 500, "\
A modular assault rifle designed for use by US Special Ops units. The 'L' in\n\
its name stands for light, as it uses the lightweight .223 round. It is very\n\
accurate and low on recoil.",
mfb(IF_MODE_BURST));

GUN("scar_h", "FN SCAR-H",	 5,4950,c_blue,	STEEL,	PLASTIC,
	"rifle",	AT_308,	16, 32, 20,  2,  1,  8, -4,  8,  8, 20, 550, "\
A modular assault rifle designed for use by US Special Ops units. The 'H' in\n\
its name stands for heavy, as it uses the powerful .308 round. It is fairly\n\
accurate and low on recoil.",
mfb(IF_MODE_BURST));

GUN("steyr_aug", "Steyr AUG",	 6,4900,c_blue, STEEL,	PLASTIC,
	"rifle",	AT_223, 14, 32, 17,  1, -3,  7, -8,  8,  6, 30, 550, "\
The Steyr AUG is an Austrian assault rifle that uses a bullpup design. It is\n\
used in the armed forces and police forces of many nations, and enjoys\n\
low recoil and high accuracy.",
mfb(IF_MODE_BURST));

GUN("m249", "M249",		 1,7500,c_ltred,STEEL,	PLASTIC,
//  SKILL       AMMO    VOL WGT MDG HIT DMG ACC REC DUR BST CLIP RELOAD
	"rifle",	AT_223,	32, 68, 27, -4, -6, 20,  6,  7, 30,200, 750, "\
The M249 is a mountable machine gun used by the US military and SWAT teams.\n\
Quite innaccurate and difficult to control, the M249 is designed to fire many\n\
rounds very quickly.",
mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	 MAT1	MAT2
GUN("v29", "V29 laser pistol",	 1,7200,c_magenta,STEEL,PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	"pistol",	AT_FUSION,4, 6,  5,  1, -2, 20,  0,  8,  0, 20, 200, "\
The V29 laser pistol was designed in the mid-21st century, and was one of the\n\
first firearms to use fusion as its ammunition. It is larger than most\n\
traditional handguns, but displays no recoil whatsoever.",
0);

GUN("ftk93", "FTK-93 fusion gun", 1,9800,c_magenta,STEEL, PLASTIC,
	"rifle",	AT_FUSION,18,20, 10, 1, 40, 10,  0,  9,  0,  2, 600, "\
A very powerful fusion rifle developed shortly before the influx of monsters.\n\
It can only hold two rounds at a time, but a special superheating unit causes\n\
its bolts to be extremely deadly.",
0);

GUN("nx17", "NX-17 charge rifle",1,12000,c_magenta,STEEL, PLASTIC,
	"rifle",	AT_NULL, 13,16,  8, -1,  0,   6,  0,  8,  0, 10,   0, "\
A multi-purpose rifle, designed for use in conjunction with a unified power\n\
supply, or UPS. It does not reload normally; instead, press fire once to\n\
start charging it from your UPS, then again to unload the charge.",
mfb(IF_CHARGE)|mfb(IF_NO_UNLOAD));

//  NAME		RAR PRC COLOR	 MAT1	MAT2
GUN("flamethrower_simple", "simple flamethr.",1,1600,c_pink,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP RELOAD
	"shotgun",	AT_GAS,  16,  8,  8, -1, -5,  6,  0,  6,  0,800, 800, "\
A simple, home-made flamethrower. While its capacity is not superb, it is\n\
more than capable of igniting terrain and monsters alike.",
mfb(IF_FIRE_100));

GUN("flamethrower", "flamethrower",	 1,3800,c_pink,	STEEL,	MNULL,
	"shotgun",	AT_GAS,  20, 14, 10, -2,  0,  4,  0,  8,  4,1600, 900, "\
A large flamethrower with substantial gas reserves. Very menacing and\n\
deadly.",
mfb(IF_FIRE_100));

GUN("launcher_simple", "tube 40mm launcher",0, 400,c_ltred,STEEL,	WOOD,
	"launcher",	AT_40MM,12, 20, 13, -1,  0, 16,  0,  6, 0,  1, 350, "\
A simple, home-made grenade launcher. Basically a tube with a pin firing\n\
mechanism to activate the grenade.",
0);

//  NAME		RAR PRC COLOR	 MAT1	MAT2
GUN("m79", "M79 launcher",	 5,4000,c_ltred,STEEL,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP RELOAD
	"launcher",	AT_40MM,14, 24, 16, -1,  3,  4, -5,  8, 0,  1, 250, "\
A widely-used grenade launcher that first saw use by American forces in the\n\
Vietnam war. Though mostly replaced by more modern launchers, the M79 still\n\
sees use with many units worldwide.",
0);

GUN("m320", "M320 launcher",	10,8500,c_ltred,STEEL,	MNULL,
	"launcher",	AT_40MM,  5, 13,  6,  0,  0, 12,  5,  9,  0,  1, 150, "\
Developed by Heckler & Koch, the M320 grenade launcher has the functionality\n\
of larger launchers in a very small package. However, its smaller size\n\
contributes to a lack of accuracy.",
0);

GUN("mgl", "Milkor MGL",	 6,10400,c_ltred,STEEL,	MNULL,
	"launcher",	AT_40MM, 24, 45, 13, -1,  0,  5, -2,  8,  2,  6, 300, "\
The Milkor Multi-Grenade Launcher is designed to compensate for the drawback\n\
of single-shot grenade launchers by allowing sustained heavy firepower.\n\
However, it is still slow to reload and must be used with careful planning.",
mfb(IF_RELOAD_ONE)|mfb(IF_MODE_BURST));

GUN("LAW", "M72 LAW",	200,8500,c_ltred,STEEL,	MNULL,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP RELOAD
	"launcher",	AT_66MM, 12, 13,  6,  0,  0, 12,  5,  9,  0,  1, 150, "\
A single use rocket launcher, developed during WW2 as a countermeasure\n\
to the increasing prevalance of tanks. Once fired, it cannot be reloaded\n\
and must be disposed of.",
mfb(IF_NO_UNLOAD)|mfb(IF_BACKBLAST));

//  NAME		    RAR PRC COLOR		MAT1	MAT2
GUN("coilgun", "coilgun",		1, 200,c_ltblue,	IRON,	MNULL,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP RELOAD
	"pistol",	AT_NAIL, 6, 30, 10, -1,  8, 10,  0,  5,  0, 100, 600, "\
A homemade gun, using electromagnets to accelerate a ferromagnetic\n\
projectile to high velocity. Powered by UPS.",
mfb(IF_USE_UPS));

GUN("hk_g80", "H&K G80 Railgun",		2,9200,c_ltblue,STEEL,	MNULL,
	"rifle",	AT_12MM,12, 36, 12,  1,  5,  15, 0,  8,  0, 20, 550, "\
Developed by Heckler & Koch in 2033, the railgun magnetically propels\n\
a ferromagnetic projectile using an alternating current. This makes it\n\
silent while still deadly. Powered by UPS.",
mfb(IF_USE_UPS));

GUN("plasma_rifle", "Boeing XM-P Plasma Rifle",		1,13000,c_ltblue,STEEL,	MNULL,
	"rifle",	AT_PLASMA,15, 40, 12, 1,  5,  5, 0,  8,  5, 25, 700, "\
Boeing developed the focused plasma weaponry together with DARPA. It heats\n\
hydrogen to create plasma and envelops it with polymers to reduce blooming.\n\
While powerful, it suffers from short range. Powered by UPS.",
mfb(IF_USE_UPS)|mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("revolver_shotgun", "Shotgun Revolver",1,600,c_red,IRON,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	"shotgun",	AT_SHOT,12, 24, 14,  3,  0,  6,  5,  6,  0,  6, 100, "\
A shotgun modified to use a revolver cylinder mechanism, it can hold\n\
6 cartridges.",
mfb(IF_RELOAD_ONE));

//	NAME      	 RAR  PRC  COLOR     MAT1   MAT2     VOL WGT DAM CUT HIT
GUNMOD("suppressor", "suppressor",	 15,  480, c_dkgray, STEEL, PLASTIC,  2,  1,  3,  0,  2,
//	ACC DAM NOI CLP REC BST NEWTYPE,	PISTOL	SHOT	SMG	RIFLE
	-1, -4,-25,  0,  0,  0,	AT_NULL,	true,	false,	true,	true,
	0, "\
Using a suppressor is almost an imperative in zombie-infested regions. Gunfire\n\
is very noisy, and will attract predators. Its only drawback is a reduced\n\
muzzle velocity, resulting in less accuracy and damage.",
0);

GUNMOD("grip", "enhanced grip",  12, 280, c_brown,  STEEL, PLASTIC,   1,  1,  0,  0, -1,
	 2,  0,  0,  0, -2,  0, AT_NULL,	false,	true,	true,	true,
	0, "\
A grip placed forward on the barrel allows for greater control and accuracy.\n\
Aside from increased weight, there are no drawbacks.",
0);

GUNMOD("barrel_big", "barrel extension",10,400,  c_ltgray, STEEL, MNULL,    4,  1,  5,  0,  2,
	 6,  1,  0,  0,  5,  0,	AT_NULL,	false,	false,	true,	true,
	0, "\
A longer barrel increases the muzzle velocity of a firearm, contributing to\n\
both accuracy and damage.  However, the longer barrel tends to vibrate after\n\
firing, greatly increasing recoil.",
0);

//	NAME      	 RAR  PRC  COLOR     MAT1   MAT2     VOL WGT DAM CUT HIT
GUNMOD("barrel_small", "shortened barrel", 6, 320, c_ltgray, STEEL, MNULL,    1,  1, -2,  0, -1,
//	ACC DAM NOI CLP REC BST NEWTYPE,	PISTOL	SHOT	SMG	RIFLE
	-5,  0,  2,  0, -6,  0, AT_NULL,	false,	false,	true,	true,
	0, "\
A shortened barrel results in markedly reduced accuracy, and a minor increase\n\
in noise, but also reduces recoil greatly as a result of the improved\n\
managability of the firearm.",
0);

GUNMOD("barrel_rifled", "rifled barrel",    5, 220, c_ltgray, STEEL, MNULL,    2,  1,  3,  0,  1,
	10,-20,  0,  0,  0, -1, AT_NULL,	false,	true,	false,	false,
	0, "\
Rifling a shotgun barrel is mainly done in order to improve its accuracy when\n\
firing slugs. The rifling makes the gun less suitable for shot, however.",
0);

GUNMOD("clip", "extended magazine",	  8,  560, c_ltgray, STEEL, PLASTIC,  1,  1, -2,  0, -1,
	-1,  0,  0, 50,  0,  0, AT_NULL,	true,	true,	true,	true,
	0, "\
Increases the ammunition capacity of your firearm by 50%, but the added bulk\n\
reduces accuracy slightly.",
0);

//	NAME      	 RAR  PRC  COLOR     MAT1   MAT2     VOL WGT DAM CUT HIT
GUNMOD("clip2", "double magazine",	   4, 720, c_ltgray, STEEL, PLASTIC,  2,  2,  0,  0,  0,
//	ACC DAM NOI CLP REC BST NEWTYPE,	PISTOL	SHOT	SMG	RIFLE
	-2,  0,  0,100,  2,  0, AT_NULL,	false,	true,	true,	true,
	0, "\
Completely doubles the ammunition capacity of your firearm, but the added\n\
bulk reduces accuracy and increases recoil.",
0);

GUNMOD("spare_mag", "spare magazine",	   15, 200, c_ltgray, STEEL, PLASTIC,  1,  1,  0,  0,  0,
	0,  0,  0,  0,  0,  0, AT_NULL,	true,	true,	true,	true,
	0, "\
A spare magazine you can keep on hand to make reloads faster, but must itself\n\
 be reloaded before it can be used again.",
0);

GUNMOD("stablizer", "gyroscopic stablizer",4,680,c_blue,  STEEL, PLASTIC,  3,  2,  0,  0, -3,
	 2, -2,  0,-10, -8,  0, AT_NULL,	false,	false,	true,	true,
	0, "\
An advanced unit that straps onto the side of your firearm and reduces\n\
vibration, greatly reducing recoil and increasing accuracy.  However, it also\n\
takes up space in the magazine slot, reducing ammo capacity.",
0);

GUNMOD("blowback", "rapid blowback",   3, 700, c_red,    STEEL, PLASTIC,  0,  1,  0,  0,  0,
	-3,  0,  4,  0,  0,  6, AT_NULL,	false,	false,	true,	true,
	0, "\
An improved blowback mechanism makes your firearm's automatic fire faster, at\n\
the cost of reduced accuracy and increased noise.",
0);

//	NAME      	 RAR  PRC  COLOR     MAT1   MAT2     VOL WGT DAM CUT HIT
GUNMOD("autofire", "auto-fire mechanism",2,650,c_red,    STEEL, PLASTIC,  1,  2,  2,  0, -1,
//	ACC DAM NOI CLP REC BST NEWTYPE,	PISTOL	SHOT	SMG	RIFLE
	-2,  0,  2,  0,  2,  3, AT_NULL,	true,	false,	false,	false,
	(mfb(AT_22)|mfb(AT_9MM)|mfb(AT_38)|mfb(AT_40)|mfb(AT_57)|mfb(AT_46)), "\
A simple mechanism that converts a pistol to a fully-automatic weapon, with\n\
a burst size of three rounds. However, it reduces accuracy, and increases\n\
noise and recoil.",
0);

//	NAME      	 RAR  PRC  COLOR     MAT1   MAT2     VOL WGT DAM CUT HIT
GUNMOD("retool_45", ".45 caliber retool",3,480, c_green,  STEEL, MNULL,    2,  2,  3,  0, -1,
//	ACC DAM NOI CLP REC BST NEWTYPE,	PISTOL	SHOT	SMG	RIFLE
	-2,  0,  0,  0,  2,  0, AT_45,		true,	false,	true,	false,
	(mfb(AT_9MM)|mfb(AT_38)|mfb(AT_40)|mfb(AT_44)), "\
Replacing several key parts of a 9mm, .38, .40 or .44 firearm converts it to\n\
a .45 firearm.  The conversion results in reduced accuracy and increased\n\
recoil.",
0);

GUNMOD("retool_9mm", "9mm caliber retool",3,420, c_green,  STEEL, MNULL,    1,  1,  0,  0, -1,
	-1,  0,  0,  0,  0,  0, AT_9MM,		true,	false,	true,	false,
	(mfb(AT_38)|mfb(AT_40)|mfb(AT_44)|mfb(AT_45)), "\
Replacing several key parts of a .38, .40, .44 or .45 firearm converts it to\n\
a 9mm firearm.  The conversion results in a slight reduction in accuracy.",
0);

GUNMOD("retool_22", ".22 caliber retool",2,320, c_green,  STEEL, MNULL,    1,  1, -2,  0, -1,
	-1,  0,  0,  0,  0,  0, AT_22,		true,	false,	true,	true,
	(mfb(AT_9MM)|mfb(AT_38)|mfb(AT_40)|mfb(AT_57)|mfb(AT_46)|mfb(AT_762)|
	 mfb(AT_223)), "\
Replacing several key parts of a 9mm, .38, .40, 5.7mm, 4.6mm, 7.62mm or .223\n\
firearm converts it to a .22 firearm. The conversion results in a slight\n\
reduction in accuracy.",
0);

//	NAME      	 RAR  PRC  COLOR     MAT1   MAT2     VOL WGT DAM CUT HIT
GUNMOD("retool_57", "5.7mm caliber retool",1,460,c_green, STEEL, MNULL,    1,  1, -3,  0, -1,
//	ACC DAM NOI CLP REC BST NEWTYPE,	PISTOL	SHOT	SMG	RIFLE
	-1,  0,  0,  0,  0,  0, AT_57,		true,	false,	true,	false,
	(mfb(AT_22)|mfb(AT_9MM)|mfb(AT_38)), "\
FN Hestal sells a conversion kit, used to convert .22, 9mm, or .38 firearms\n\
to their proprietary 5.7x28mm, a round designed for accuracy and armor\n\
penetration.",
0);

GUNMOD("retool_46", "4.6mm caliber retool",1,460,c_green, STEEL, MNULL,    1,  1, -3,  0, -1,
	-1,  0,  0,  0,  0,  0, AT_46,		true,	false,	true,	false,
	(mfb(AT_22)|mfb(AT_9MM)|mfb(AT_38)), "\
Heckler and Koch sells a conversion kit, used to convert .22, 9mm, or .38\n\
firearms to their proprietary 4.6x30mm, a round designed for accuracy and\n\
armor penetration.",
0);

//	NAME      	RAR  PRC  COLOR     MAT1   MAT2      VOL WGT DAM CUT HIT
GUNMOD("retool_308", ".308 caliber retool",2,520,c_green, STEEL, MNULL,     2,  1,  4,  0,  1,
//	ACC DAM NOI CLP REC BST NEWTYPE		PISTOL	SHOT	SMG	RIFLE
	-2,  0,  0,-20,  0,  0, AT_308,		false,	true,	false,	true,
	(mfb(AT_SHOT)|mfb(AT_762)|mfb(AT_223)|mfb(AT_3006)), "\
This kit is used to convert a shotgun or 7.62mm, .223 or .30-06 rifle to the\n\
popular and powerful .308 caliber. The conversion results in reduced ammo\n\
capacity and a slight reduction in accuracy.",
0);

GUNMOD("retool_223", ".223 caliber retool",2,500,c_green, STEEL, MNULL,     2,  1,  4,  0,  1,
	-2,  0,  0,-10,  0,  0, AT_223,		false,	true,	false,	true,
	(mfb(AT_SHOT)|mfb(AT_762)|mfb(AT_3006)|mfb(AT_308)), "\
This kit is used to convert a shotgun or 7.62mm, .30-06, or .308 rifle to the\n\
popular, accurate, and damaging .223 caliber. The conversion results in\n\
slight reductions in both accuracy and ammo capacity.",
0);

//	NAME      	RAR  PRC  COLOR     MAT1   MAT2      VOL WGT DAM CUT HIT
GUNMOD("conversion_battle", "battle rifle conversion",1,680,c_magenta,STEEL,MNULL, 4,  3,  6,  0, -1,
//	ACC DAM NOI CLP REC BST NEWTYPE		PISTOL	SHOT	SMG	RIFLE
	-6,  6,  4, 20,  4,  4, AT_NULL,	false,	false,	false,	true,
	0, "\
This is a complete conversion kit, designed to turn a rifle into a powerful\n\
battle rifle. It reduces accuracy, and increases noise and recoil, but also\n\
increases damage, ammo capacity, and fire rate.",
0);

GUNMOD("conversion_sniper", "sniper conversion",1,660, c_green,  STEEL, MNULL,     1,  2,  0,  0, -1,
	10,  8,  3,-15,  0,-99, AT_NULL,	false,	false,	false,	true,
	0, "\
This is a complete conversion kit, designed to turn a rifle into a deadly\n\
sniper rifle. It decreases ammo capacity, and removes any automatic fire\n\
capabilities, but also increases accuracy and damage.",
0);

GUNMOD("m203", "M203",		2,650,	c_ltred, STEEL,	MNULL,        2,  1,  2,  0, -1,
	-2,  0,  0,  1,  0, 0, AT_40MM,		false,	false,	false,	true,
	0, "\
The M203 was originally designed for use with M16 variants but today can be\n\
attached to almost any rifle. It allows a single 40mm grenade to be loaded\n\
and fired.",
mfb(IF_MODE_AUX));

//	NAME      	RAR  PRC  COLOR     MAT1   MAT2      VOL WGT DAM CUT HIT
GUNMOD("bayonet", "bayonet",	 6, 400, c_ltcyan, STEEL, MNULL,       2,  2,  0, 16, -3,
//	ACC DAM NOI CLP REC BST NEWTYPE		PISTOL	SHOT	SMG	RIFLE
	  0,  0,  0,  0,  3,  0, AT_NULL,	false,	true,	true,	true,
	0, "\
A bayonet is a stabbing weapon that can be attached to the front of a\n\
shotgun, sub-machinegun or rifle, allowing a melee attack to deal\n\
piercing damage. The added length increases recoil slightly.",
mfb(IF_STAB));

GUNMOD("u_shotgun", "underslung shotgun", 2,650,  c_ltred, STEEL, MNULL,        2,  1,  2,  0, -1,
        -2,  0,  0,  2,  0, 0, AT_SHOT,         false,  false,  false,  true,
        0, "\
A miniaturized shotgun with 2 barrels, which can be mounted under the\n\
barrel of many rifles. It allows two shotgun shells to be loaded and fired.",
mfb(IF_MODE_AUX));

GUNMOD("gun_crossbow", "rail-mounted crossbow", 2, 500,  c_ltred, STEEL, WOOD,      2,  1,  2,  0, -1,
        0,  0,  0,  1,  0, 0, AT_BOLT,         false,  true,  false,  true,
        0, "\
A kit to attach a pair of crossbow arms and a firing rail to\n\
the barrel of a long firearm. It allows crossbow bolts to be fired.",
mfb(IF_MODE_AUX)|mfb(IF_STR_RELOAD));

BOOK("mag_porn", "Playboy",			20,  30,c_pink,		PAPER,	MNULL,
// VOL WGT DAM HIT	TYPE		LEV REQ FUN INT TIME
    1,  1, -3,  1,	NULL,	 0,  0,  1,  0,  10, "\
You can read it for the articles. Or not.");

BOOK("mag_tv", "US Weekly",		40,  40,c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	"speech",	 1,  0,  1,  3,  8, "\
Weekly news about a bunch of famous people who're all (un)dead now.");

BOOK("mag_news", "TIME magazine",		35,  40,c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	NULL,	 0,  0,  2,  7,  10, "\
Current events concerning a bunch of people who're all (un)dead now.");

BOOK("mag_cars", "Top Gear magazine",	40,  45,c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	"driving",	 1,  0,  1,  2,  8, "\
Lots of articles about cars and driving techniques.");

BOOK("mag_cooking", "Bon Appetit",		30,  45,c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	"cooking",	 1,  0,  1,  5,  8, "\
Exciting recipes and restaurant reviews. Full of handy tips about cooking.");

BOOK("mag_carpentry", "Birdhouse Monthly",       30,  45,c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	"carpentry",	 1,  0,  1,  5,  8, "\
A riveting periodical all about birdhouses and their construction.");

BOOK("mag_guns", "Guns n Ammo",		20,  48,c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	"gun",		 1,  0,  1,  2,  7, "\
Reviews of firearms, and various useful tips about their use.");

BOOK("mag_archery", "Archery for Kids",		20,  48, c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	"archery",		 1,  0,  1,  2,  7, "\
Will you be able to place the arrow right into bull's eye?\n\
It is not that easy, but once you know how it's done,\n\
you will have a lot of fun with archery.");

BOOK("mag_gaming", "Computer Gaming",			20,  30,c_pink,		PAPER,	MNULL,
// VOL WGT DAM HIT	TYPE		LEV REQ FUN INT TIME
    1,  1, -3,  1,	NULL,	 0,  0,  2,  7,  8, "\
Reviews of recently released computer games and previews\n\
of upcoming titles.");

BOOK("mag_comic", "comic book",			20,  30,c_pink,		PAPER,	MNULL,
// VOL WGT DAM HIT	TYPE		LEV REQ FUN INT TIME
    1,  1, -3,  1,	NULL,	 0,  0,  2,  0,  7, "\
A super-hero comic.");

BOOK("mag_firstaid", "Paramedics",		15,  48, c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	"firstaid",		 1,  0,  1,  1,  8, "\
An educational magazine for EMTs.");

BOOK("mag_dodge", "Dance Dance Dance!",		20,  48,c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	"dodge",		 1,  0,  1,  2,  8, "\
Learn the moves of the trendiest dances right now.");

BOOK("mag_throwing", "Diskobolus",		20,  48,c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	"throw",		 1,  0,  1,  1,  8, "\
A biannual magazine devoted to discus throw.");

BOOK("mag_swimming", "Swim Planet",		20,  48,c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	"swimming",		 1,  0,  1,  1,  8, "\
The world's leading resource about aquatic sports.");

BOOK("novel_romance", "romance novel",		30,  55,c_ltblue,	PAPER,	MNULL,
    4,  1, -2,  0,	NULL,	 0,  0,  2,  4, 15, "\
Drama and mild smut.");

BOOK("novel_spy", "spy novel",		28,  55,c_ltblue,	PAPER,	MNULL,
    4,  1, -2,  0,	NULL,	 0,  0,  3,  5, 18, "\
A tale of intrigue and espionage amongst Nazis, no, Commies, no, Iraqis!");

//	NAME			RAR PRC	COLOR		MAT1	MAT2
BOOK("novel_scifi", "scifi novel",		20,  55,c_ltblue,	PAPER,	MNULL,
// VOL WGT DAM HIT	TYPE		LEV REQ FUN INT TIME
    3,  1, -3,  0,	NULL,	 0,  0,  3,  6, 20, "\
Aliens, ray guns, and space ships.");

BOOK("novel_drama", "drama novel",		40,  55,c_ltblue,	PAPER,	MNULL,
    4,  1, -2,  0,	NULL,	 0,  0,  4,  7, 25, "\
A real book for real adults.");

BOOK("novel_fantasy", "fantasy novel",		20,  55,c_ltblue,	PAPER,	MNULL,
    4,  1, -2,  0,	NULL,	 0,  0,  4,  7, 20, "\
Basic Sword & Sorcery.");

BOOK("novel_mystery", "mystery novel",		25,  55,c_ltblue,	PAPER,	MNULL,
    4,  1, -2,  0,	NULL,	 0,  0,  4,  7, 18, "\
A detective investigates an unusual murder in a secluded location.");

BOOK("novel_horror", "horror novel",		18,  55,c_ltblue,	PAPER,	MNULL,
    4,  1, -2,  0,	NULL,	 0,  0,  1,  7, 18, "\
Maybe not the best reading material considering the situation.");

BOOK("manual_brawl", "101 Wrestling Moves",	30, 180,c_green,	PAPER,	MNULL,
    2,  1, -4,  0, 	"unarmed",	 3,  0,  0,  3, 15, "\
It seems to be a wrestling manual, poorly photocopied and released on spiral-\n\
bound paper. Still, there are lots of useful tips for unarmed combat.");

BOOK("manual_knives", "Spetsnaz Knife Techniques",12,200,c_green,	PAPER,	MNULL,
    1,  1, -5,  0,	"cutting",	 4,  1,  0,  4, 18, "\
A classic Soviet text on the art of attacking with a blade.");

//	NAME			RAR PRC	COLOR		MAT1	MAT2
BOOK("manual_mechanics", "Under the Hood",		35, 190,c_green,	PAPER,	MNULL,
// VOL WGT DAM HIT	TYPE		LEV REQ FUN INT TIME
    3,  1, -3,  0,	"mechanics",	 3,  0,  0,  5, 18, "\
An advanced mechanics manual, covering all sorts of topics.");

BOOK("manual_survival", "Pitching a Tent",20,200,c_green,  PAPER,  MNULL,
// VOL WGT DAM HIT      TYPE            LEV REQ FUN INT TIME
    3,  1,  -3, 0,      "survival",    3,   0,  0,  4,  18,"\
A guide detailing the basics of woodsmanship and outdoor survival.");

BOOK("manual_speech", "Self-Esteem for Dummies",	50, 160,c_green,	PAPER,	MNULL,
    3,  1, -3,  0,	"speech",	 3,  0,  0,  5, 20, "\
Full of useful tips for showing confidence in your speech.");

BOOK("manual_business", "How to Succeed in Business",40,180,c_green,	PAPER,	MNULL,
    3,  1, -3,  0,	"barter",	 3,  0, -1,  6, 25, "\
Useful if you want to get a good deal when purchasing goods.");

BOOK("manual_first_aid", "The Big Book of First Aid",40,200,c_green,	PAPER,	MNULL,
    5,  2, -2,  0,	"firstaid",	 3,  0,  0,  7, 20, "\
It's big and heavy, but full of great information about first aid.");

BOOK("manual_computers", "How to Browse the Web",	20, 170,c_green,	PAPER,	MNULL,
    3,  1, -3,  0,	"computer",	 2,  0,  0,  5, 15, "\
Very beginner-level information about computers.");

//	NAME			RAR PRC	COLOR		MAT1	MAT2
BOOK("cookbook", "Cooking on a Budget",	35, 160,c_green,	PAPER,	MNULL,
// VOL WGT DAM HIT	TYPE		LEV REQ FUN INT TIME
    4,  1, -2,  0,	"cooking",	 3,  0,  0,  4, 10, "\
A nice cook book that goes beyond recipes and into the chemistry of food.");

BOOK("cookbook_human", "To Serve Man", 1, 400, c_green, PAPER, MNULL,
    4, 1, -2, 0, "cooking", 4, 2, -5, 4, 10, "\
It's... it's a cookbook!");

BOOK("manual_electronics", "What's a Transistor?",	20, 200,c_green,	PAPER,	MNULL,
    3,  1, -3,  0,	"electronics",	 3,  0,  0,  7, 20, "\
A basic manual of electronics and circuit design.");

BOOK("manual_tailor", "Sew What?  Clothing!",	15, 190,c_green,	PAPER,	MNULL,
    3,  1, -3,  0,	"tailor",	 3,  0,  0,  4, 18, "\
A colorful book about tailoring.");

BOOK("manual_traps", "How to Trap Anything",	12, 240,c_green,	PAPER,	MNULL,
    2,  1, -3,  0,	"traps",	 4,  0,  0,  4, 20, "\
A worn manual that describes how to set and disarm a wide variety of traps.");

//	NAME			RAR PRC	COLOR		MAT1	MAT2
BOOK("manual_carpentry", "Building for Beginners",  10, 220,c_green,	PAPER,	MNULL,
// VOL WGT DAM HIT	TYPE		LEV REQ FUN INT TIME
    2,  1, -3,  0,	"carpentry",	 3,  0,  0,  5, 16, "\
A large, paperback book detailing several beginner's projects in\n\
construction.");

BOOK("textbook_computers", "Computer Science 301",	 8, 500,c_blue,		PAPER,	MNULL,
    7,  4,  5,  1,	"computer",	 5,  2, -2, 11, 35, "\
A college textbook on computer science.");

BOOK("textbook_electronics", "Advanced Electronics",	 6, 520,c_blue,		PAPER,	MNULL,
    7,  5,  5,  1,	"electronics",	 5,  2, -1, 11, 35, "\
A college textbook on circuit design.");

BOOK("textbook_business", "Advanced Economics",	12, 480,c_blue,		PAPER,	MNULL,
    7,  4,  5,  1,	"barter",	 5,  3, -1,  9, 30, "\
A college textbook on economics.");

BOOK("textbook_mechanics", "Mechanical Mastery",12,495,c_blue,PAPER,MNULL,
    6,  3,  4,  1,      "mechanics",   6,   3, -1,  6,  30,"\
An advanced guide on mechanics and welding, covering topics like\n\
\"Grinding off rust\" and \"Making cursive E\'s\".");

//	NAME			RAR PRC	COLOR		MAT1	MAT2
BOOK("textbook_chemistry", "Chemistry Textbook",	11, 495,c_blue,		PAPER,	MNULL,
// VOL WGT DAM HIT	TYPE		LEV REQ FUN INT TIME
    8,  6,  5,  1,	"cooking",	 6,  3, -1, 12, 35, "\
A college textbook on chemistry.");

BOOK("textbook_carpentry", "Engineering 301",		 6, 550,c_blue,		PAPER,	MNULL,
    6,  3,  4,  1,	"carpentry",	 6,  3, -1,  8, 30, "\
A textbook on civil engineering and construction.");

BOOK("SICP", "SICP",			 3, 780,c_blue,		PAPER,	MNULL,
    6,  5,  6,  0,	"computer",	 8,  4, -1, 13, 50, "\
A classic text, \"The Structure and Interpretation of Computer Programs.\"\n\
Written with examples in LISP, but applicable to any language.");

BOOK("textbook_robots", "Robots for Fun & Profit",  1, 920,c_blue,		PAPER,	MNULL,
    8,  8,  8,  1,	"electronics",	10,  5, -1, 14, 55, "\
A rare book on the design of robots, with lots of helpful step-by-step guides."
);

CONT("bag_plastic", "plastic bag",	50,  1,	c_ltgray,	PLASTIC,MNULL,
// VOL WGT DAM HIT	VOL	FLAGS
    1,  0, -8, -4,	24,	0, "\
A small, open plastic bag. Essentially trash.");

CONT("bottle_plastic", "plastic bottle",	70,  8,	c_ltcyan,	PLASTIC,MNULL,
    2,  0, -8,  1,	 2,	mfb(con_rigid)|mfb(con_wtight)|mfb(con_seals),"\
A resealable plastic bottle, holds 500mls of liquid.");

CONT("bottle_glass", "glass bottle",	70, 12,	c_cyan,		GLASS,	MNULL,
    2,  1,  8,  1,	 3,	mfb(con_rigid)|mfb(con_wtight)|mfb(con_seals),"\
A resealable glass bottle, holds 750mls of liquid.");

CONT("can_drink", "aluminum can",	70,  1,	c_ltblue,	STEEL,	MNULL,
    1,  0,  0,  0,	 1,	mfb(con_rigid)|mfb(con_wtight), "\
An aluminum can, like what soda comes in.");

CONT("can_food", "tin can",		65,  2,	c_blue,		IRON,	MNULL,
    1,  0, -1,  1,	 1,	mfb(con_rigid)|mfb(con_wtight), "\
A tin can, like what beans come in.");

CONT("box_small", "sm. cardboard box",50, 0,	c_brown,	PAPER,	MNULL,
    4,  0, -5,  1,	 4,	mfb(con_rigid), "\
A small cardboard box. No bigger than a foot in any dimension.");

CONT("canteen", "plastic canteen",	20,  1000,	c_green,	PLASTIC,MNULL,
    6,  2, -8,  1,	 6,	mfb(con_rigid)|mfb(con_wtight)|mfb(con_seals),"\
A large military-style water canteen, with a 1.5 liter capacity");

CONT("jerrycan", "plastic jerrycan",	10,  2500,	c_green,	PLASTIC,MNULL,
    40,  4, -2,  -2,	 40,	mfb(con_rigid)|mfb(con_wtight)|mfb(con_seals),"\
A bulky plastic jerrycan, meant to carry fuel, but can carry other liquids\n\
in a pinch. It has a capacity of 10 liters.");

CONT("jug_plastic", "gallon jug",	10,  2500,	c_ltcyan,	PLASTIC,MNULL,
    10,  2, -8,  1,	 10,	mfb(con_rigid)|mfb(con_wtight)|mfb(con_seals),"\
A standard plastic jug used for household cleaning chemicals.");

CONT("flask_glass", "glass flask",	10,  2500,	c_ltcyan,	GLASS,MNULL,
    1,  0, 8,  1,	 1,	mfb(con_rigid)|mfb(con_wtight)|mfb(con_seals),"\
A 250 ml laboratory conical flask, with a rubber bung.");

CONT("waterskin", "waterskin",   0,  0, c_brown, LEATHER, MNULL,
// VOL WGT DAM HIT	VOL	FLAGS
    6, 4,  -8, -5,   6, mfb(con_wtight)|mfb(con_seals), "\
A watertight leather bag, can hold 1.5 liters of water.");

CONT("jerrycan_big", "steel jerrycan", 20, 5000, c_green, STEEL, MNULL,
    100, 7, -3, -3, 100, mfb(con_rigid)|mfb(con_wtight)|mfb(con_seals),"\
A steel jerrycan, meant to carry fuel, but can carry other liquds\n\
in a pinch. It has a capacity of 25 liters.");

CONT("keg", "aluminum keg", 20, 6000, c_ltcyan, STEEL, MNULL,
    200, 12, -4, -4, 200, mfb(con_rigid)|mfb(con_wtight)|mfb(con_seals),"\
A reusable aluminum keg, used for shipping beer.\n\
It has a capcity of 50 liters.");

//	NAME		RAR PRC	SYM  COLOR	MAT1	MAT
TOOL("lighter", "cheap lighter",		60,  35,',', c_blue,	PLASTIC,IRON,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    0,  0,  0,  0,  0, 100,100, 1,  0, AT_NULL,	"null", &iuse::lighter, 0, "\
A lighter must be carried to use various drugs, like cigarettes, or to light\n\
things like molotov cocktails.  You can also use a lighter to light nearby\n\
items on fire.");

TOOL("matches", "matchbook", 60, 10,',', c_blue,     PAPER, MNULL,
    0,  0,  0,  0,  0,   20, 20, 1,  0, AT_NULL, "null", &iuse::lighter, 0, "\
Matches must be carried to use various drugs, like cigarettes, or to light\n\
things like molotov cocktails.  You can also use matches to light nearby\n\
items on fire.");

TOOL("fire_drill", "fire drill",    20,  5, ',', c_blue,   WOOD, MNULL,
    1,  1,  0,  0,  0,  20, 20, 1,  0, AT_NULL, "null", &iuse::primitive_fire, 0, "\
A fire drill is a simple item for firestarting, made from two pieces of wood\n\
and some string. Although it is constructed out of simple materials, it's\n\
slow and rather difficult to get a fire started with this tool.");

TOOL("sewing_kit", "sewing kit",	30,120, ',', c_red,	PLASTIC,IRON,
    2,  0, -3,  0, -1,  200, 50, 1,  0, AT_THREAD, "null", &iuse::sew, 0, "\
Use a sewing kit on an article of clothing to attempt to repair or reinforce\n\
that clothing. This uses your tailoring skill.");

TOOL("scissors", "scissors",	50,  45,',', c_ltred,	IRON,	PLASTIC,
    1,  1,  0,  8, -1,   0,  0, 0,  0, AT_NULL, "null", &iuse::scissors,
mfb(IF_SPEAR), "\
Use scissors to cut items made from cotton (mostly clothing) into rags.");

TOOL("hammer", "hammer",		35, 70, ';', c_brown,	IRON,	WOOD,
    2,  5, 17,  0,  1,   0,  0, 0,  0, AT_NULL, "null", &iuse::hammer, 0, "\
Use a hammer, with nails and two by fours in your inventory, to board up\n\
adjacent doors and windows.");

//	NAME		RAR PRC	SYM  COLOR	MAT1	MAT
TOOL("extinguisher", "fire extinguisher",20,700,';', c_red,	IRON,	MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
   16, 20, 26,  0, -1,  80, 80, 1,  0, AT_NULL, "null", &iuse::extinguisher,0,
"Use a fire extinguisher to put out adjacent fires.");

TOOL("battery_compartment", "extra battery mod", 0, 500, ';', c_white, PLASTIC, IRON,
    1,  1,  0,  0,  -2, 0,  0,  0,  0, AT_NULL, "null", &iuse::extra_battery, 0,
"With enough electronics skill, you could attach this to your devices to increase their battery capacity.");

TOOL("flashlight", "flashlight (off)",40, 380,';', c_blue,	PLASTIC, IRON,
    3,  2,  1,  0,  2, 100,100, 0,  0, AT_BATT, "null", &iuse::light_off,0,"\
Using this flashlight will turn it on, assuming it is charged with batteries.\n\
A turned-on flashlight will provide light during the night or while\n\
underground.");

TOOL("flashlight_on", "flashlight (on)",  0, 380,';', c_blue,	PLASTIC, IRON,
    3,  2,  1,  0,  2, 100,100, 0, 15, AT_BATT,"flashlight",&iuse::light_on,
mfb(IF_LIGHT_20),
"This flashlight is turned on, and continually draining its batteries. It\n\
provides light during the night or while underground. Use it to turn it off.");

TOOL("lightstrip_dead", "lightstrip (dead)", 0, 200, ';', c_white, PLASTIC, IRON,
    1,  1,  1,  0,  2,  0,  0,  0,  0, AT_NULL, "null", &iuse::none, 0,"\
A burnt-out lightstrip. You could disassemble this to recover the amplifier\n\
circuit.");

TOOL("lightstrip_inactive", "lightstrip (inactive)", 0, 200, ';', c_white, PLASTIC, IRON,
    1,  1,  1,  0,  2,  12000, 12000, 0, 0, AT_NULL, "null", &iuse::lightstrip, 0,"\
A light-emitting circuit wired directly to some batteries. Once activated, provides\n\
25 hours of light per 3 (battery) charges. When the batteries die, you'll need to\n\
scrap it to recover the components that are reusable.");

TOOL("lightstrip", "lightstrip (active)", 0, 200, ';', c_green, PLASTIC, IRON,
    1,  1,  1,  0,  2,  12000, 12000, 0, 150, AT_NULL, "lightstrip_dead", &iuse::lightstrip_active, mfb(IF_LIGHT_1),"\
A light-emitting circuit wired directly to some batteries. Provides a weak light,\n\
lasting 25 hours per 3 (battery) charges. When the batteries die, you'll need to\n\
scrap it to recover the components that are reusable.");

TOOL("glowstick", "glowstick", 60, 300, ';', c_ltblue, PLASTIC, MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    1,  0,  0,  0, -1, 1200, 1200, 0, 0, AT_NULL, "null", &iuse::glowstick,0,"\
A small blue light glowstick, bend it to break the glass cylinder inside and\n\
start the reaction to produce a very small amount of light.");

TOOL("glowstick_lit", "active glowstick", 0, 300, ';', c_ltblue, PLASTIC, MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    1,  0,  0,  0, -1, 1200, 1200, 0, 2, AT_NULL, "null", &iuse::glowstick_active,mfb(IF_LIGHT_1),"\
This glowstick is active and producing light. It will last for a few hours\n\
before burning out.");

TOOL("hotplate", "hotplate",	10, 250,';', c_green,	IRON,	PLASTIC,
    5,  6,  8,  0, -1, 200, 100,  0,  0, AT_BATT, "null", &iuse::cauterize_elec,0,"\
A small heating element. Indispensable for cooking and chemistry. Try not to\n\
burn yourself.");

//	NAME		RAR PRC	SYM  COLOR	MAT1	MAT
TOOL("soldering_iron", "soldering iron",	70, 200,',', c_ltblue,	IRON,	MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    3,  1,  2,  6,  0, 200, 20,  0,  0, AT_BATT, "null", &iuse::cauterize_elec,
mfb(IF_SPEAR), "\
A piece of metal that can get very hot. Necessary for electronics crafting.\n\
You could also use it to cauterize wounds, if you had to.");

TOOL("water_purifier", "water purifier",   5,1200,';', c_ltblue,	PLASTIC, IRON,
   12, 20,  2,  0, -3, 100,100, 1,  0, AT_BATT,"null",&iuse::water_purifier,0,
"Using this item on a container full of water will purify the water. Water\n\
taken from uncertain sources like a river may be dirty.");

TOOL("two_way_radio", "two-way radio",	10, 800,';', c_yellow,	PLASTIC, IRON,
    2,  3, 10,  0,  0, 100,100, 1,  0, AT_BATT, "null",&iuse::two_way_radio,0,
"Using this allows you to send out a signal; either a general SOS, or if you\n\
are in contact with a faction, to send a direct call to them.");

TOOL("radio", "radio (off)",	20, 420,';', c_yellow,	PLASTIC, IRON,
    4,  2,  4,  0, -1, 100,100, 0,  0, AT_BATT,	"null", &iuse::radio_off, 0,"\
Using this radio turns it on. It will pick up any nearby signals being\n\
broadcast and play them audibly.");

//	NAME		RAR PRC	SYM  COLOR	MAT1	MAT
TOOL("radio_on", "radio (on)",	 0, 420,';', c_yellow,	PLASTIC, IRON,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    4,  2,  4,  0, -1, 100,100, 0,  8, AT_BATT, "radio",&iuse::radio_on, 0,"\
This radio is turned on, and continually draining its batteries. It is\n\
playing the broadcast being sent from any nearby radio towers.");

TOOL("noise_emitter", "noise emitter (off)", 0, 600, ';', c_yellow, PLASTIC, IRON,
    4,  3,  6,  0, -1, 100,100, 0,  0, AT_BATT, "null", &iuse::noise_emitter_off, 0,"\
This device was constructed by 'enhancing' a radio with some amplifier\n\
circuits. It's completely lost its ability to pick up a station, but it's\n\
nice and loud now. Could be useful to distract zombies.");

TOOL("noise_emitter_on", "noise emitter (on)", 0, 600, ';', c_yellow, PLASTIC, IRON,
    4,  3,  6,  0, -1, 100,100, 0,  1, AT_BATT, "noise_emitter",&iuse::noise_emitter_on, 0,"\
This device has been turned on and is emitting horrible sounds of radio\n\
static. Quick, get away from it before it draws zombies to you!");

TOOL("roadmap", "road map", 40, 10, ';', c_yellow, PAPER, MNULL,
     1, 0, 0, 0, -1, 1, 1, 0, 0, AT_NULL, "null", &iuse::roadmap, 0, "\
A road map. Use it to read points of interest, including, but not\n\
limited to, location(s) of hospital(s) nearby.");

TOOL("crowbar", "crowbar",		18, 130,';', c_ltblue,	IRON,	MNULL,
    4,  9, 16,  3,  2,  0,  0,  0,  0, AT_NULL,	"null", &iuse::crowbar, 0,"\
A prying tool. Use it to open locked doors without destroying them, or to\n\
lift manhole covers.");

TOOL("hoe", "hoe",		30,  90,'/', c_brown,	IRON,	WOOD,
   14, 14, 12, 10,  3,  0,  0,  0,  0, AT_NULL, "null", &iuse::makemound,
	mfb(IF_STAB), "\
A farming implement. Use it to turn tillable land into a slow-to-cross pile\n\
of dirt.");

TOOL("shovel", "shovel",		40, 100,'/', c_brown,	IRON,	WOOD,
   16, 18, 15,  5,  3,  0,  0,  0,  0, AT_NULL,	"null", &iuse::dig, 0, "\
A digging tool. Use it to dig pits adjacent to your location.");

TOOL("chainsaw_off", "chainsaw (off)",	 7, 350,'/', c_red,	IRON,	PLASTIC,
   12, 40, 10,  0, -4, 400, 0,  0,  0, AT_GAS,	"null", &iuse::chainsaw_off,0,
"Using this item will, if loaded with gas, cause it to turn on, making a very\n\
powerful, but slow, unwieldy, and noisy, melee weapon.");
TECH("chainsaw_off",  mfb(TEC_SWEEP) );

//	NAME		RAR VAL	SYM  COLOR	MAT1	MAT
TOOL("chainsaw_on", "chainsaw (on)",	 0, 350,'/', c_red,	IRON,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
   12, 40,  4, 70, -5, 400, 0,  0,  1, AT_GAS,	"chainsaw_off",
	&iuse::chainsaw_on, mfb(IF_MESSY), "\
This chainsaw is on, and is continuously draining gasoline. Use it to turn\n\
it off.");
TECH("chainsaw_on",  mfb(TEC_SWEEP) );

TOOL("jackhammer", "jackhammer",	 2, 890,';', c_magenta,	IRON,	MNULL,
   26, 54, 20,  6, -4, 120,  0,10,  0, AT_GAS,	"null", &iuse::jackhammer,0,"\
This jackhammer runs on gasoline. Use it (if loaded) to blast a hole in\n\
adjacent solid terrain.");

TOOL("jacqueshammer", "jacqueshammer",     2, 890,  ';', c_magenta, IRON,   MNULL,
   26, 54, 20,  6, -4, 120,  0,10,  0, AT_GAS,  "null", &iuse::jacqueshammer,0,"\
Ce jacqueshammer marche a l'essence. Utilisez-le (si charge)\n\
pour creuser un trou dans un terrain solide adjacent.");

TOOL("bubblewrap", "bubblewrap",	50,  40,';', c_ltcyan,	PLASTIC,MNULL,
    2,  0, -8,  0,  0,  0,  0,  0,  0, AT_NULL,	"null", &iuse::set_trap,0,"\
A sheet of plastic covered with air-filled bubbles. Use it to set it on the\n\
ground, creating a trap that will warn you with noise when something steps on\n\
it.");

TOOL("beartrap", "bear trap",	 5, 120,';', c_cyan,	IRON,	MNULL,
    12, 10,  9,  1, -2,  0,  0,  0,  0, AT_NULL,	"null", &iuse::set_trap,0,"\
A spring-loaded pair of steel jaws. Use it to set it on the ground, creating\n\
a trap that will ensnare and damage anything that steps on it. If you are\n\
carrying a shovel, you will have the option of burying it.");

//	NAME		RAR VAL	SYM  COLOR	MAT1	MAT
TOOL("board_trap", "nailboard trap",	 0, 30, ';', c_brown,	WOOD,	MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
   18, 18, 12,  6, -3,  0,  0,  0,  0, AT_NULL, "null", &iuse::set_trap,0,"\
Several pieces of wood, nailed together, with nails sticking straight up. If\n\
an unsuspecting victim steps on it, they'll get nails through the foot.");

TOOL("tripwire", "tripwire trap",	 0, 35, ';', c_ltgray,	PAPER,	MNULL,
    1,  0,-10,  0, -1,  0,  0,  0,  0, AT_NULL, "null", &iuse::set_trap,0,"\
A tripwire trap must be placed across a doorway or other thin passage. Its\n\
purpose is to trip up bypassers, causing them to stumble and possibly hurt\n\
themselves minorly.");

TOOL("crossbow_trap", "crossbow trap",	 0,600, ';', c_green,	IRON,	WOOD,
    7, 10,  4,  0, -2,  0,  0,  0,  0, AT_NULL,	"null", &iuse::set_trap,0,"\
A simple tripwire is attached to the trigger of a loaded crossbow. When\n\
pulled, the crossbow fires. Only a single round can be used, after which the\n\
trap is disabled.");

TOOL("shotgun_trap", "shotgun trap",	 0,450, ';', c_red,	IRON,	WOOD,
    7, 11, 12,  0, -2,  0,  0,  0,  0, AT_NULL,	"null", &iuse::set_trap,0,"\
A simple tripwire is attached to the trigger of a loaded sawn-off shotgun.\n\
When pulled, the shotgun fires. Two rounds are used; the first time the\n\
trigger is pulled, one or two may be used.");

//	NAME		RAR VAL	SYM  COLOR	MAT1	MAT
TOOL("blade_trap", "blade trap",	 0,500, ';', c_ltgray,	IRON,	MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
   20, 21,  4, 16, -4,  0,  0,  0,  0, AT_NULL,	"null", &iuse::set_trap,0,"\
A machete is attached laterally to a motor, with a tripwire controlling its\n\
throttle. When the tripwire is pulled, the blade is swung around with great\n\
force. The trap forms a 3x3 area of effect.");

TOOL("light_snare_kit", "light snare kit",   0,100, ';', c_brown,  WOOD,   MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL REVER    FUNCTION
   1, 3,  0, 10, 0,  0,  0,  0,  0, AT_NULL, "null", &iuse::set_trap,0,"\
A kit for a simple trap consisting of a string noose and a snare trigger.\n\
Requires a young tree nearby. Effective at trapping and killing some small\n\
animals.");

TOOL("heavy_snare_kit", "heavy snare kit",   0,250, ';', c_brown,  WOOD,   MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL REVER    FUNCTION
   1, 5,  0, 20, 0,  0,  0,  0,  0, AT_NULL, "null", &iuse::set_trap,0,"\
A kit for a simple trap consisting of a rope noose and a snare trigger.\n\
Requires a tree nearby. Effective at trapping monsters.");

TOOL("landmine", "land mine",	 3,2400,';', c_red,	IRON,	MNULL,
    5,  6, 10,  0, -1,  0,  0,  0,  0, AT_NULL, "null", &iuse::set_trap,0,"\
An anti-personnel mine that is triggered when stepped upon.");

TOOL("geiger_off", "geiger ctr (off)", 8, 300,';', c_green,	PLASTIC,STEEL,
    2,  2,  2,  0,  0,100,100,  1,  0, AT_BATT,	"null", &iuse::geiger,0,"\
A tool for measuring radiation. Using it will prompt you to choose whether\n\
to scan yourself or the terrain, or to turn it on, which will provide\n\
continuous feedback on ambient radiation.");

TOOL("geiger_on", "geiger ctr (on)",	0, 300, ';', c_green,	PLASTIC,STEEL,
    2,  2,  2,  0,  0,100,100,  0, 10, AT_BATT, "geiger_off",&iuse::geiger,0,
"A tool for measuring radiation. It is in continuous scan mode, and will\n\
produce quiet clicking sounds in the presence of ambient radiation. Using it\n\
allows you to turn it off, or scan yourself or the ground.");

//	NAME		RAR VAL	SYM  COLOR	MAT1	MAT
TOOL("teleporter", "teleporter",      10,6000,';', c_magenta,	PLASTIC,STEEL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    3, 12,  4,  0, -1, 20, 20,  1,  0, AT_PLUT,	"null", &iuse::teleport,0,"\
An arcane device, powered by plutonium fuel cells. Using it will cause you\n\
to teleport a short distance away.");

TOOL("canister_goo", "goo canister",     8,3500,';', c_dkgray,  STEEL,	MNULL,
    6, 22,  7,  0,  1,  1,  1,  1,  0, AT_NULL,	"null", &iuse::can_goo,0,"\
\"Warning: contains highly toxic and corrosive materials. Contents may be\n\
 sentient. Open at your own risk.\"");

TOOL("pipebomb", "pipe bomb",	 4, 150,'*', c_white,	IRON,	MNULL,
    2,  3, 11,  0,  1,  0,  0,  0,  0, AT_NULL,	"null", &iuse::pipebomb,0,"\
A section of a pipe filled with explosive materials. Use this item to light\n\
the fuse, which gives you 3 turns before it detonates. You will need a\n\
lighter. It is somewhat unreliable, and may fail to detonate.");

TOOL("pipebomb_act", "active pipe bomb", 0,   0,'*', c_white,	IRON,	MNULL,
    2,  3, 11,  0,  1,  3,  3,  0,  1, AT_NULL,	"null", &iuse::pipebomb_act,0,
"This pipe bomb's fuse is lit, and it will explode any second now. Throw it\n\
immediately!");

//	NAME		RAR VAL	SYM  COLOR	MAT1	MAT
TOOL("grenade", "grenade",		 3, 400,'*', c_green,	IRON,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    1,  1, 10,  0, -1,  0,  0,  0,  0, AT_NULL, "null", &iuse::grenade,0,"\
Use this item to pull the pin, turning it into an active grenade. You will\n\
then have five turns before it explodes; throwing it would be a good idea.");

TOOL("grenade_act", "active grenade",	 0,   0,'*', c_green,	IRON,	PLASTIC,
    1,  1, 10,  0, -1,  5,  5,  0,  1, AT_NULL, "null", &iuse::grenade_act,0,
"This grenade is active, and will explode any second now. Better throw it!");

TOOL("flashbang", "flashbang",	 3, 380,'*', c_white,	STEEL,	PLASTIC,
    1,  1,  8,  0, -1,  0,  0,  0,  0, AT_NULL,	"null", &iuse::flashbang,0,"\
Use this item to pull the pin, turning it into an active flashbang. You will\n\
then have five turns before it detonates with intense light and sound,\n\
blinding, deafening and disorienting anyone nearby.");

TOOL("flashbang_act", "active flashbang", 0,   0,'*', c_white,	STEEL,	PLASTIC,
    1,  1,  8,  0, -1,  5,  5,  0,  1, AT_NULL,	"null", &iuse::flashbang_act,
0,"This flashbang is active, and will soon detonate with intense light and\n\
sound, blinding, deafening and disorienting anyone nearby.");

//	NAME		RAR VAL	SYM  COLOR	MAT1	MAT
TOOL("EMPbomb", "EMP grenade",	 2, 600,'*', c_cyan,	STEEL,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    1,  1,  8,  0, -1,  0,  0,  0,  0, AT_NULL,	"null", &iuse::EMPbomb,0,"\
Use this item to pull the pin, turning it into an active EMP grenade. You\n\
will then have three turns before it detonates, creating an EMP field that\n\
damages robots and drains bionic energy.");

TOOL("EMPbomb_act", "active EMP grenade",0,  0,'*', c_cyan,	STEEL,	PLASTIC,
    1,  1,  8,  0, -1,  3,  3,  0,  1, AT_NULL,	"null", &iuse::EMPbomb_act,0,
"This EMP grenade is active, and will shortly detonate, creating a large EMP\n\
field that damages robots and drains bionic energy.");

//	NAME		RAR VAL	SYM  COLOR	MAT1	MAT
TOOL("scrambler", "scrambler grenade",	 2, 600,'*', c_cyan,	STEEL,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    1,  1,  8,  0, -1,  0,  0,  0,  0, AT_NULL,	"null", &iuse::scrambler,0,"\
This is a highly modified EMP grenade, designed to scramble robots' control\n\
chips, rather than destroy them. This converts the robot to your side for a \n\
short time, before the backup systems kick in.");

TOOL("scrambler_act", "active scrambler grenade",0,  0,'*', c_cyan,	STEEL,	PLASTIC,
    1,  1,  8,  0, -1,  3,  3,  0,  1, AT_NULL,	"null", &iuse::scrambler_act,0,
"This scrambler grenade is active, and will soon detonate, releasing a control\n\
wave that temporarily converts robots to your side.");

TOOL("gasbomb", "teargas canister",3,  600,'*', c_yellow,	STEEL, MNULL,
    1,  1,  6,  0, -1,  0,  0,  0,  0, AT_NULL, "null", &iuse::gasbomb,0,"\
Use this item to pull the pin. Five turns after you do that, it will begin\n\
to expell a highly toxic gas for several turns. This gas damages and slows\n\
those who enter it, as well as obscuring vision and scent.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("gasbomb_act", "active teargas",	0,    0,'*', c_yellow,	STEEL, MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    1,  1,  6,  0, -1,  5,  5,  0,  1, AT_NULL, "null", &iuse::gasbomb_act,0,
"This canister of teargas has had its pin removed, indicating that it is (or\n\
will shortly be) expelling highly toxic gas.");

TOOL("smokebomb", "smoke bomb",	5,  180,'*', c_dkgray,	STEEL,	MNULL,
    1,  1,  5,  0, -1,  0,  0,  0,  0, AT_NULL,	"null", &iuse::smokebomb,0,"\
Use this item to pull the pin. Five turns after you do that, it will begin\n\
to expell a thick black smoke. This smoke will slow those who enter it, as\n\
well as obscuring vision and scent.");

TOOL("smokebomb_act", "active smoke bomb",0,  0, '*', c_dkgray,	STEEL,	MNULL,
    1,  1,  5,  0, -1,  0,  0,  0,  1, AT_NULL, "null",&iuse::smokebomb_act,0,
"This smoke bomb has had its pin removed, indicating that it is (or will\n\
shortly be) expelling thick smoke.");

TOOL("molotov", "molotov cocktail",0,  200,'*', c_ltred,	GLASS,	COTTON,
    2,  2,  8,  0,  1,  0,  0,  0,  0, AT_NULL,	"null", &iuse::molotov,0,"\
A bottle of flammable liquid with a rag inserted. Use this item to light the\n\
rag; you will, of course, need a lighter in your inventory to do this. After\n\
lighting it, throw it to cause fires.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("molotov_lit", "molotov cocktail (lit)",0,0,'*', c_ltred,	GLASS,	COTTON,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    2,  2,  8,  0,  1,  1,  1,  0,  0, AT_NULL,"molotov",&iuse::molotov_lit,0,
"A bottle of flammable liquid with a flaming rag inserted. Throwing it will\n\
cause the bottle to break, spreading fire. The flame may go out shortly if\n\
you do not throw it. Dropping it while lit is not safe.");

TOOL("acidbomb", "acid bomb", 	  0,500,'*', c_yellow,	GLASS,	MNULL,
     1,  1,  4,  0, -1,  0,  0,  0,  0,AT_NULL,	"null", &iuse::acidbomb,0,"\
A glass vial, split into two chambers. The divider is removable, which will\n\
cause the chemicals to mix. If this mixture is exposed to air (as happens\n\
if you throw the vial) they will spill out as a pool of potent acid.");

TOOL("acidbomb_act", "acid bomb (active)",0,  0,'*', c_yellow, GLASS, MNULL,
    1,  1,  4,  0,  -1,  0,  0,  0,  0,AT_NULL,"null",&iuse::acidbomb_act,0,"\
A glass vial, with two chemicals mixing inside. If this mixture is exposed\n\
to air (as happens if you throw the vial), they will spill out as a pool of\n\
potent acid.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("dynamite", "dynamite",	5,  700,'*', c_red,	PLASTIC,MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    6, 10,  4,  0, -3,  0,  0,  0,  0, AT_NULL,	"null", &iuse::dynamite,0,"\
Several sticks of explosives with a fuse attached. Use this item to light\n\
the fuse; you will, of course, need a lighter in your inventory to do this.\n\
Shortly after lighting the fuse, this item will explode, so get away!");

TOOL("dynamite_act", "dynamite (lit)",	5,    0,'*', c_red,	PLASTIC,MNULL,
    6, 10,  4,  0, -3,  0,  0,  0,  1, AT_NULL,	"null", &iuse::dynamite_act,0,
"The fuse on this dynamite is lit and hissing.  It'll explode any moment now.");

TOOL("firecracker_pack", "pack of firecrackers",    5,  100,'*', c_red, PAPER, MNULL,
    0, 0,  1,  0, -3,  25,  25,  0,  0, AT_NULL, "null", &iuse::firecracker_pack,0,"\
A pack of 25 firecrackers with a starter fuse. Use this item to light the\n\
fuse; you will need a lighter of course. Shortly after you light the fuse\n\
they will begin to explode, so throw them quickly!");

TOOL("firecracker_pack_act", "pack of firecrackers (lit)",    5,  0,'*', c_red, PAPER, MNULL,
    0, 0,  0,  0, -3,  0,  0,  0,  0, AT_NULL, "null", &iuse::firecracker_pack_act,0,"\
A pack of 25 firecrackers that has been lit, the fuse is hissing.\n\
Throw them quickly before the start to explode.");

TOOL("firecracker", "firecracker",    5,  2,';', c_red, PAPER, MNULL,
    0, 0,  1,  0, -3,  0,  0,  0,  0, AT_NULL, "null", &iuse::firecracker,0,"\
A firecracker with a short fuse. Use this item to light the fuse; you will\n\
need a lighter of course. Shortly after you light the fuse it will explode,\n\
so throw it quickly!");

TOOL("firecracker_act", "firecracker (lit)",    5,  0,';', c_red, PAPER, MNULL,
    0, 0,  1,  0, -3,  0,  0,  0,  1, AT_NULL, "null", &iuse::firecracker_act,0,"\
A firecracker that has been lit, the fuse is hissing. Throw it quickly before\n\
it explodes.");

TOOL("mininuke", "mininuke",	1, 1800,'*', c_ltgreen,	STEEL,	PLASTIC,
    3,  4,  8,  0, -2,  0,  0,  0,  0, AT_NULL, "null", &iuse::mininuke,0,"\
An extremely powerful weapon--essentially a hand-held nuclear bomb. Use it\n\
to activate the timer. Ten turns later it will explode, leaving behind a\n\
radioactive crater. The explosion is large enough to take out a house.");

TOOL("mininuke_act", "mininuke (active)",0,   0,'*', c_ltgreen,	STEEL,	PLASTIC,
    3,  4,  8,  0, -2,  0,  0,  0,  1, AT_NULL, "null", &iuse::mininuke_act,0,
"This miniature nuclear bomb has a light blinking on the side, showing that\n\
it will soon explode. You should probably get far away from it.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("pheromone", "zombie pheromone",1,  400,'*', c_yellow,	FLESH,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    1,  1, -5,  0, -1,  3,  3,  1,  0, AT_NULL,	"null", &iuse::pheromone,0,"\
This is some kind of disgusting ball of rotting meat. Squeezing it causes a\n\
small cloud of pheromones to spray into the air, causing nearby zombies to\n\
become friendly for a short period of time.");

TOOL("portal", "portal generator",2, 6600, ';', c_magenta, STEEL,	PLASTIC,
    2, 10,  6,  0, -1, 10, 10,  5,  0, AT_NULL,	"null", &iuse::portal,0,"\
A rare and arcane device, covered in alien markings.");

TOOL("bot_manhack", "inactive manhack",1,  600, ',', c_ltgreen, STEEL, PLASTIC,
    1,  3,  6,  6, -3,  0,  0,  0,  0, AT_NULL, "null", &iuse::manhack,0,"\
An inactive manhack. Manhacks are fist-sized robots that fly through the\n\
air. They are covered with whirring blades and attack by throwing themselves\n\
against their target. Use this item to activate the manhack.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("bot_turret", "inactive turret",  1,4000,';',c_ltgreen,	STEEL,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    12, 12, 8,  0, -3,  0,  0,  0,  0, AT_NULL, "null", &iuse::turret,0,"\
An inactive turret. Using this item involves turning it on and placing it\n\
on the ground, where it will attach itself. The turret will then identify\n\
you as a friendly, and attack all enemies with an SMG.");

TOOL("UPS_off", "UPS (off)",	 5,2800,';',c_ltgreen,	STEEL,	PLASTIC,
   12,  6, 10,  0, -1,1000, 0,  0,  0, AT_BATT, "null", &iuse::UPS_off,0,"\
A unified power supply, or UPS, is a device developed jointly by military and\n\
scientific interests for use in combat and the field. The UPS is designed to\n\
power armor and some guns, but drains batteries quickly.");

TOOL("UPS_on", "UPS (on)",	 0,2800,';',c_ltgreen,	STEEL,	PLASTIC,
   12,  6, 10,  0, -1,1000, 0,  0,  2, AT_BATT,	"UPS_off", &iuse::UPS_on,0,"\
A unified power supply, or UPS, is a device developed jointly by military and\n\
scientific interests for use in combat and the field. The UPS is designed to\n\
power armor and some guns, but drains batteries quickly.");

TOOL("tazer", "tazer",		 3,1400,';',c_ltred,	IRON,	PLASTIC,
    1,  3,  6,  0, -1, 500, 0,100, 0, AT_BATT, "null", &iuse::tazer,0,"\
A high-powered stun gun. Use this item to attempt to electrocute an adjacent\n\
enemy, damaging and temporarily paralyzing them. Because the shock can\n\
actually jump through the air, it is difficult to miss.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("mp3", "mp3 player (off)",18, 800,';',c_ltblue,	IRON,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    1,  1,  0,  0,  0, 100,100, 0,  0, AT_BATT, "null", &iuse::mp3,0,"\
This battery-devouring device is loaded up with someone's music collection.\n\
Fortunately, there's lots of songs you like, and listening to it will raise\n\
your morale slightly. Use it to turn it on.");

TOOL("mp3_on", "mp3 player (on)",	 0, 800,';',c_ltblue,	IRON,	PLASTIC,
    1,  1,  0,  0,  0, 100,100, 0, 10, AT_BATT, "mp3", &iuse::mp3_on,0,"\
This mp3 player is turned on and playing some great tunes, raising your\n\
morale steadily while on your person. It runs through batteries quickly; you\n\
can turn it off by using it. It also obscures your hearing.");

TOOL("vortex_stone", "vortex stone",     2,3000,';',c_pink,	STONE,	MNULL,
    2,  0,  6,  0,  0,   1,  1, 1,  0, AT_NULL, "null", &iuse::vortex,0,"\
A stone with spirals all over it, and holes around its perimeter. Though it\n\
is fairly large, it weighs next to nothing. Air seems to gather around it.");

TOOL("dogfood", "dog food",         5,  60,';',c_red,     FLESH,     MNULL,
    1,  2,  0,  0,  -5,  0,  0,  0,  0, AT_NULL, "null", &iuse::dogfood, 0, "\
Food for dogs. It smells strange, but dogs love it.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("boobytrap", "booby trap",        0,500,';',c_ltcyan,   STEEL,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
     3,  2,  0,  0, -4,  0,  0,  0,  0, AT_NULL, "null", &iuse::set_trap, 0, "\
A crude explosive device triggered by a piece of string.");

TOOL("c4", "C4-Explosive",      5, 6000,';',c_ltcyan, PLASTIC,     STEEL,
     6,  2,  0,  0, -4,  0,  0,  0,  0, AT_NULL, "null", &iuse::c4, 0, "\
Highly explosive, use with caution! Armed with a small timer.");

TOOL("c4armed", "C4-Explosive(armed)",0,6000,';',c_ltcyan, PLASTIC,     STEEL,
     6,  2,  0,  0, -4,  9,  9,  0,  1, AT_NULL, "null", &iuse::c4armed, 0, "\
Highly explosive, use with caution. Comes with a small timer.\n\
It's armed and ticking!");

TOOL("dog_whistle", "dog whistle",	  0,  300,';',c_white,	STEEL,	MNULL,
     0,  0,  0,  0,  0,  0,  0,  0,  0, AT_NULL, "null", &iuse::dog_whistle,
0, "\
A small whistle. When used, it produces a high tone that causes nearby\n\
friendly dogs to either follow you closely and stop attacking, or start\n\
attacking enemies if they are currently docile.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("vacutainer", "vacutainer",	 10,300,';', c_ltcyan,	PLASTIC,MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
     1,  0,  0,  6, -3,  0,  0,  0,  0, AT_NULL, "null", &iuse::vacutainer,
mfb(IF_SPEAR), "\
A tool for drawing blood, including a vacuum-sealed test tube for holding the\n\
sample. Use this tool to draw blood, either from yourself or from a corpse\n\
you are standing on.");

//  NAME        RAR PRC SYM  COLOR  MAT1    MAT
TOOL("welder", "welder",   25,900,';', c_ltred,  IRON,MNULL,
// VOL WGT DAM CUT HIT   MAX DEF  USE SEC   FUEL    REVERT    FUNCTION
     6, 24,  7,  0, -1, 500,  0,  50,  0, AT_BATT, "null", &iuse::none,
0, "\
A tool for welding metal pieces together. Useful for construction.");

TOOL("cot", "cot",      40,1000,';', c_green, IRON, COTTON,
     8, 10, 6, 0, -1, 0, 0, 0, 0, AT_NULL, "null", &iuse::set_trap,
0, "\
A military style fold up cot, not quite as comfortable as a bed\n\
but much better than slumming it on the ground.");

TOOL("rollmat", "rollmat",  40,400,';', c_blue, PLASTIC, MNULL,
     4, 3,  0, 0, -1, 0, 0, 0, 0, AT_NULL, "null", &iuse::set_trap,
0, "\
A sheet of foam which can be rolled tightly for storage.\n\
Insulates you from the floor, making it easier to sleep");

TOOL("xacto", "X-Acto knife",	10,  40,';', c_dkgray,	IRON,	PLASTIC,
	 1,  0,  0, 14, -4,  0, 0, 0, 0, AT_NULL, "null", &iuse::knife, mfb(IF_SPEAR), "\
A small, very sharp knife.  Causes decent damage but is difficult to hit\n\
with. Its small tip allows for a precision strike in the hands of the skilled.");
TECH("xacto", mfb(TEC_PRECISE));

TOOL("scalpel", "scalpel",	48,  40,',', c_cyan,	STEEL,	MNULL,
	 1,  0,  0, 18, -4,  0, 0, 0, 0, AT_NULL, "null", &iuse::knife, mfb(IF_SPEAR), "\
A small, very sharp knife, used in surgery. Its small tip allows for a\n\
precision strike in the hands of the skilled.");
TECH("scalpel", mfb(TEC_PRECISE));

TOOL("machete", "machete",	 5, 280,'/', c_blue,	IRON,	MNULL,
	 8, 14,  6, 28,  2,  0, 0, 0, 0, AT_NULL, "null", &iuse::knife, 0, "\
This huge iron knife makes an excellent melee weapon.");
TECH("machete",  mfb(TEC_WBLOCK_1) );

TOOL("katana", "katana",		 2, 980,'/', c_ltblue,	STEEL,	MNULL,
	16, 16,  4, 45,  1,  0, 0, 0, 0, AT_NULL, "null", &iuse::knife, mfb(IF_STAB), "\
A rare sword from Japan. Deadly against unarmored targets, and still very\n\
effective against armor.");
TECH("katana",  mfb(TEC_RAPID)|mfb(TEC_WBLOCK_2) );

TOOL("spear_knife", "knife spear",      5,  140,'/', c_ltred,   WOOD,   STEEL,
         6,  6,  2, 28,  1,  0, 0, 0, 0, AT_NULL, "null", &iuse::knife, mfb(IF_SPEAR), "\
A simple wood pole made deadlier by the blade tied to it.");
TECH("spear_knife",  mfb(TEC_WBLOCK_1) | mfb(TEC_RAPID) );

TOOL("rapier", "rapier",		 3, 980,'/', c_ltblue,	STEEL,	MNULL,
	 6,  9, 5, 28,  2,  0, 0, 0, 0, AT_NULL, "null", &iuse::knife, mfb(IF_STAB), "\
Preferred weapon of gentlemen and swashbucklers. Light and quick, it makes\n\
any battle a stylish battle.");
TECH("rapier",  mfb(TEC_RAPID)|mfb(TEC_WBLOCK_1)|mfb(TEC_PRECISE) );

TOOL("pike", "awl pike",        5,2000,'/', c_ltcyan,	IRON,	WOOD,
        14, 18,  8, 50,  2,  0, 0, 0, 0, AT_NULL, "null", &iuse::knife, mfb(IF_SPEAR), "\
A medieval weapon consisting of a wood shaft, tipped with an iron spike.\n\
Though large and heavy compared to other spears, its accuracy and damage\n\
are unparalled.");

TOOL("broadsword", "broadsword",	30,1200,'/',c_cyan,	IRON,	MNULL,
	 7, 11,  8, 35,  2,  0, 0, 0, 0, AT_NULL, "null", &iuse::knife, mfb(IF_STAB), "\
An early modern sword seeing use in the 16th, 17th ane 18th centuries.\n\
Called 'broad' to contrast with the slimmer rapiers.");
TECH("broadsword",  mfb(TEC_WBLOCK_1) );

TOOL("makeshift_machete", "makeshift machete", 0, 100, '/', c_ltgray, IRON, MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC   FUEL    REVERT    FUNCTION
    7,  12,  4,  15, 1,  0,  0,  0,  0, AT_NULL, "null", &iuse::knife,
    mfb(IF_STAB), "\
A large blade that has had a portion of the handle wrapped\n\
in duct tape, making it easier to wield as a rough machete.");

TOOL("makeshift_halberd", "makeshift halberd", 0, 100, '/', c_ltgray, IRON, MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC   FUEL    REVERT    FUNCTION
    13, 17, 4, 15,   2,  0, 0, 0, 0, AT_NULL, "null", &iuse::knife, mfb(IF_STAB), "\
A large blade attached to a long stick. Could do a considerable\n\
amount of damage.");
TECH("makeshift_halberd",  mfb(TEC_WBLOCK_1) );

TOOL("knife_steak", "steak knife",	85,  25,';', c_ltcyan,	STEEL,	MNULL,
     1,  2,  2, 10, -3, 0, 0, 0, 0, AT_NULL, "null", &iuse::knife,
 mfb(IF_STAB), "\
A sharp knife. Makes a poor melee weapon, but is decent at butchering\n\
corpses.");

TOOL("knife_butcher", "butcher knife",	10,  80,';', c_cyan,	STEEL,	MNULL,
	 3,  6,  4, 18, -3 ,0, 0, 0, 0, AT_NULL, "null", &iuse::knife, 0, "\
A sharp, heavy knife. Makes a good melee weapon, and is the best item for\n\
butchering corpses.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
TOOL("knife_combat", "combat knife",	14, 100,';', c_blue,	STEEL,  PLASTIC,
//	VOL WGT DAM CUT HIT FLAGS
	 2,  2,  2, 22, -2, 0, 0, 0, 0, AT_NULL, "null", &iuse::knife,
mfb(IF_STAB), "\
Designed for combat, and deadly in the right hands. Can be used to butcher\n\
corpses.");

TOOL("saw", "wood saw",	15, 40, ';', c_cyan,	IRON,	WOOD,
	 7,  3, -6,  1, -2, 0, 0, 0, 0, AT_NULL, "null", &iuse::lumber,
0, "\
A flimsy saw, useful for cutting through wood objects.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
TOOL("ax", "wood axe",	 8, 105,'/', c_ltgray,	WOOD,	IRON,
//	VOL WGT DAM CUT HIT FLAGS
	17, 15, 24, 18,  1, 0, 0, 0, 0, AT_NULL, "null", &iuse::lumber,
0, "\
A large two-handed axe. Makes a good melee weapon, but is a bit slow.");

TOOL("hacksaw", "hack saw",	17, 65, ';', c_ltcyan,	IRON,	MNULL,
	 4,  2,  1,  1, -1, 0, 0, 0, 0, AT_NULL, "null", &iuse::hacksaw,
0, "\
A sturdy saw, useful for cutting through metal objects.");

//  NAME        RAR PRC SYM  COLOR  MAT1    MAT
TOOL("tent_kit", "tent",	17, 65, ';', c_green,	IRON,	MNULL,
	 10,  20,  4,  0, -3, 0, 0, 0, 0, AT_NULL, "null", &iuse::tent,
0, "\
A small tent, just big enough to fit a person comfortably.");

TOOL("torch", "torch",    95,  0, '/', c_brown,   WOOD,   MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL     REVERT    FUNCTION
    6, 10, 12,  0,  3, 25, 25,  0, 0, AT_NULL, "null", &iuse::torch,
0,"\
A large stick, wrapped in gasoline soaked rags. When lit, produces\n\
a fair amount of light");

TOOL("torch_lit", "torch (lit)",    95,  0, '/', c_brown,   WOOD,   MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL     REVERT    FUNCTION
    6, 10, 12,  0,  3, 25, 25,  1, 15, AT_NULL, "torch_done", &iuse::torch_lit,
mfb(IF_FIRE),"\
A large stick, wrapped in gasoline soaked rags. This is burning,\n\
producing plenty of light");

//    NAME              RAR PRC SYM COLOR       MAT1    MAT2
TOOL("candle", "candle",           40,  0, ',', c_white,  VEGGY,  MNULL,
    1,  1,  0,  0, -2, 100, 100, 0, 0, AT_NULL, "can_food", &iuse::candle,
0, "\
A thick candle, doesn't provide very much light, but it can burn for\n\
quite a long time.");

//    NAME              RAR PRC SYM COLOR       MAT1    MAT2
TOOL("candle_lit", "candle (lit)",           40,  0, ',', c_white,  VEGGY,  MNULL,
    1,  1,  0,  0, -2, 100, 100, 1, 50, AT_NULL, "null", &iuse::candle_lit,
mfb(IF_LIGHT_4), "\
A thick candle, doesn't provide very much light, but it can burn for\n\
quite a long time. This candle is lit.");

//  NAME        RAR PRC SYM  COLOR  MAT1    MAT
TOOL("brazier", "brazier",  50,900,';', c_ltred,  IRON,MNULL,
// VOL WGT DAM CUT HIT   MAX DEF  USE SEC   FUEL    REVERT    FUNCTION
     6, 5,  11,  0, 1,    0,  0,  0,  0, AT_NULL, "null", &iuse::set_trap,
0, "\
A large stand with slots in the side. (a)ctivate it and place it somewhere\n\
then set fires in it with no risk of spreading.");

//  NAME        RAR PRC SYM  COLOR  MAT1    MAT
TOOL("funnel", "funnel",  50,900,';', c_yellow,  PLASTIC,MNULL,
// VOL WGT DAM CUT HIT   MAX DEF  USE SEC   FUEL    REVERT    FUNCTION
     6, 5,  11,  0, 1,    0,  0,  0,  0, AT_NULL, "null", &iuse::set_trap,
0, "\
A funnel used to collect rainwater. (a)ctivate it and place it outside\n\
then collect water from it when it rains.");

TOOL("puller", "kinetic bullet puller",		5, 100, ';', c_blue,	STEEL,	PLASTIC,
    2,  4, 10,  0,  0,   0,  0, 0,  0, AT_NULL, "null", &iuse::bullet_puller, 0, "\
A tool used for disassembling firearm ammunition.");

TOOL("press", "hand press & die set",		5, 100, ';', c_blue,	STEEL,	PLASTIC,
    2,  4, 8,  0,  -2,   100, 100, 0,  0, AT_BATT, "null", &iuse::none, 0, "\
A small hand press for hand loading firearm ammunition. Comes with everything \n\
you need to start hand loading.");

TOOL("screwdriver", "screwdriver",	40, 65, ';', c_ltcyan,	IRON,	PLASTIC,
	 1,  1,  2,  8,  1, 0, 0, 0, 0, AT_NULL, "null", &iuse::none, mfb(IF_SPEAR), "\
A Philips-head screwdriver, important for almost all electronics crafting and\n\
most mechanics crafting.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
TOOL("wrench", "wrench",		30, 86, ';', c_ltgray,	IRON,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 2,  5, 15,  0,  2, 0, 0, 0, 0, AT_NULL, "null", &iuse::none, 0, "\
An adjustable wrench. Makes a decent melee weapon, and is used in many\n\
mechanics crafting recipes.");

TOOL("snare_trigger", "snare trigger", 50, 15, ';', c_brown, WOOD, MNULL,
    1, 0, 0, 0, -1, 0, 0, 0, 0, AT_NULL, "null", &iuse::none, 0, "\
A stick that has been cut into a trigger mechanism for a snare trap.");

TOOL("boltcutters", "bolt cutters",		5, 100, ';', c_blue,	STEEL,	PLASTIC,
    5,  4, 10,  4,  1,   0,  0, 0,  0, AT_NULL, "null", &iuse::boltcutters, 0, "\
A large pair of bolt cutters, you could use them to cut padlocks or heavy\n\
gauge wire.");

TOOL("mop", "mop",		30, 24, '/', c_blue, PLASTIC, MNULL,
   10, 8,   6,  0,  1,  0,  0,  0,  0, AT_NULL, "null", &iuse::mop, 0, "\
An unwieldy mop. Good for cleaning up spills.");
TECH("mop",  mfb(TEC_WBLOCK_1) );

TOOL("picklocks", "picklock kit",    20, 0,  ';', c_blue, STEEL,   MNULL,
   0,  0,   0,  0,  0,  0,  0,  0,  0, AT_NULL, "null", &iuse::picklock, 0, "\
A set of sturdy steel picklocks, essential for silently opening locks.");

TOOL("pickaxe", "pickaxe",	60, 160,'/', c_ltred,	WOOD,	MNULL,
   12, 11, 12,  0,  -1, 0,  0,  0,  0, AT_NULL, "null", &iuse::pickaxe, 0, "\
A large steel pickaxe, strike the earth!");

TOOL("spray_can", "spray can", 50, 10, ';', c_ltblue, PLASTIC, MNULL,
1, 1, 0, 0, 0, 10, 10, 1, 0, AT_NULL, "null", &iuse::spray_can, 0, "\
A spray can, filled with paint. Use this tool to make graffiti on the floor.");

TOOL("rag", "rag",    1, 0,  ',', c_white, COTTON,   MNULL,
   1,  1,   0,  0,  0,  0,  0,  0,  0, AT_NULL, "null", &iuse::rag, 0, "\
Rag, useful in crafting and possibly stopping bleeding");

//	NAME		RAR PRC	SYM  COLOR	MAT1	MAT
TOOL("pda", "PDA",		60,  35,',', c_blue,	PLASTIC,IRON,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    1,  1,  0,  0,  0, 100,100, 1,  0, AT_NULL,	"null", &iuse::pda, 0, "\
A small multipurpose electronic device. Can be loaded with a variety\n\
of apps, providing all kinds of functionality.");

//	NAME		RAR PRC	SYM  COLOR	MAT1	MAT
TOOL("pda_flashlight", "PDA - Flashlight",		60,  35,',', c_blue,	PLASTIC,IRON,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    1,  1,  0,  0,  0, 100,100, 1,  0, AT_NULL,	"null", &iuse::pda_flashlight, 0, "\
A small multipurpose electronic device. This PDA has its flashlight\n\
app on, and is providing light.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
TOOL("pockknife", "pocket knife",	14, 100,';', c_blue,	STEEL,  PLASTIC,
//	VOL WGT DAM CUT HIT FLAGS
	 0,  2,  0, 10, -4, 0, 0, 0, 0, AT_NULL, "null", &iuse::knife,
mfb(IF_STAB), "\
A small pocket knife, not great for combat, but better than nothing.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
TOOL("needle_bone", "bone needle",     0, 0,';', c_white, FLESH, MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
     0,  0,  0,  0, 0, 200, 0,  1,  0, AT_THREAD, "null", &iuse::sew,
mfb(IF_STAB), "\
A sharp needle made from a bone. It would be useful for making rough\n\
clothing and items");

TOOL("primitive_hammer", "stone hammer",		35, 70, ';', c_ltgray,	STONE,	WOOD,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    2,  8, 12,  0,  0,   0,  0, 0,  0, AT_NULL, "null", &iuse::hammer, 0, "\
A rock affixed to a stick, functions adequately as a hammer, but really\n\
can't compare to a proper hammer.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
TOOL("primitive_axe", "stone axe",	 8, 105,'/', c_ltgray,	WOOD,	STONE,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
	17, 19, 15, 18,  0, 0, 0, 0, 0, AT_NULL, "null", &iuse::lumber,
0, "\
A sharpened stone affixed to a stick, works passably well as ane axe\n\
but really can't compare to a proper axe..");

TOOL("primitive_shovel", "stone shovel",		40, 100,'/', c_brown,	STONE,	WOOD,
   16, 21, 15,  5,  3,  0,  0,  0,  0, AT_NULL,	"null", &iuse::dig, 0, "\
A flattened stone affixed to a stick, works passably well as a shovel\n\
but really can't compare to a real shovel.");

TOOL("digging_stick", "digging stick",		40, 100,'/', c_brown,  WOOD, MNULL,
    6, 10, 12,  0,  3, 0,  0,  0,  0, AT_NULL,	"null", &iuse::dig, 0, "\
A large stick, with the end carved into a blade for digging. Can be used\n\
to dig shallow pits, but not deep ones.");

//  NAME        RAR PRC SYM  COLOR  MAT1    MAT
TOOL("shelter_kit", "shelter kit",	17, 65, ';', c_green,	WOOD,	LEATHER,
	 40,  20,  4,  0, -3, 0, 0, 0, 0, AT_NULL, "null", &iuse::shelter,
0, "\
A small shelter, made of sticks and skins. (a)ctivate it to place.");

//  NAME        RAR PRC SYM  COLOR  MAT1    MAT
TOOL("damaged_shelter_kit", "damaged shelter kit",	17, 65, ';', c_green,	WOOD,	LEATHER,
	 40,  20,  4,  0, -3, 0, 0, 0, 0, AT_NULL, "null", &iuse::none,
0, "\
A small shelter, made of sticks and skins. (a)ctivate it to place.\n\
This shelter has been damaged, and needs repairs.");

TOOL("heatpack", "heatpack",	60, 65, ';', c_blue,	PLASTIC,	MNULL,
	 1,  1,  1,  1,  1, 0, 0, 0, 0, AT_NULL, "null", &iuse::heatpack,
0, "\
A heatpack, used to treat sports injuries and heat food.  Usable only once.");

TOOL("heatpack_used", "used heatpack",	2, 10, ';', c_blue,	PLASTIC,	MNULL,
	 1,  1,  1,  1,  1, 0, 0, 0, 0, AT_NULL, "null", &iuse::none,
0, "\
A heatpack, used to treat sports injuries and heat food.  This one\n\
has been used already and is now useless.");

TOOL("LAW_Packed", "Packed M72 LAW", 30, 500, ')', c_red, STEEL, MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    6, 13,  6,  0,  0, 1,   1,  1,  0, AT_NULL, "null", &iuse::LAW,
0, "\
An M72 LAW, packed in its storage form. (a)ctivate it to pop it out\n\
and make it ready to fire. Once activated, it cannot be repacked.");

TOOL("jar_meat_canned", "sealed jar of canned meat",	50,	75,'%', c_red,		GLASS,	FLESH,
    2,  3,  8,  1,	 3,	1, 1, 1, 0, AT_NULL, "null", &iuse::dejar, 0,"\
Sealed glass jar containing meat.  Activate to open and enjoy.");

TOOL("jar_human_canned", "sealed jar of canned cad",	50,	75,'%', c_red,		GLASS,	FLESH,
    2,  3,  8,  1,	 3,	1, 1, 1, 0, AT_NULL, "null", &iuse::dejar, 0,"\
Sealed glass jar containing human meat.  Activate to open and enjoy.");

TOOL("jar_veggy_canned", "sealed jar of canned veggy",	50,	65, '%', c_green,		GLASS,	VEGGY,
    2,  3,  8,  1,	 3,	1, 1, 1, 0, AT_NULL, "null", &iuse::dejar, 0,"\
Sealed glass jar containing veggy.  Activate to open and enjoy.");

TOOL("jar_apple_canned", "sealed jar of canned apple",	50,	50, '%', c_red,		GLASS,	VEGGY,
    2,  3,  8,  1,	 3,	1, 1, 1, 0, AT_NULL, "null", &iuse::dejar, 0,"\
Sealed glass jar containing a sliced apple.  Activate to open and enjoy.");

// BIONICS
// These are the modules used to install new bionics in the player.  They're
// very simple and straightforward; a difficulty, followed by a NULL-terminated
// list of options.
#define BIO(id, name, rarity, price, color, difficulty, des) \
itypes[id]=new it_bionic(id, rarity,price,name,des,':',\
color, STEEL, PLASTIC, 10, 18, 8, 0, 0, 0, difficulty)

#define BIO_SINGLE(id,rarity,price,color,difficulty) \
     BIO(id, std::string("CBM: ")+bionics[id]->name, rarity,price,color,difficulty, \
           word_rewrap(bionics[id]->description, 50)) \

//  Name                     RAR PRICE    COLOR   DIFFICULTY
BIO("bio_power_storage", "CBM: Power Storage",	24, 3800,	c_green,	 1, "\
Compact Bionics Module that upgrades your power capacity by 4 units. Having\n\
at least one of these is a prerequisite to using powered bionics. You will\n\
also need a power supply, found in another CBM."); // This is a special case, which increases power capacity by 4

 BIO("bio_power_storage_mkII", "CBM: Power Storage Mk. II", 8, 10000, c_green, 1, "\
Compact Bionics Module developed at DoubleTech Industries as a replacement\n\
for the highly sucessful CBM: Power Storage. Increases you power capacity\n\
by 10 units."); // This is another special case, increases power capacity by 10 units

// SOFTWARE
#define SOFTWARE(id, name, price, swtype, power, description) \
itypes[id]=new it_software(id, 0, price, name, description,\
	' ', c_white, MNULL, MNULL, 0, 0, 0, 0, 0, 0, swtype, power)

//Macguffins
#define MACGUFFIN(id, name, price, sym, color, mat1, mat2, volume, wgt, dam, cut,\
                  to_hit, readable, function, description) \
itypes[id]=new it_macguffin(id, 0, price, name, description,\
	sym, color, mat1, mat2, volume, wgt, dam, cut, to_hit, 0, readable,\
	function)

// BIONIC IMPLANTS
// Sometimes a bionic needs to set you up with a dummy weapon, or something
// similar.  For the sake of clarity, no matter what the type of item, place
// them all here.

// power sources
BIO_SINGLE("bio_solar", 2, 3500, c_yellow, 4);
BIO_SINGLE("bio_batteries", 5, 800, c_yellow, 4);
BIO_SINGLE("bio_metabolics", 4, 700, c_yellow, 4);
BIO_SINGLE("bio_furnace", 2, 4500, c_yellow, 4);
BIO_SINGLE("bio_ethanol", 6, 1200, c_yellow, 4);
// utilities
BIO_SINGLE("bio_tools", 3, 8000, c_ltgray, 6);
BIO_SINGLE("bio_storage", 3, 4000, c_ltgray, 7);
BIO_SINGLE("bio_flashlight", 8, 200, c_ltgray, 2);
BIO_SINGLE("bio_lighter", 6, 1300, c_ltgray, 4);
BIO_SINGLE("bio_magnet", 5, 2000, c_ltgray, 2);
// neurological
BIO_SINGLE("bio_memory", 2, 10000, c_pink, 9);
BIO_SINGLE("bio_painkiller", 4, 2000, c_pink, 4);
BIO_SINGLE("bio_alarm", 7, 250, c_pink, 1);
// sensory
BIO_SINGLE("bio_ears", 2, 5000, c_ltblue, 6);
BIO_SINGLE("bio_eye_enhancer", 2, 8000, c_ltblue, 11);
BIO_SINGLE("bio_night_vision", 2, 9000, c_ltblue, 11);
BIO_SINGLE("bio_infrared", 4, 4500, c_ltblue, 6);
BIO_SINGLE("bio_scent_vision", 4, 4500, c_ltblue, 8);
// aquatic
BIO_SINGLE("bio_membrane", 3, 4500, c_blue, 6);
BIO_SINGLE("bio_gills", 3, 4500, c_blue, 6);
// combat augs
BIO_SINGLE("bio_targeting", 2, 6500, c_red, 5);
BIO_SINGLE("bio_ground_sonar", 3, 4500, c_red, 5);
// hazmat
BIO_SINGLE("bio_purifier", 3, 4500, c_ltgreen, 4);
BIO_SINGLE("bio_climate", 4, 3500, c_ltgreen, 3);
BIO_SINGLE("bio_heatsink", 4, 3500, c_ltgreen, 3);
BIO_SINGLE("bio_blood_filter", 4, 3500, c_ltgreen, 3);
// nutritional
BIO_SINGLE("bio_recycler", 2, 8500, c_green, 6);
BIO_SINGLE("bio_digestion", 3, 5500, c_green, 6);
BIO_SINGLE("bio_evap", 3, 5500, c_green, 4);
BIO_SINGLE("bio_water_extractor", 3, 5500, c_green, 5);
// was: desert survival (all dupes)
// melee:
BIO_SINGLE("bio_shock", 2, 5500, c_red, 5);
BIO_SINGLE("bio_heat_absorb", 2, 5500, c_red, 5);
BIO_SINGLE("bio_claws", 2, 5500, c_red, 5);
// armor:
BIO_SINGLE("bio_carbon", 3, 7500, c_cyan, 9);
BIO_SINGLE("bio_armor_head", 3, 3500, c_cyan, 5);
BIO_SINGLE("bio_armor_torso", 3, 3500, c_cyan, 4);
BIO_SINGLE("bio_armor_arms", 3, 3500, c_cyan, 3);
BIO_SINGLE("bio_armor_legs", 3, 3500, c_cyan, 3);
// espionage
BIO_SINGLE("bio_face_mask", 1, 8500, c_magenta, 5);
BIO_SINGLE("bio_scent_mask", 1, 8500, c_magenta, 5);
BIO_SINGLE("bio_cloak", 1, 8500, c_magenta, 5);
BIO_SINGLE("bio_fingerhack", 1, 3500, c_magenta, 2);
// defensive
BIO_SINGLE("bio_ads", 1, 9500, c_ltblue, 7);
BIO_SINGLE("bio_ods", 1, 9500, c_ltblue, 7);
// medical
BIO_SINGLE("bio_nanobots", 3, 9500, c_ltred, 6);
BIO_SINGLE("bio_blood_anal", 3, 3200, c_ltred, 2);
// construction
BIO_SINGLE("bio_resonator", 2, 12000, c_dkgray, 11);
BIO_SINGLE("bio_hydraulics", 3, 4000, c_dkgray, 6);
// super soldier
BIO_SINGLE("bio_time_freeze", 1, 14000, c_white, 11);
BIO_SINGLE("bio_teleport", 1, 7000, c_white, 7);
// ranged combat
BIO_SINGLE("bio_blaster", 13, 2200, c_red, 3);
BIO_SINGLE("bio_laser", 2, 7200, c_red, 5);
BIO_SINGLE("bio_emp", 2, 7200, c_red, 5);
// power armor
BIO_SINGLE("bio_power_armor_interface", 20, 1200, c_yellow, 1);

SOFTWARE("software_useless", "misc software", 300, SW_USELESS, 0, "\
A miscellaneous piece of hobby software. Probably useless.");

SOFTWARE("software_hacking", "hackPRO", 800, SW_HACKING, 2, "\
A piece of hacking software.");

SOFTWARE("software_medical", "MediSoft", 600, SW_MEDICAL, 2, "\
A piece of medical software.");

SOFTWARE("software_math", "MatheMAX", 500, SW_SCIENCE, 3, "\
A piece of mathematical software.");

SOFTWARE("software_blood_data", "infection data", 200, SW_DATA, 5, "\
Medical data on zombie blood.");

MACGUFFIN("note", "note", 0, '?', c_white, PAPER, MNULL, 1, 0, 0, 0, 0,
	true, &iuse::mcg_note, "\
A hand-written paper note.");

MELEE("poppy_flower", "poppy flower",   1, 400,',', c_magenta,	VEGGY,	MNULL,
	 1,  0, -8,  0, -3, 0, "\
A poppy stalk with some petals.");

MELEE("poppy_bud", "a poppy bud",   1, 400,',', c_magenta,	VEGGY,	MNULL,
	 1,  0, -8,  0, -3, 0, "\
Contains some substances commonly produced by mutated poppy flower");

// Finally, add all the keys from the map to a vector of all possible items
for(std::map<std::string,itype*>::iterator iter = itypes.begin(); iter != itypes.end(); ++iter){
    if(iter->first == "null" || iter->first == "corpse" || iter->first == "toolset" || iter->first == "fire" || iter->first == "apparatus"){
        pseudo_itype_ids.push_back(iter->first);
    } else {
        standard_itype_ids.push_back(iter->first);
    }
}

//    NAME		RARE SYM COLOR		MAT1	MAT2
MELEE("bio_claws_weapon", "adamantite claws",0,0,'{', c_pink,	STEEL,	MNULL,
//	VOL WGT DAM CUT HIT
	 2,  0,  8, 16,  4,
 mfb(IF_STAB)|mfb(IF_UNARMED_WEAPON)|mfb(IF_NO_UNWIELD), "\
Short and sharp claws made from a high-tech metal.");

//  NAME		RARE  TYPE	COLOR		MAT
AMMO("bio_fusion_ammo", "Fusion blast",	 0,0, AT_FUSION,c_dkgray,	MNULL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 0,  0, 40,  0, 10,  1,  0,  5, "", mfb(AMMO_INCENDIARY));

//  NAME		RARE	COLOR		MAT1	MAT2
GUN("bio_blaster_gun", "fusion blaster",	 0,0,c_magenta,	STEEL,	PLASTIC,
//	SKILL		AMMO	   VOL WGT MDG HIT DMG ACC REC DUR BST CLIP REL
	"rifle",	AT_FUSION, 12,  0,  0,  0,  0,  4,  0, 10,  0,  1, 500,
"",0);







// Unarmed Styles
#define STYLE(id, name, dam, description, ...) \
itypes[id]=new it_style(id, 0, 0, name, description, '$', \
                              c_white, MNULL, MNULL, 0, 0, dam, 0, 0, 0); \
setvector((static_cast<it_style*>(itypes[id]))->moves, __VA_ARGS__, NULL); \
itypes[id]->item_flags |= mfb(IF_UNARMED_WEAPON); \
martial_arts_itype_ids.push_back(id)

STYLE("style_karate", "karate", 2, "\
Karate is a popular martial art, originating from Japan. It focuses on\n\
rapid, precise attacks, blocks, and fluid movement. A successful hit allows\n\
you an extra dodge and two extra blocks on the following round.",

"quickly punch", TEC_RAPID, 0,
"block", TEC_BLOCK, 2,
"karate chop", TEC_PRECISE, 4
);

STYLE("style_aikido", "aikido", 0, "\
Aikido is a Japanese martial art focused on self-defense, while minimizing\n\
injury to the attacker. It uses defense throws and disarms. Damage done\n\
while using this technique is halved, but pain inflicted is doubled.",

"feint at", TEC_FEINT, 2,
"throw", TEC_DEF_THROW, 2,
"disarm", TEC_DISARM, 3,
"disarm", TEC_DEF_DISARM, 4
);

STYLE("style_judo", "judo", 0, "\
Judo is a martial art that focuses on grabs and throws, both defensive and\n\
offensive. It also focuses on recovering from throws; while using judo, you\n\
will not lose any turns to being thrown or knocked down.",

"grab", TEC_GRAB, 2,
"throw", TEC_THROW, 3,
"throw", TEC_DEF_THROW, 4
);

STYLE("style_tai_chi", "tai chi", 0, "\
Though tai chi is often seen as a form of mental and physical exercise, it is\n\
a legitimate martial art, focused on self-defense. Its ability to absorb the\n\
force of an attack makes your Perception decrease damage further on a block.",

"block", TEC_BLOCK, 1,
"disarm", TEC_DEF_DISARM, 3,
"strike", TEC_PRECISE, 4
);

STYLE("style_capoeira", "capoeira", 1, "\
A dance-like style with its roots in Brazilian slavery, capoeira is focused\n\
on fluid movement and sweeping kicks. Moving a tile will boost attack and\n\
dodge; attacking boosts dodge, and dodging boosts attack.",

"bluff", TEC_FEINT, 1,
"low kick", TEC_SWEEP, 3,
"spin and hit", TEC_COUNTER, 4,
"spin-kick", TEC_WIDE, 5
);

STYLE("style_krav_maga", "krav maga", 4, "\
Originating in Israel, Krav Maga is based on taking down an enemy quickly and\n\
effectively. It focuses on applicable attacks rather than showy or complex\n\
moves. Popular among police and armed forces everywhere.",

"quickly punch", TEC_RAPID, 2,
"block", TEC_BLOCK, 2,
"feint at", TEC_FEINT, 3,
"jab", TEC_PRECISE, 3,
"disarm", TEC_DISARM, 3,
"block", TEC_BLOCK_LEGS, 4,
"counter-attack", TEC_COUNTER, 4,
"disarm", TEC_DEF_DISARM, 4,
"", TEC_BREAK, 4,
"grab", TEC_GRAB, 5
);

STYLE("style_muay_thai", "muay thai", 4, "\
Also referred to as the \"Art of 8 Limbs,\" Muay Thai is a popular fighting\n\
technique from Thailand that uses powerful strikes. It does extra damage\n\
against large or strong opponents.",

"slap", TEC_RAPID, 2,
"block", TEC_BLOCK, 3,
"block", TEC_BLOCK_LEGS, 4,
"power-kick", TEC_BRUTAL, 4,
"counter-attack", TEC_COUNTER, 5
);

STYLE("style_ninjutsu", "ninjutsu", 1, "\
Ninjutsu is a martial art and set of tactics used by ninja in feudal Japan.\n\
It focuses on rapid, precise, silent strikes. Ninjutsu is entirely silent.\n\
It also provides small combat bonuses the turn after moving a tile.",

"quickly punch", TEC_RAPID, 3,
"jab", TEC_PRECISE, 4,
"block", TEC_BLOCK, 4
);

STYLE("style_taekwondo", "taekwondo", 2, "\
Taekwondo is the national sport of Korea, and was used by the South Korean\n\
army in the 20th century. Focused on kicks and punches, it also includes\n\
strength training; your blocks absorb extra damage the stronger you are.",

"block", TEC_BLOCK, 2,
"block", TEC_BLOCK_LEGS, 3,
"jab", TEC_PRECISE, 4,
"brutally kick", TEC_BRUTAL, 4,
"spin-kick", TEC_SWEEP, 5
);

STYLE("style_tiger", "tiger style", 4, "\
One of the five Shaolin animal styles. Tiger style focuses on relentless\n\
attacks above all else. Strength, not Dexterity, is used to determine hits;\n\
you also receive an accumulating bonus for several turns of sustained attack.",

"grab", TEC_GRAB, 4
);

STYLE("style_crane", "crane style", 0, "\
One of the five Shaolin animal styles. Crane style uses intricate hand\n\
techniques and jumping dodges. Dexterity, not Strength, is used to determine\n\
damage; you also receive a dodge bonus the turn after moving a tile.",

"feint at", TEC_FEINT, 2,
"block", TEC_BLOCK, 3,
"", TEC_BREAK, 3,
"hand-peck", TEC_PRECISE, 4
);

STYLE("style_leopard", "leopard style", 3, "\
One of the five Shaolin animal styles. Leopard style focuses on rapid,\n\
strategic strikes. Your Perception and Intelligence boost your accuracy, and\n\
moving a single tile provides an increased boost for one turn.",

"swiftly jab", TEC_RAPID, 2,
"counter-attack", TEC_COUNTER, 4,
"leopard fist", TEC_PRECISE, 5
);

STYLE("style_snake", "snake style", 1, "\
One of the five Shaolin animal styles. Snake style uses sinuous movement and\n\
precision strikes. Perception increases your chance to hit as well as the\n\
damage you deal.",

"swiftly jab", TEC_RAPID, 2,
"feint at", TEC_FEINT, 3,
"snakebite", TEC_PRECISE, 4,
"writhe free from", TEC_BREAK, 4
);

STYLE("style_dragon", "dragon style", 2, "\
One of the five Shaolin animal styles. Dragon style uses fluid movements and\n\
hard strikes. Intelligence increases your chance to hit as well as the\n\
damage you deal. Moving a tile will boost damage further for one turn.",

"", TEC_BLOCK, 2,
"grab", TEC_GRAB, 4,
"counter-attack", TEC_COUNTER, 4,
"spin-kick", TEC_SWEEP, 5,
"dragon strike", TEC_BRUTAL, 6
);

STYLE("style_centipede", "centipede style", 0, "\
One of the Five Deadly Venoms. Centipede style uses an onslaught of rapid\n\
strikes. Every strike you make reduces the movement cost of attacking by 4;\n\
this is cumulative, but is reset entirely if you are hit even once.",

"swiftly hit", TEC_RAPID, 2,
"block", TEC_BLOCK, 3
);

STYLE("style_venom_snake", "viper style", 2, "\
One of the Five Deadly Venoms. Viper Style has a unique three-hit combo; if\n\
you score a critical hit, it is initiated. The second hit uses a coned hand\n\
to deal piercing damage, and the 3rd uses both hands in a devastating strike.",

"", TEC_RAPID, 3,
"feint at", TEC_FEINT, 3,
"writhe free from", TEC_BREAK, 4
);

STYLE("style_scorpion", "scorpion style", 3, "\
One of the Five Deadly Venoms. Scorpion Style is a mysterious art that focuses\n\
on utilizing pincer-like fists and a stinger-like kick. Critical hits will do\n\
massive damage, knocking your target far back.",

"block", TEC_BLOCK, 3,
"pincer fist", TEC_PRECISE, 4
);

STYLE("style_lizard", "lizard style", 1, "\
One of the Five Deadly Venoms. Lizard Style focuses on using walls to one's\n\
advantage. Moving alongside a wall will make you run up along it, giving you\n\
a large to-hit bonus. Standing by a wall allows you to use it to boost dodge.",

"block", TEC_BLOCK, 2,
"counter-attack", TEC_COUNTER, 4
);

STYLE("style_toad", "toad style", 0, "\
One of the Five Deadly Venoms. Immensely powerful, and immune to nearly any\n\
weapon. You may meditate by pausing for a turn; this will give you temporary\n\
armor, proportional to your Intelligence and Perception.",

"block", TEC_BLOCK, 3,
"grab", TEC_GRAB, 4
);

STYLE("style_zui_quan", "zui quan", 1, "\
Also known as \"drunken boxing,\" Zui Quan imitates the movement of a drunk\n\
to confuse the enemy. The turn after you attack, you may dodge any number of\n\
attacks with no penalty.",

"stumble and leer at", TEC_FEINT, 3,
"counter-attack", TEC_COUNTER, 4
);


// Finally, load up artifacts!
 std::ifstream fin;
 fin.open("save/artifacts.gsav");
 if (!fin.is_open())
  return; // No artifacts yet!

 bool done = fin.eof();
 while (!done) {
  char arttype = ' ';
  fin >> arttype;

  if (arttype == 'T') {
   it_artifact_tool *art = new it_artifact_tool();

   int num_effects, chargetmp, m1tmp, m2tmp, voltmp, wgttmp, bashtmp,
       cuttmp, hittmp, flagstmp, colortmp, pricetmp, maxtmp;
   fin >> pricetmp >> art->sym >> colortmp >> m1tmp >> m2tmp >> voltmp >>
          wgttmp >> bashtmp >> cuttmp >> hittmp >> flagstmp >>
          chargetmp >> maxtmp >> num_effects;
   art->price = pricetmp;
   art->color = int_to_color(colortmp);
   art->m1 = material(m1tmp);
   art->m2 = material(m2tmp);
   art->volume = voltmp;
   art->weight = wgttmp;
   art->melee_dam = bashtmp;
   art->melee_cut = cuttmp;
   art->m_to_hit = hittmp;
   art->charge_type = art_charge(chargetmp);
   art->item_flags = flagstmp;
   art->max_charges = maxtmp;
   for (int i = 0; i < num_effects; i++) {
    int effect;
    fin >> effect;
    art->effects_wielded.push_back( art_effect_passive(effect) );
   }
   fin >> num_effects;
   for (int i = 0; i < num_effects; i++) {
    int effect;
    fin >> effect;
    art->effects_activated.push_back( art_effect_active(effect) );
   }
   fin >> num_effects;
   for (int i = 0; i < num_effects; i++) {
    int effect;
    fin >> effect;
    art->effects_carried.push_back( art_effect_passive(effect) );
   }

   std::string namepart;
   std::stringstream namedata;
   bool start = true;
   do {
    fin >> namepart;
    if (namepart != "-") {
     if (!start)
      namedata << " ";
     else
      start = false;
     namedata << namepart;
    }
   } while (namepart.find("-") == std::string::npos);
   art->name = namedata.str();
   start = true;

   std::stringstream descdata;
   do {
    fin >> namepart;
    if (namepart == "=") {
     descdata << "\n";
     start = true;
    } else if (namepart != "-") {
     if (!start)
      descdata << " ";
     descdata << namepart;
     start = false;
    }
   } while (namepart.find("-") == std::string::npos && !fin.eof());
   art->description = descdata.str();

   itype_id this_id = "artifact"+itypes.size();
   art->id = this_id;
   itypes[this_id]=art;

  } else if (arttype == 'A') {
   it_artifact_armor *art = new it_artifact_armor();

   int num_effects, m1tmp, m2tmp, voltmp, wgttmp, bashtmp, cuttmp,
       hittmp, covertmp, enctmp, dmgrestmp, cutrestmp, envrestmp, warmtmp,
       storagetmp, flagstmp, colortmp, pricetmp;
   fin >> pricetmp >> art->sym >> colortmp >> m1tmp >> m2tmp >> voltmp >>
          wgttmp >> bashtmp >> cuttmp >> hittmp >> flagstmp >>
          covertmp >> enctmp >> dmgrestmp >> cutrestmp >> envrestmp >>
          warmtmp >> storagetmp >> num_effects;
   art->price = pricetmp;
   art->color = int_to_color(colortmp);
   art->m1 = material(m1tmp);
   art->m2 = material(m2tmp);
   art->volume = voltmp;
   art->weight = wgttmp;
   art->melee_dam = bashtmp;
   art->melee_cut = cuttmp;
   art->m_to_hit = hittmp;
   art->covers = covertmp;
   art->encumber = enctmp;
   art->dmg_resist = dmgrestmp;
   art->cut_resist = cutrestmp;
   art->env_resist = envrestmp;
   art->warmth = warmtmp;
   art->storage = storagetmp;
   art->item_flags = flagstmp;
   for (int i = 0; i < num_effects; i++) {
    int effect;
    fin >> effect;
    art->effects_worn.push_back( art_effect_passive(effect) );
   }

   std::string namepart;
   std::stringstream namedata;
   bool start = true;
   do {
    if (!start)
     namedata << " ";
    else
     start = false;
    fin >> namepart;
    if (namepart != "-")
     namedata << namepart;
   } while (namepart.find("-") == std::string::npos);
   art->name = namedata.str();
   start = true;

   std::stringstream descdata;
   do {
    fin >> namepart;
    if (namepart == "=") {
     descdata << "\n";
     start = true;
    } else if (namepart != "-") {
     if (!start)
      descdata << " ";
     descdata << namepart;
     start = false;
    }
   } while (namepart.find("-") == std::string::npos && !fin.eof());
   art->description = descdata.str();

   itype_id this_id = "artifact"+itypes.size();
   art->id = this_id;
   itypes[this_id]=art;
  }

/*
  std::string chomper;
  getline(fin, chomper);
*/
  if (fin.eof())
   done = true;
 } // Done reading the file
 fin.close();


}

std::string ammo_name(ammotype t)
{
 switch (t) {
  case AT_NAIL:   return "nails";
  case AT_BB:	  return "BBs";
  case AT_BOLT:	  return "bolts";
  case AT_ARROW:  return "arrows";
  case AT_SHOT:	  return "shot";
  case AT_22:	  return ".22";
  case AT_9MM:	  return "9mm";
  case AT_762x25: return "7.62x25mm";
  case AT_38:	  return ".38";
  case AT_40:	  return ".40";
  case AT_44:	  return ".44";
  case AT_45:	  return ".45";
  case AT_57:	  return "5.7mm";
  case AT_46:	  return "4.6mm";
  case AT_762:	  return "7.62x39mm";
  case AT_223:	  return ".223";
  case AT_3006:   return ".30-06";
  case AT_308:	  return ".308";
  case AT_40MM:   return "40mm grenade";
  case AT_66MM:   return "High Explosive Anti Tank Warhead";
  case AT_GAS:	  return "gasoline";
  case AT_THREAD: return "thread";
  case AT_BATT:   return "batteries";
  case AT_PLUT:   return "plutonium";
  case AT_MUSCLE: return "Muscle";
  case AT_FUSION: return "fusion cell";
  case AT_12MM:   return "12mm slugs";
  case AT_PLASMA: return "hydrogen";
  case AT_WATER: return "clean water";
  default:	  return "XXX";
 }
}

itype_id default_ammo(ammotype guntype)
{
 switch (guntype) {
 case AT_NAIL:	return "nail";
 case AT_BB:	return "bb";
 case AT_BOLT:	return "bolt_wood";
 case AT_ARROW: return "arrow_wood";
 case AT_SHOT:	return "shot_00";
 case AT_22:	return "22_lr";
 case AT_9MM:	return "9mm";
 case AT_762x25:return "762_25";
 case AT_38:	return "38_special";
 case AT_40:	return "10mm";
 case AT_44:	return "44magnum";
 case AT_45:	return "45_acp";
 case AT_57:	return "57mm";
 case AT_46:	return "46mm";
 case AT_762:	return "762_m43";
 case AT_223:	return "223";
 case AT_308:	return "308";
 case AT_3006:	return "270";
 case AT_40MM:  return "40mm_concussive";
 case AT_66MM:  return "66mm_HEAT";
 case AT_BATT:	return "battery";
 case AT_FUSION:return "laser_pack";
 case AT_12MM:  return "12mm";
 case AT_PLASMA:return "plasma";
 case AT_PLUT:	return "plut_cell";
 case AT_GAS:	return "gasoline";
 case AT_THREAD:return "thread";
 case AT_WATER:return "water_clean";
 }
 return "null";
}

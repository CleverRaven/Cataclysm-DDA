#include "itype.h"
#include "game.h"

// Armor colors
#define C_SHOES  c_blue
#define C_PANTS  c_brown
#define C_BODY   c_yellow
#define C_TORSO  c_ltred
#define C_GLOVES c_ltblue
#define C_MOUTH  c_white
#define C_EYES   c_cyan
#define C_HAT    c_dkgray
#define C_STORE  c_green

// GENERAL GUIDELINES
// When adding a new item, you MUST REMEMBER to insert it in the itype_id enum
//  at the top of itype.h!
//  Additionally, you should check mapitemsdef.cpp and insert the new item in
//  any appropriate lists.
void game::init_itypes ()
{
// First, the null object.  NOT REALLY AN OBJECT AT ALL.  More of a concept.
 itypes.push_back(
  new itype(0, 0, 0, "none", "", '#', c_white, MNULL, MNULL, 0, 0, 0, 0, 0, 0));
 itypes.push_back(
  new itype(1, 0, 0, "corpse", "A dead body.", '%', c_white, MNULL, MNULL, 0, 0,
            0, 0, 0, 0));
 int index = 1;
 
// Drinks
// Stim should be -8 to 8.
// quench MAY be less than zero--salt water and liquor make you thirstier.
// Thirst goes up by 1 every 5 minutes; so a quench of 12 lasts for 1 hour

#define DRINK(name,rarity,price,color,container,quench,nutr,spoils,stim,\
healthy,addict,charges,fun,use_func,addict_func,des) \
	index++;itypes.push_back(new it_comest(index,rarity,price,name,des,'~',\
color,LIQUID,2,1,0,0,0,0,quench,nutr,spoils,stim,healthy,addict,charges,\
fun,container,itm_null,use_func,addict_func));

//     NAME		RAR PRC	COLOR     CONTAINER
DRINK("water",		90, 50,	c_ltcyan, itm_bottle_plastic,
//	QUE NUT SPO STM HTH ADD CHG FUN use_func	addiction type
	50,  0,  0,  0,  0,  0,  1,  0,&iuse::none,	ADD_NULL, "\
Water, the stuff of life, the best thirst-quencher available.");

DRINK("water",		 0, 50,	c_ltcyan, itm_null,// Dirty water, from rivers
	30,  0,  0,  0, -4,  0,  1,  0,&iuse::poison,	ADD_NULL, "\
Water, the stuff of life, the best thirst-quencher available.");

DRINK("salt water",	20,  5,	c_ltcyan, itm_bottle_plastic,
	-30, 0,  0,  0,  1,  0,  1, -1,&iuse::none,	ADD_NULL, "\
Water with salt added.  Not good for drinking.");

DRINK("orange juice",	50, 38,	c_yellow, itm_bottle_plastic,
	35,  4,120,  0,  2,  0,  1,  0,&iuse::none,	ADD_NULL, "\
Fresh squeezed from real oranges!  Tasty and nutritious.");

DRINK("energy drink",	55, 45,	c_magenta,itm_can_drink,
	15,  1,  0,  8, -2,  2,  1,  5,&iuse::caff,	ADD_CAFFEINE, "\
Popular among those who need to stay up late working.");

//     NAME		RAR PRC	COLOR     CONTAINER
DRINK("cola",		70, 35,	c_brown,  itm_can_drink,
//	QUE NUT SPO STM HTH ADD CHG FUN use_func	addiction type
	18,  3,  0,  6, -1,  2,  1,  5,&iuse::caff,	ADD_CAFFEINE, "\
Things go better with cola.  Sugar water with caffeine added.");

DRINK("root beer",	65, 30,	c_brown,  itm_can_drink,
	18,  3,  0,  1, -1,  0,  1,  3,&iuse::none,	ADD_NULL, "\
Like cola, but without caffeine.  Still not that healthy.");

DRINK("milk",		50, 35,	c_white,  itm_bottle_glass,
	25,  8,  8,  0,  1,  0,  1,  0,&iuse::none,	ADD_NULL, "\
Baby cow food, appropriated for adult humans.  Spoils rapidly.");

DRINK("V8",		15, 35,	c_red,    itm_can_drink,
	 6, 28,240,  0,  1,  0,  1,  0,&iuse::none,	ADD_NULL, "\
Contains up to 8 vegetables!  Nutritious and tasty.");

DRINK("whiskey",	16, 85,	c_brown,  itm_bottle_glass,
	-12, 4,  0,-12, -2,  5, 20, 30,&iuse::alcohol,	ADD_ALCOHOL, "\
Made from, by, and for real Southern colonels!");

//     NAME		RAR PRC	COLOR     CONTAINER
DRINK("vodka",		20, 78,	c_ltcyan, itm_bottle_glass,
//	QUE NUT SPO STM HTH ADD CHG FUN use_func	addiction type
	-10, 2,  0,-12, -2,  5, 20, 30,&iuse::alcohol,	ADD_ALCOHOL, "\
In Soviet Russia, vodka drinks you!");

DRINK("rum",		14, 85,	c_ltcyan, itm_bottle_glass,
	-12, 2,  0,-10, -2,  5, 20, 30,&iuse::alcohol,	ADD_ALCOHOL, "\
Drinking this might make you feel like a pirate.  Or not.");

DRINK("tequila",	12, 88,	c_brown,  itm_bottle_glass,
	-12, 2,  0,-12, -2,  6, 20, 35,&iuse::alcohol,	ADD_ALCOHOL, "\
Don't eat the worm!  Wait, there's no worm in this bottle.");

DRINK("beer",           60, 35, c_brown,  itm_bottle_glass,
        16, 4,  0, -4, -1,  2, 1, 20, &iuse::alcohol,   ADD_ALCOHOL, "\
Best served cold, in a glass, and with a lime - but you're not that lucky.");

DRINK("bleach",		20, 18,	c_white,  itm_bottle_plastic,
	-96, 0,  0,  0, -8,  0,  1,-30,&iuse::blech,	ADD_NULL, "\
Don't drink it.  Mixing it with ammonia produces toxic gas.");

//     NAME		RAR PRC	COLOR     CONTAINER
DRINK("ammonia",	24, 30,	c_yellow, itm_bottle_plastic,
//	QUE NUT SPO STM HTH ADD CHG FUN use_func	addiction type
	-96, 0,  0,  0, -2,  0,  1,-30,&iuse::blech,	ADD_NULL, "\
Don't drink it.  Mixing it with bleach produces toxic gas.");

DRINK("mutagen",	 8,8000,c_magenta,itm_bottle_glass,
	  0, 0,  0,  0, -2,  0,  1,  0,&iuse::mutagen,	ADD_NULL, "\
A rare substance of uncertain origins.  Causes you to mutate.");

DRINK("purifier",	12,16000,c_pink,  itm_bottle_glass,
	  0, 0,  0,  0,  1,  0,  1,  0,&iuse::purifier,	ADD_NULL, "\
A rare stem-cell treatment, which causes mutations and other genetic defects\n\
to fade away.");


#define FOOD(name,rarity,price,color,mat1,container,volume,weight,quench,\
nutr,spoils,stim,healthy,addict,charges,fun,use_func,addict_func,des) \
	index++;itypes.push_back(new it_comest(index,rarity,price,name,des,'%',\
color,mat1,volume,weight,0,0,0,0,quench,nutr,spoils,stim,healthy,addict,charges,\
fun,container,itm_null,use_func,addict_func));
// FOOD

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("chunk of meat",	50, 50,	c_red,		FLESH,  itm_null,
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func    addiction type
    1,  2,  0, 20, 24,  0, -1,  0,  1,-10,	&iuse::none, ADD_NULL, "\
Freshly butchered meat.  You could eat it raw, but cooking it is better.");

FOOD("chunk of veggy",	30, 60,	c_green,	VEGGY,	itm_null,
    1,  2,  0, 20, 80,  0,  1,  0,  1,  0,	&iuse::none, ADD_NULL, "\
A raw chunk of vegetable.  Fine for eating raw, tastier when cooked.");

FOOD("tainted meat",	60,  4,	c_red,		FLESH,	itm_null,
    1,  2,  0, 20,  4,  0,  0,  0,  1,-10,	&iuse::poison, ADD_NULL, "\
Meat that's obviously unhealthy.  You could eat it, but it will poison you.");

FOOD("tainted veggy",	35,  5,	c_green,	VEGGY,	itm_null,
    1,  2,  0, 20, 10,  0,  1,  0,  1,  0,	&iuse::poison, ADD_NULL, "\
Vegetable that looks poisonous.  You could eat it, but it will poison you.");

FOOD("cooked meat",	 0, 75, c_red,		FLESH,	itm_null,
    1,  2,  0, 50, 24,  0,  0,  0,  1,  8,	&iuse::none,	ADD_NULL, "\
Freshly cooked meat.  Very nutritious.");

FOOD("cooked veggy",	 0, 70, c_green,	VEGGY,	itm_null,
    1,  2,  0, 40, 50,  0,  1,  0,  1,  0,	&iuse::none,	ADD_NULL, "\
Freshly cooked vegetables.  Very nutritious.");

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("apple",		70, 16,	c_red,		VEGGY,  itm_null,
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func    addiction type
    1,  1,  3, 16,160,  0,  4,  0,  1,  3,	&iuse::none, ADD_NULL, "\
An apple a day keeps the doctor away.");

FOOD("orange",		65, 18,	c_yellow,	VEGGY,	itm_null,
    1,  1,  8, 14, 96,  0,  0,  0,  1,  3,	&iuse::none, ADD_NULL, "\
Sweet citrus fruit.  Also comes in juice form.");

FOOD("lemon",		50, 12, c_yellow,	VEGGY,	itm_null,
    1,  1,  5,  5,180,  0,  0,  0,  1, -4,	&iuse::none, ADD_NULL, "\
Very sour citrus.  Can be eaten if you really want.");

FOOD("potato chips",	65, 12,	c_magenta,	VEGGY,	itm_bag_plastic,
    2,  1, -2,  8,  0,  0,  0,  0,  3,  6,	&iuse::none, ADD_NULL, "\
Betcha can't eat just one.");

FOOD("pretzels",	55, 13,	c_brown,	VEGGY,	itm_bag_plastic,
    2,  1, -2,  9,  0,  0,  0,  0,  3,  2,	&iuse::none, ADD_NULL, "\
A salty treat of a snack.");

FOOD("chocolate bar",	50, 20,	c_brown,	VEGGY,	itm_wrapper,
    1,  1,  0,  8,  0,  1,  0,  0,  1,  8,	&iuse::none, ADD_NULL, "\
Chocolate isn't very healthy, but it does make a delicious treat.");

FOOD("beef jerky",	55, 24,	c_red,		FLESH,  itm_bag_plastic,
    1,  1, -3, 12,  0,  0, -1,  0,  3,  4,	&iuse::none, ADD_NULL, "\
Salty dried meat that never goes bad, but will make you thirsty.");

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("meat sandwich", 30, 60,	c_ltgray,	FLESH,	itm_wrapper,
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func    addiction type
    1,  2,  0, 50, 36,  0,  0,  0,  1,  5,	&iuse::none, ADD_NULL, "\
Bread and turkey, that's it.");

FOOD("candy",		80, 14,	c_magenta,	VEGGY,	itm_bag_plastic,
    0, 0,  -1,  2,  0,  1, -2,  0,  3,  4,	&iuse::none, ADD_NULL, "\
A big bag of peanut butter cups... your favorite!");

FOOD("mushroom",         4, 14,	c_brown,	VEGGY,	itm_null,
    1,  0,  0, 14,  0,  0,  1,  0,  1,  0,	&iuse::none, ADD_NULL, "\
Mushrooms are tasty, but be careful.  Some can poison you, while others are\n\
hallucinogenic.");

FOOD("mushroom",	 3, 14,	c_brown,	VEGGY,	itm_null,
    1,  0,  0, 10,  0,  0,  0,  0,  1,  0,	&iuse::poison, ADD_NULL, "\
Mushrooms are tasty, but be careful.  Some can poison you, while others are\n\
hallucinogenic.");

FOOD("mushroom",	 1, 14,	c_brown,	VEGGY,	itm_null,
    1,  0,  0, 10,  0, -4,  0,  0,  1,  6,	&iuse::hallu, ADD_NULL, "\
Mushrooms are tasty, but be careful.  Some can poison you, while others are\n\
hallucinogenic.");

FOOD("blueberries",	 3, 20,	c_blue,		VEGGY,	itm_null,
    1,  1,  2, 16, 60,  0,  0,  0,  1,  0,	&iuse::none, ADD_NULL, "\
They're blue, but that doesn't mean they're sad.");

FOOD("strawberries",	 2, 10,	c_red,		VEGGY,	itm_null,
    1,  1,  3, 18, 60,  0,  0,  0,  1,  0,	&iuse::none, ADD_NULL, "\
Tasty juicy berry, often found growing wild in fields.");

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("tomato",		 9, 25,	c_red,		VEGGY,  itm_null,
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func    addiction type
    1,  1,  3, 18, 90,  0,  0,  0,  1,  0,	&iuse::none, ADD_NULL, "\
Juicy red tomato.  It gained popularity in Italy after being brought back\n\
from the New World.");

FOOD("broccoli",	 9, 40,	c_green,	VEGGY,	itm_null,
    2,  2,  0, 30,160,  0,  1,  0,  1,  0,	&iuse::none, ADD_NULL, "\
It's a bit tough, but quite delicious.");

FOOD("zucchini",	 7, 30,	c_ltgreen,	VEGGY,	itm_null,
    2,  1,  0, 20,120,  0,  1,  0,  1,  0,	&iuse::none, ADD_NULL, "\
A tasty summer squash.");

FOOD("frozen dinner",	50, 80,	c_magenta,	FLESH,	itm_box_small,
    5,  4, -2, 72, 60,  0, -2,  0,  1,  5,	&iuse::none, ADD_NULL, "\
Now with ONE POUND of meat and ONE POUND of carbs!");

FOOD("raw spaghetti",	40, 12,	c_yellow,	VEGGY,	itm_box_small,
    6,  2,  0,  6,  0,  0,  0,  0,  1, -8,	&iuse::none, ADD_NULL, "\
It could be eaten raw if you're desperate, but is much better cooked.");

FOOD("cooked spaghetti", 0, 28,	c_yellow,	VEGGY,	itm_box_small,
   10,  3,  0, 60, 20,  0,  0,  0,  1,  2,	&iuse::none, ADD_NULL, "\
Fresh wet noodles.  Very tasty.");

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("raw macaroni",	40, 15,	c_yellow,	VEGGY,	itm_box_small,
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func    addiction type
    3,  1,  0,  3,  0,  0,  0,  0,  1, -8,	&iuse::none, ADD_NULL, "\
It could be eaten raw if you're desperate, but is much better cooked.");

FOOD("mac & cheese",	 0, 38,	c_yellow,	VEGGY,	itm_box_small,
    4,  2,  0, 60, 10,  0, -1,  0,  1,  3,	&iuse::none, ADD_NULL, "\
When the cheese starts flowing, Kraft gets your noodle going.");

FOOD("ravioli",		25, 75,	c_cyan,		FLESH,	itm_can_food,
    1,  2,  0, 48,  0,  0,  0,  0,  1,  0,	&iuse::none, ADD_NULL, "\
Meat encased in little dough satchels.  Tastes fine raw.");

FOOD("red sauce",	20, 24,	c_red,		VEGGY,	itm_can_food,
    2,  3,  0, 20,  0,  0,  0,  0,  1,  1,	&iuse::none, ADD_NULL, "\
Tomato sauce, yum yum.");

FOOD("pesto",		15, 20,	c_ltgreen,	VEGGY,	itm_can_food,
    2,  3,  0, 18,  0,  0,  1,  0,  1,  4,	&iuse::none, ADD_NULL, "\
Olive oil, basil, garlic, pine nuts.  Simple and deliicous.");

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("beans",		40, 55,	c_cyan,		VEGGY,	itm_can_food,
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func    addiction type
    1,  2,  0, 40,  0,  0,  0,  0,  1,  0,	&iuse::none, ADD_NULL, "\
Canned beans.  A staple for hobos.");

FOOD("corn",		35, 40,	c_cyan,		VEGGY,	itm_can_food,
    1,  2,  5, 30,  0,  0,  0,  0,  1, -2,	&iuse::none, ADD_NULL, "\
Canned corn in water.  Eat up!");

FOOD("SPAM",		30, 50,	c_cyan,		FLESH,	itm_can_food,
    1,  2, -3, 48,  0,  0,  0,  0,  1, -8,	&iuse::none, ADD_NULL, "\
Yuck, not very tasty.  But it is quite filling.");

FOOD("pineapple",	30, 50,	c_cyan,		VEGGY,	itm_can_food,
    1,  2,  5, 26,  0,  0,  1,  0,  1,  7,	&iuse::none, ADD_NULL, "\
Canned pinapple rings in water.  Quite tasty.");

FOOD("coconut milk",	10, 45,	c_cyan,		VEGGY,	itm_can_food,
    1,  2,  5, 30,  0,  0,  0,  0,  1,  0,	&iuse::none, ADD_NULL, "\
A dense, sweet creamy sauce, often used in curries.");

FOOD("sardines",	14, 25,	c_cyan,		FLESH,	itm_can_food,
    1,  1, -8, 14,  0,  0,  0,  0,  1,  0,	&iuse::none, ADD_NULL, "\
Salty little fish.  They'll make you thirsty.");

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("tuna fish",	35, 35,	c_cyan,		FLESH,	itm_can_food,
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func    addiction type
    1,  2,  0, 24,  0,  0,  0,  0,  1,  0,	&iuse::none, ADD_NULL, "\
Now with 95 percent less dolphins!");

FOOD("cat food",	20,  8,	c_cyan,		FLESH,	itm_can_food,
    1,  2,  0, 10,  0,  0, -1,  0,  1,-24,	&iuse::none, ADD_NULL, "\
Blech, so gross.  Save it for when you're about to die of starvation.");

FOOD("honey comb",	10, 35,	c_yellow,	VEGGY,	itm_null,
    1,  1,  0, 20,  0,  0, -2,  0,  1,  9,	&iuse::none, ADD_NULL, "\
A large chunk of wax, filled with honey.  Very tasty.");

FOOD("royal jelly",	 8,200,	c_magenta,	VEGGY,	itm_null,
    1,  1,  0, 10,  0,  0,  3,  0,  1,  7,	&iuse::royal_jelly, ADD_NULL, "\
A large chunk of wax, filled with dense, dark honey.  Useful for curing all\n\
sorts of afflictions.");

FOOD("misshapen fetus",	 1,150,	c_magenta,	FLESH,	itm_null,
    4,  4,  0,  8,  0,  0, -8,  0,  1,-60,	&iuse::mutagen, ADD_NULL, "\
Eating this is about the most disgusting thing you can imagine, and it will\n\
cause your DNA to mutate as well.");

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("arm",		 4,250,	c_brown,	FLESH,	itm_null,
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func       addiction type
    8, 14,  0, 12,  0,  0, -8,  0,  1, -20,	&iuse::mutagen, ADD_NULL, "\
Eating this would be pretty gross.  It causes you to mutate.");

FOOD("leg",		 4,250,	c_brown,	FLESH,	itm_null,
   12, 24,  0, 16,  0,  0, -8,  0,  1, -20,	&iuse::mutagen, ADD_NULL, "\
Eating this would be pretty gross.  It causes you to mutate.");

FOOD("ant egg",		 5, 80,	c_white,	FLESH,	itm_null,
    4,  2, 10, 100, 0,  0, -1,  0,  1, -10,	&iuse::none,	ADD_NULL, "\
A large ant egg, the size of a softball.  Extremely nutrtious, but gross.");

FOOD("marloss berry",	 2,600, c_pink,		VEGGY,	itm_null,
    1,  1, 20, 40,  0,  0,-10,  0,  1, 30,	&iuse::marloss,	ADD_NULL, "\
This looks like a blueberry the size of your fist, but pinkish in color.  It\n\
has a strong but delicious aroma, but is clearly either mutated or of alien\n\
origin.");

// MEDS
#define MED(name,rarity,price,color,tool,mat,stim,healthy,addict,\
charges,fun,use_func,addict_func,des) \
	index++;itypes.push_back(new it_comest(index,rarity,price,name,des,'!',\
color,mat,1,1,0,0,0,0,0,0,0,stim,healthy,addict,charges,\
fun,itm_null,tool,use_func,addict_func));

//  NAME		RAR PRC	COLOR		TOOL
MED("bandages",		50, 60,	c_white,	itm_null,
//	MATERIAL STM HTH ADD CHG FUN use_func		addiction type
	COTTON,   0,  0,  0,  3,  0,&iuse::bandage,	ADD_NULL, "\
Simple cloth bandages.  Used for healing small amounts of damage.");

MED("first aid",	35,350,	c_red,		itm_null,
	PLASTIC,  0,  0,  0,  2,  0,&iuse::firstaid,	ADD_NULL, "\
A full medical kit, with bandages, anti-biotics, and rapid healing agents.\n\
Used for healing large amounts of damage.");

MED("vitamins",		75, 45,	c_cyan,		itm_null,
	PLASTIC,  0,  3,  0, 50,  0,&iuse::none,	ADD_NULL, "\
Take frequently to improve your immune system.");

MED("aspirin",		85, 30,	c_cyan,		itm_null,
	PLASTIC,  0, -1,  0, 50,  0,&iuse::pkill_1,	ADD_NULL, "\
Low-grade painkiller.  Best taken in pairs.");

MED("caffeine pills",	25, 60,	c_cyan,		itm_null,
	PLASTIC, 15,  0,  3, 20,  0,&iuse::caff,	ADD_CAFFEINE, "\
No-doz pills.  Useful for staying up all night.");

//  NAME		RAR PRC	COLOR		TOOL
MED("sleeping pills",	15, 50,	c_cyan,		itm_null,
//	MATERIAL STM HTH ADD CHG FUN use_func		addiction type
	PLASTIC, -8,  0, 40, 20,  0,&iuse::sleep,	ADD_SLEEP, "\
Prescription sleep aids.  Will make you very tired.");

MED("iodine tablets",	 5,140, c_yellow,	itm_null,
	PLASTIC,  0, -1,  0, 10,  0,&iuse::iodine,	ADD_NULL, "\
Iodine tablets are used for recovering from irradiation.  They are not\n\
spectacularly effective, but are better than nothing.");

MED("Dayquil",		70, 75,	c_yellow,	itm_null,
	PLASTIC,  0,  1,  0, 10,  0,&iuse::flumed,	ADD_NULL, "\
Daytime flu medication.  Will halt all flu symptoms for a while.");

MED("Nyquil",		70, 85,	c_blue,		itm_null,
	PLASTIC, -7,  1,  0, 10,  0,&iuse::flusleep,	ADD_NULL, "\
Nighttime flu medication.  Will halt all flu symptoms for a while, plus make\n\
you sleepy.");

MED("inhaler",		14,200,	c_ltblue,	itm_null,
	PLASTIC,  1,  0,  0,100,  0,&iuse::inhaler,	ADD_NULL, "\
Vital medicine for those with asthma.  Those without asthma can use it for a\n\
minor stimulant boost.");

//  NAME		RAR PRC	COLOR		TOOL
MED("codeine",		15,400,	c_cyan,		itm_null,
//	MATERIAL STM HTH ADD CHG FUN use_func		addiction type
	PLASTIC, -2,  0, 10, 20, 10,&iuse::pkill_2,	ADD_PKILLER, "\
A weak opiate, prescribed for light to moderate pain.");

MED("oxycodone",	 7,900,	c_cyan,		itm_null,
	PLASTIC, -4, -1, 16, 20, 18,&iuse::pkill_3,	ADD_PKILLER, "\
A strong opiate, prescribed for moderate to intense pain.");

MED("tramadol",		11,300,	c_cyan,		itm_null,
	PLASTIC,  0,  0,  6, 25,  6,&iuse::pkill_l,	ADD_PKILLER, "\
A long-lasting opiate, prescribed for moderate pain.  Its painkiller effects\n\
last for several hours, but are not as strong as oxycodone.");

MED("Xanax",		10,600,	c_cyan,		itm_null,
	PLASTIC, -4,  0,  0, 20, 20,&iuse::xanax,	ADD_NULL, "\
Anti-anxiety medication.  It will reduce your stimulant level steadily, and\n\
will temporarily cancel the effects of anxiety, like the Hoarder trait.");

//  NAME		RAR PRC	COLOR		TOOL
MED("Adderall",		10,750,	c_cyan,		itm_null,
//	MATERIAL STM HTH ADD CHG FUN use_func		addiction type
	PLASTIC, 24,  0, 10, 40, 10,&iuse::none,	ADD_SPEED, "\
A strong stimulant prescribed for ADD.  It will greatly increase your\n\
stimulant level, but is quite addictive.");

MED("Thorazine",	 7,500,	c_cyan,		itm_null,
	PLASTIC,  0,  0,  0, 15,  0,&iuse::thorazine,	ADD_NULL, "\
Anti-psychotic medication.  Used to control the symptoms of schizophrenia and\n\
similar ailments.  Also popular as a way to come down for a bad trip.");

MED("Prozac",		10,650,	c_cyan,		itm_null,
	PLASTIC, -4,  0,  0, 40,  0,&iuse::prozac,	ADD_NULL, "\
A strong anti-depressant.  Useful if your morale level is very low.");

MED("cigarettes",	90,120,	c_dkgray,	itm_lighter,
	VEGGY,    1, -1, 40, 20,  5,&iuse::cig,		ADD_CIG, "\
These will boost your dexterity, intelligence, and perception for a short\n\
time.  They are quite addictive.");

//  NAME		RAR PRC	COLOR
MED("marijuana",	20,180,	c_green,	itm_lighter,
//	MATERIAL STM HTH ADD CHG FUN use_func		addiction type
	VEGGY,   -8,  0,  0, 15, 18,&iuse::weed,	ADD_NULL, "\
Really useful only for relaxing.  Will reduce your attributes and reflexes.");

MED("cocaine",		 8,420,	c_white,	itm_null,
	POWDER,  20, -2, 30,  8, 25,&iuse::coke,	ADD_COKE, "\
A strong, illegal stimulant.  Highly addictive.");

MED("methamphetamine",	 2,400, c_ltcyan,	itm_null,
	POWDER,  10, -4, 50,  6, 30,&iuse::meth,	ADD_SPEED, "\
A very strong illegal stimulant.  Extremely addictive and bad for you, but\n\
also extremely effective in boosting your alertness.");

//  NAME		RAR PRC	COLOR
MED("heroin",		 1,600,	c_brown,	itm_syringe,
//	MATERIAL STM HTH ADD CHG FUN use_func		addiction type
	POWDER, -10, -3, 60,  4, 45,&iuse::pkill_4,	ADD_PKILLER, "\
A very strong illegal opiate.  Unless you have an opiate tolerance, avoid\n\
heroin, as it will be too strong for you.");

// MELEE WEAPONS
// Only use secondary material if it will have a major impact.
// dam is a somewhat rigid bonus--anything above 30, tops, is ridiculous
// cut is even MORE rigid, and should be kept lower still
// to_hit (affects chances of hitting) should be kept small, -5 to +5
// Note that do-nothing objects (e.g. superglue) belong here too!
#define MELEE(name,rarity,price,sym,color,mat1,mat2,volume,wgt,dam,cut,to_hit,\
              flags, des)\
	index++;itypes.push_back(new itype(index,rarity,price,name,des,sym,\
color,mat1,mat2,volume,wgt,dam,cut,to_hit,flags))

//    NAME		RAR PRC SYM  COLOR	MAT1	MAT2
MELEE("paper wrapper",	50,  1, ',', c_ltgray,	PAPER,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  0, -8,  0, -2, 0, "\
Just a piece of butcher's paper.  Good for starting fires.");

MELEE("syringe",	 8, 25, ',', c_ltcyan,	PLASTIC,MNULL,
	 1,  0, -4,  6, -2, mfb(WF_SPEAR), "\
A medical syringe.  Used for administering heroin and other drugs.");

MELEE("rag",		72, 10, ';', c_dkgray,	COTTON,	MNULL,
	 2,  1,-10,  0,  0, 0, "\
A small piece of cloth.  Useful for making molotov cocktails and not much else."
);

MELEE("fur pelt",	 0, 10, ',', c_brown,	WOOL,	FLESH,
	 1,  1, -8,  0,  0, 0, "\
A small bolt of fur from an animal.  Can be made into warm clothing.");

MELEE("leather pelt",	 0, 20, ',', c_red,	LEATHER,FLESH,
	 1,  1, -2,  0, -1, 0, "\
A small piece of thick animal hide.  Can be made into tough clothing.");

//    NAME		RAR PRC SYM  COLOR	MAT1	MAT2
MELEE("superglue",	30, 18, ',', c_white,	PLASTIC,MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  0, -2,  0, -2, 0, "\
A tube of strong glue.  Used in many crafting recipes.");

MELEE("ID card",	 2,600, ',', c_pink,	PLASTIC,MNULL,
	 0,  0, -8,  1, -3, 0, "\
This ID card once belonged to a scientist of some sort.  It has a magnetic\n\
stripe on the back; perhaps it can be used on a control panel.");

MELEE("electrohack",	 3,400, ',', c_green,	PLASTIC,STEEL,
	 2,  2,  5,  0,  1, 0, "\
This device has many ports attached, allowing to to connect to almost any\n\
control panel or other electronic machine (but not computers).  With a little\n\
skill, it can be used to crack passwords and more.");

MELEE("string - 6 in",	 2,  5, ',', c_ltgray,	COTTON,	MNULL,
	 0,  0,-20,  0,  1, 0, "\
A small piece of cotton string.");

MELEE("string - 3 ft",	40, 30, ',', c_ltgray,	COTTON,	MNULL,
	 1,  0, -5,  0,  1, 0, "\
A long piece of cotton string.  Use scissors on it to cut it into smaller\n\
pieces.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("rope - 6 ft",	 4, 45, ',', c_yellow,	WOOD,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 2,  4,  1,  0,  1, mfb(WF_WRAP), "\
A short piece of nylon rope.  Too small to be of much use.");

MELEE("rope - 30 ft",	35,100, ',', c_yellow,	WOOD,	MNULL,
	10, 20,  1,  0, -10, 0, "\
A long nylon rope.  Useful for keeping yourself safe from falls.");

MELEE("steel chain",	20, 80, '/', c_cyan,	STEEL,	MNULL,
	 4,  8, 12,  0,  3, mfb(WF_WRAP), "\
A heavy steel chain.  Useful as a weapon, or for crafting.");

MELEE("processor board",15,120, ',', c_ltcyan,	IRON,	PLASTIC,
	 1,  0, -3,  0, -1, 0, "\
A central processor unit, useful in advanced electronics crafting.");

MELEE("RAM",		22, 90, ',', c_ltcyan,	IRON,	PLASTIC,
	 1,  0, -5,  0, -1, 0, "\
A stick of memory.  Useful in advanced electronics crafting.");

MELEE("power converter",16,170, ',', c_ltcyan,	IRON,	PLASTIC,
	 4,  2,  5,  0, -1, 0, "\
A power supply unit.  Useful in lots of electronics recipes.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("amplifier circuit",8,200,',', c_ltcyan,	IRON,	PLASTIC,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  0, -5,  0, -1, 0, "\
A circuit designed to amplify the strength of a signal.  Useful in lots of\n\
electronics recipes.");

MELEE("transponder circuit",5,240,',',c_ltcyan,	IRON,	PLASTIC,
	 1,  0, -5,  0, -1, 0, "\
A circuit designed to repeat a signal.  Useful for crafting communications\n\
equipment.");

MELEE("signal receiver",10,135, ',', c_ltcyan,	IRON,	PLASTIC,
	 1,  0, -4,  0, -1, 0, "\
A module designed to receive many forms of signals.  Useful for crafting\n\
communications equipment.");

MELEE("antenna",	18, 80, ',', c_ltcyan,	STEEL,	MNULL,
	 1,  0, -6,  0,  2, 0, "\
A simple thin aluminum shaft.  Useful in lots of electronics recipes.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("chunk of steel", 30, 10, ',', c_ltblue,	STEEL,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 2,  6, 12,  0, -2, 0, "\
A misshapen chunk of steel.  Makes a decent weapon in a pinch, and is also\n\
useful for some crafting recipes.");

MELEE("electric motor",  2,120, ',', c_ltcyan,	IRON,	MNULL,
	 4,  6,  4,  0,  0, 0, "\
A powerful electric motor.  Useful for crafting.");

MELEE("rubber hose",	15, 80, ',', c_green,	PLASTIC,MNULL,
	 3,  2,  4,  0,  3, mfb(WF_WRAP), "\
A flexible rubber hose.  Useful for crafting.");

MELEE("sheet of glass",	 5,135, ']', c_ltcyan,	GLASS,	MNULL,
	50, 20, 16,  0, -5, 0, "\
A large sheet of glass.  Easily shattered.  Useful for re-paning windows.");

MELEE("manhole cover",	 0, 20, ']', c_dkgray,	IRON,	MNULL,
	45,250, 20,  0,-10, 0, "\
A heavy iron disc which generally covers a ladder into the sewers.  Lifting\n\
it from the manhole is impossible without a crowbar.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("rock",		40,  0, '*', c_ltgray,	STONE,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  3, 14,  0, -1, 0, "\
A rock the size of a baseball.  Makes a decent melee weapon, and is also good\n\
for throwing at enemies.");

MELEE("heavy stick",	95,  0, '/', c_brown,	WOOD,	MNULL,
	 6, 10, 12,  0,  3, 0, "\
A sturdy, heavy stick.  Makes a decent melee weapon, and can be cut into two\n\
by fours for crafting.");

MELEE("broom",		30, 24, '/', c_blue,	PLASTIC,MNULL,
	10,  8,  6,  0,  1, 0, "\
A long-handled broom.  Makes a terrible weapon unless you're chasing cats.");

MELEE("mop",		20, 28, '/', c_ltblue,	PLASTIC,MNULL,
	11, 12,  5,  0, -2, 0, "\
An unwieldy mop.  Essentially useless.");

MELEE("screwdriver",	40, 65, ';', c_ltcyan,	IRON,	PLASTIC,
	 1,  1,  2,  8,  1, mfb(WF_SPEAR), "\
A Philips-head screwdriver, important for almost all electronics crafting and\n\
most mechanics crafting.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("wrench",		30, 86, ';', c_ltgray,	IRON,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 2,  5, 15,  0,  2, 0, "\
An adjustable wrench.  Makes a decent melee weapon, and is used in many\n\
mechanics crafting recipes.");

MELEE("wood saw",	15, 40, ';', c_cyan,	IRON,	WOOD,
	 7,  3, -6,  1, -2, 0, "\
A flimsy saw, useful for cutting through wood objects.");

MELEE("hack saw",	17, 65, ';', c_ltcyan,	IRON,	MNULL,
	 4,  2,  1,  1, -1, 0, "\
A sturdy saw, useful for cutting through metal objects.");

MELEE("sledge hammer",	 6, 120,'/', c_brown,	WOOD,	IRON,
	18, 34, 40,  0,  0, 0, "\
A large, heavy hammer.  Makes a good melee weapon for the very strong, but is\n\
nearly useless in the hands of the weak.");

MELEE("hatchet",	10,  95,';', c_ltgray,	IRON,	WOOD,
	 6,  7, 12, 12,  1, 0, "\
A one-handed hatchet.  Makes a great melee weapon, and is useful both for\n\
cutting wood, and for use as a hammer.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("wood ax",	 8, 105,'/', c_ltgray,	WOOD,	IRON,
//	VOL WGT DAM CUT HIT FLAGS
	17, 15, 24, 18,  1, 0, "\
A large two-handed axe.  Makes a good melee weapon, but is a bit slow.");

MELEE("nail board",	 5,  80,'/', c_ltred,	WOOD,	MNULL,
	 6,  6, 16,  6,  1, mfb(WF_STAB), "\
A long piece of wood with several nails through one end; essentiall a simple\n\
mace.  Makes a great melee weapon.");

MELEE("X-Acto knife",	10,  40,';', c_dkgray,	IRON,	PLASTIC,
	 1,  2,  0, 14, -4, mfb(WF_SPEAR), "\
A small, very sharp knife.  Causes decent damage but is difficult to hit with."
);

MELEE("pot",		25,  45,')', c_ltgray,	IRON,	MNULL,
	 8,  6,  9,  0,  1, 0, "\
Useful for boiling water when cooking spaghetti and more.");

MELEE("frying pan",	25,  50,')', c_ltgray,	IRON,	MNULL,
	 6,  6, 14,  0,  2, 0, "\
A cast-iron pan.  Makes a decent melee weapon, and is used for cooking.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("butter knife",	90,  15,';', c_ltcyan,	STEEL, 	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  2,  2,  1, -2, 0, "\
A dull knife, absolutely worthless in combat.");

MELEE("steak knife",	85,  25,';', c_ltcyan,	STEEL,	MNULL,
	 1,  2,  2, 10, -3, mfb(WF_STAB), "\
A sharp knife.  Makes a poor melee weapon, but is decent at butchering\n\
corpses.");

MELEE("butcher knife",	10,  80,';', c_cyan,	STEEL,	MNULL,
	 3,  6,  4, 20, -3, 0, "\
A sharp, heavy knife.  Makes a good melee weapon, and is the best item for\n\
butchering corpses.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("combat knife",	14, 100,';', c_blue,	STEEL,  PLASTIC,
//	VOL WGT DAM CUT HIT FLAGS
	 2,  2,  2, 22, -3, mfb(WF_STAB), "\
Designed for combat, and deadly in the right hands.  Can be used to butcher\n\
corpses.");

MELEE("two by four", 	60,  80,'/', c_ltred,	WOOD,	MNULL,
	 6,  6, 14,  0,  1, 0, "\
A plank of wood.  Makes a decent melee weapon, and can be used to board up\n\
doors and windows if you have a hammer and nails.");

MELEE("muffler",	30,  30,'/', c_ltgray,	IRON,	MNULL,
	20, 20, 19,  0, -3, 0, "\
A muffler from a car.  Very unwieldy as a weapon.  Useful in a few crafting\n\
recipes.");

MELEE("pipe",		20,  75,'/', c_dkgray,	STEEL,	MNULL,
	 4, 10, 13,  0,  3, 0, "\
A steel pipe, makes a good melee weapon.  Useful in a few crafting recipes.");

MELEE("baseball bat",	60, 160,'/', c_ltred,	WOOD,	MNULL,
	12, 10, 28,  0,  3, 0, "\
A sturdy wood bat.  Makes a great melee weapon.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("machete",	 5, 280,'/', c_blue,	IRON,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 8, 14,  6, 28,  2, 0, "\
This huge iron knife makes an excellent melee weapon.");

MELEE("katana",		 2, 980,'/', c_ltblue,	STEEL,	MNULL,
	16, 16, 18, 45,  1, mfb(WF_STAB), "\
A rare sword from Japan.  Deadly against unarmored targets, and still very\n\
effective against the armored.");

MELEE("wood spear",	 5,  40,'/', c_ltred,	WOOD,	MNULL,
	 5,  3,  4, 18,  1, mfb(WF_SPEAR), "\
A simple wood pole with one end sharpened.");

MELEE("steel spear",      5,  140,'/', c_ltred,   WOOD,   STEEL,
         6,  6,  2, 28,  1, mfb(WF_SPEAR), "\
A simple wood pole made deadlier by the knife tied to it.");

MELEE("expandable baton",8, 175,'/', c_blue,	STEEL,	MNULL,
	 1,  4, 12,  0,  2, 0, "\
A telescoping baton that collapses for easy storage.  Makes an excellent\n\
melee weapon.");

MELEE("bee sting",	 5,  70,',', c_dkgray,	FLESH,	MNULL,
	 1,  0,  7, 20,  1, mfb(WF_SPEAR), "\
A four-inch stinger from a giant bee.  Makes a good melee weapon.");

MELEE("wasp sting",	 5,  70,',', c_dkgray,	FLESH,	MNULL,
	 1,  0,  7, 24,  1, mfb(WF_SPEAR), "\
A four-inch stinger from a giant wasp.  Makes a good melee weapon.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("chunk of chitin",10,  15,',', c_red,	FLESH,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  0,  1,  0, -2, 0, "\
A piece of an insect's exoskeleton.  It is light and very durable.");

MELEE("empty canister",  5,  20,'*', c_ltgray,	STEEL,	MNULL,
	 1,  1,  2,  0, -1, 0, "\
An empty cansiter, which may have once held tear gas or other substances.");

MELEE("gold bar",	10,3000,'/', c_yellow,	STEEL,	MNULL,
	 2, 60, 14,  0, -1, 0, "\
A large bar of gold.  Before the apocalypse, this wouldn't been worth a small\n\
fortune; now its value is greatly diminished.");

// ARMOR
#define ARMOR(name,rarity,price,color,mat1,mat2,volume,wgt,dam,to_hit,\
encumber,dmg_resist,cut_resist,env,warmth,storage,covers,des)\
	index++;itypes.push_back(new it_armor(index,rarity,price,name,des,'[',\
color,mat1,mat2,volume,wgt,dam,0,to_hit,0,covers,encumber,dmg_resist,cut_resist,\
env,warmth,storage))

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("sneakers",	80, 100,C_SHOES,	LEATHER,	MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    5,  4, -2,  0, -1,  0,  2,  0,  2,  0,	mfb(bp_feet), "\
Guaranteed to make you run faster and jump higher!");

ARMOR("boots",		70, 120,C_SHOES,	LEATHER,	MNULL,
    7,  6,  1, -1,  1,  1,  4,  2,  4,  0,	mfb(bp_feet), "\
Tough leather boots, very durable.");

ARMOR("steeltoed boots",50, 135,C_SHOES,	LEATHER,	STEEL,
    7,  9,  4, -1,  1,  4,  4,  3,  2,  0,	mfb(bp_feet), "\
Leather boots with a steel toe.  Extremely durable.");

ARMOR("winter boots",	60, 140,C_SHOES,	PLASTIC,	WOOL,
    8,  7,  0, -1,  2,  0,  2,  1,  7,  0,	mfb(bp_feet), "\
Cumbersome boots designed for warmth.");

ARMOR("mocassins",	 5,  80,C_SHOES,	LEATHER,	WOOL,
    2,  1, -3,  0,  0,  0,  1,  0,  3,  0,	mfb(bp_feet), "\
Simple shoes made from animal pelts.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("flip-flops",	35,  25,C_SHOES,	PLASTIC,	MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    1,  1, -4, -2,  3,  0,  0,  0,  0,  0,	mfb(bp_feet), "\
Simple sandals.  Very difficult to run in.");

ARMOR("dress shoes",	50,  45,C_SHOES,	LEATHER,	MNULL,
    5,  3,  1,  1,  1,  0,  3,  0,  1,  0,	mfb(bp_feet), "\
Fancy patent leather shoes.  Not designed for running in.");

ARMOR("heels",		50,  50,C_SHOES,	LEATHER,	MNULL,
    4,  2,  6, -2,  4,  0,  0,  0,  0,  0,	mfb(bp_feet), "\
A pair of high heels.  Difficult to even walk in.");


ARMOR("jeans",		90, 180,C_PANTS,	COTTON,		MNULL,
    5,  4, -4,  1,  0,  0,  1,  0,  1,  4,	mfb(bp_legs), "\
A pair of blue jeans with two deep pockets.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("pants",		75, 185,C_PANTS,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    5,  5, -4,  1,  0,  0,  1,  0,  2,  4,	mfb(bp_legs), "\
A pair of khaki pants.  Slightly warmer than jeans.");

ARMOR("leather pants",	60, 210,C_PANTS,	LEATHER,	MNULL,
    6,  8, -2,  1,  1,  1,  7,  0,  5,  2,	mfb(bp_legs), "\
A pair of black leather pants.  Very tough, but encumbersome and without much\n\
storage.");

ARMOR("cargo pants",	70, 280,C_PANTS,	COTTON,		MNULL,
    6,  6, -3,  0,  1,  0,  2,  0,  3, 12,	mfb(bp_legs), "\
A pair of pants lined with pockets, offering lots of storage.");

ARMOR("army pants",	30, 315,C_PANTS,	COTTON,		MNULL,
    6,  7, -2,  0,  1,  0,  3,  0,  4, 14,	mfb(bp_legs), "\
A tough pair of pants lined with pockets.  Favored by the military.");

ARMOR("skirt",		75, 120,C_PANTS,	COTTON,		MNULL,
    2,  2, -5,  0, -1,  0,  0,  0,  0,  1,	mfb(bp_legs), "\
A short, breezy cotton skirt.  Easy to move in, but only has a single small\n\
pocket.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("jumpsuit",	20, 200,C_BODY,		COTTON,		PLASTIC,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    6,  6, -3, -3,  1,  0,  1,  0,  3, 16,	mfb(bp_legs)|mfb(bp_torso), "\
A full-body jumpsuit with many pockets.");

ARMOR("dress",		70, 180,C_BODY,		COTTON,		MNULL,
    8,  6, -5, -5,  3,  0,  1,  0,  2,  0,	mfb(bp_legs)|mfb(bp_torso), "\
A long cotton dress.  Difficult to move in and lacks any storage space.");

ARMOR("chitinous armor", 1,1200,C_BODY,		FLESH,		MNULL,
   90, 10,  2, -5,  2,  8, 14,  0,  1,  0,	mfb(bp_legs)|mfb(bp_torso), "\
Leg and body armor made from the exoskeletons of insects.  Light and durable.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("t shirt",	80,  80,C_TORSO,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    3,  2, -5,  0,  0,  0,  0,  0,  1,  0,	mfb(bp_torso), "\
A short-sleeved cotton shirt.");

ARMOR("polo shirt",	65,  95,C_TORSO,	COTTON,		MNULL,
    3,  2, -5,  0,  0,  0,  1,  0,  1,  0,	mfb(bp_torso), "\
A short-sleeved cotton shirt, slightly thicker than a t-shirt.");

ARMOR("dress shirt",	60, 115,C_TORSO,	COTTON,		MNULL,
    3,  3, -5,  0,  1,  0,  1,  0,  1,  1,	mfb(bp_torso), "\
A white button-down shirt with long sleeves.  Looks professional!");

ARMOR("tank top",	50,  75,C_TORSO,	COTTON,		MNULL,
    1,  1, -5,  0,  0,  0,  0,  0,  0,  0,	mfb(bp_torso), "\
A sleeveless cotton shirt.  Very easy to move in.");

ARMOR("sweatshirt",	75, 110,C_TORSO,	COTTON,		MNULL,
    9,  5, -5,  0,  1,  1,  1,  0,  3,  0,	mfb(bp_torso), "\
A thick cotton shirt.  Provides warmth and a bit of padding.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("sweater",	75, 105,C_TORSO,	WOOL,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    8,  5, -5,  0,  0,  1,  0,  0,  3,  0,	mfb(bp_torso), "\
A wool shirt.  Provides warmth.");

ARMOR("hoodie",		65, 130,C_TORSO,	COTTON,		MNULL,
   10,  5, -5,  0,  1,  1,  2,  0,  3,  9,	mfb(bp_torso), "\
A sweatshirt with a hood and a \"kangaroo pocket\" in front for storage.");

ARMOR("light jacket",	50, 105,C_TORSO,	COTTON,		MNULL,
    6,  4, -5,  0,  0,  0,  2,  0,  2,  4,	mfb(bp_torso), "\
A thin cotton jacket.  Good for brisk weather.");

ARMOR("jean jacket",	35, 120,C_TORSO,	COTTON,		MNULL,
    7,  5, -3,  0,  1,  0,  4,  0,  2,  3,	mfb(bp_torso), "\
A jacket made from denim.  Provides decent protection from cuts.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("blazer",		35, 120,C_TORSO,	WOOL,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   10,  6, -4,  0,  2,  0,  3,  0,  3,  2,	mfb(bp_torso), "\
A professional-looking wool blazer.  Quite encumbersome.");

ARMOR("leather jacket",	30, 150,C_TORSO,	LEATHER,	MNULL,
   14, 14, -2,  1,  2,  1,  9,  1,  4,  4,	mfb(bp_torso), "\
A jacket made from thick leather.  Encumbersome, but offers excellent\n\
protection from cuts.");

ARMOR("kevlar vest",	30, 800,C_TORSO,	KEVLAR,		COTTON,
   24, 24,  6, -3,  2,  4, 22,  0,  4,  4,	mfb(bp_torso), "\
A heavy bulletproof vest.  The best protection from cuts and bullets.");

ARMOR("rain coat",	50, 100,C_TORSO,	PLASTIC,	COTTON,
    9,  8, -4,  0,  2,  0,  3,  1,  2,  7,	mfb(bp_torso), "\
A plastic coat with two very large pockets.  Provides protection from rain.");

ARMOR("wool poncho",	15, 120,C_TORSO,	WOOL,		MNULL,
    7,  3, -5, -1,  0,  1,  2,  1,  2,  0,	mfb(bp_torso), "\
A simple wool garment worn over the torso.  Provides a bit of protection.");

//     NAME		RARE	COLOR		MAT1		MAT2
ARMOR("trenchcoat",	25, 225,C_TORSO,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   10,  6, -5, -1,  1,  0,  1,  1,  3, 24,	mfb(bp_torso), "\
A long coat lines with pockets.  Great for storage.");

ARMOR("winter coat",	50, 160,C_TORSO,	COTTON,		MNULL,
   12,  6, -5, -2,  3,  3,  1,  1,  8, 12,	mfb(bp_torso), "\
A padded coat with deep pockets.  Very warm.");

ARMOR("fur coat",	 5, 550,C_TORSO,	WOOL,		FLESH,
   18, 12, -5, -5,  2,  4,  2,  2, 10,  4,	mfb(bp_torso), "\
A fur coat with a couple small pockets.  Extremely warm.");

ARMOR("peacoat",	30, 180,C_TORSO,	COTTON,		MNULL,
   16, 10, -4, -3,  2,  1,  2,  0,  7, 10,	mfb(bp_torso), "\
A heavy cotton coat.  Encumbersome, but warm and with deep pockets.");

ARMOR("utility vest",	15, 200,C_TORSO,	COTTON,		MNULL,
    4,  3, -3,  0,  0,  0,  1,  0,  1, 14,	mfb(bp_torso), "\
A light vest covered in pockets and straps for storage.");

ARMOR("lab coat",	20, 155,C_TORSO,	COTTON,		MNULL,
   11,  7, -3, -2,  1,  1,  2,  0,  1, 14,	mfb(bp_torso), "\
A long white coat with several large pockets.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("light gloves",	35,  65,C_GLOVES,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    0,  0, -5,  1,  1,  0,  0,  0,  1,  0,	mfb(bp_hands), "\
A pair of thin cotton gloves.  Often used as a liner beneath other gloves.");

ARMOR("mittens",	30,  40,C_GLOVES,	WOOL,		MNULL,
    0,  0, -5,  1,  8,  0,  1,  0,  5,  0,	mfb(bp_hands), "\
A pair of warm mittens.  They are extremely encumbersome.");

ARMOR("wool gloves",	33,  50,C_GLOVES,	WOOL,		MNULL,
    1,  0, -5,  1,  3,  0,  1,  0,  3,  0,	mfb(bp_hands), "\
A thick pair of wool gloves.  Encumbersome but warm.");

ARMOR("winter gloves",	40,  65,C_GLOVES,	COTTON,		MNULL,
    1,  0, -5,  1,  5,  1,  1,  0,  4,  0,	mfb(bp_hands), "\
A pair of padded gloves.  Encumbersome but warm.");

ARMOR("leather gloves",	45,  85,C_GLOVES,	LEATHER,	MNULL,
    1,  1, -3,  2,  1,  0,  3,  0,  3,  0,	mfb(bp_hands), "\
A thin pair of leather gloves.  Good for doing manual labor.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("fingerless gloves",20,90,C_GLOVES,	LEATHER,	MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    1,  1, -3,  2,  0,  0,  2,  0,  2,  0,	mfb(bp_hands), "\
A pair of leather gloves with no fingers, allowing greater manual dexterity.");

ARMOR("rubber gloves",	20,  30,C_GLOVES,	PLASTIC,	MNULL,
    1,  1, -3,  2,  3,  0,  1,  2,  1,  0,	mfb(bp_hands), "\
A pair of rubber gloves, often used while cleaning with caustic materials.");

ARMOR("medical gloves",	70,  10,C_GLOVES,	PLASTIC,	MNULL,
    0,  0, -5,  1,  0,  0,  0,  1,  0,  0,	mfb(bp_hands), "\
A pair of thin latex gloves, designed to limit the spread of disease.");

ARMOR("fire gauntlets",	 5,  95,C_GLOVES,	LEATHER,	MNULL,
    3,  5, -2,  2,  6,  1,  2,  5,  4,  0,	mfb(bp_hands), "\
A heavy pair of leather gloves, used by firefighters for heat protection.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("dust mask",	65,  20,C_MOUTH,	COTTON,		IRON,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    0,  0, -5, -3,  1,  0,  0,  2,  1,  0,	mfb(bp_mouth), "\
A simple piece of cotton that straps over the mouth.  Provides a small amount\n\
of protection from air-borne illness and dust.");

ARMOR("bandana",	35,  28,C_MOUTH,	COTTON, 	MNULL,
    1,  0, -4, -1,  0,  0,  0,  1,  2,  0,	mfb(bp_mouth), "\
A cotton bandana, worn over the mouth for warmth and minor protection from\n\
dust and other contaminents.");

ARMOR("scarf",		45,  40,C_MOUTH,	WOOL,   	MNULL,
    2,  3, -5, -3,  1,  1,  0,  2,  3,  0,	mfb(bp_mouth), "\
A long wool scarf, worn over the mouth for warmth.");

ARMOR("filter mask",	30,  80,C_MOUTH,	PLASTIC,	MNULL,
    3,  6,  1,  1,  2,  1,  1,  7,  2,  0,	mfb(bp_mouth), "\
A mask that straps over your mouth and nose and filters air.  Protects from\n\
smoke, dust, and other contaminents quite well.");

ARMOR("gas mask",	10, 240,C_MOUTH,	PLASTIC,	MNULL,
    6,  8,  0, -3,  4,  1,  2, 16,  4,  0,	mfb(bp_mouth)|mfb(bp_eyes), "\
A full gas mask that covers the face and eyes.  Provides excellent protection\n\
from smoke, teargas, and other contaminents.");

// Eyewear - Encumberment is its effect on your eyesight.
// Environment is the defense to your eyes from noxious fumes etc.


//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("eyeglasses",	90, 150,C_EYES,		GLASS,		PLASTIC,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    1,  0, -3, -2,  0,  0,  1,  1,  0,  0,	mfb(bp_eyes), "\
A pair of glasses for the near-sighted.  Useless for anyone else.");

ARMOR("reading glasses",90,  80,C_EYES,		GLASS,		PLASTIC,
    1,  0, -3, -2,  0,  0,  1,  1,  0,  0,	mfb(bp_eyes), "\
A pair of glasses for the far-sighted.  Useless for anyone else.");

ARMOR("safety glasses", 40, 100,C_EYES,		PLASTIC,	MNULL,
    1,  0, -5, -2,  0,  2,  4,  1,  0,  0,	mfb(bp_eyes), "\
A pair of plastic glasses, used in workshops, sports, chemistry labs, and\n\
many other places.  Provides great protection from damage.");

ARMOR("swim goggles",	50, 110,C_EYES,		PLASTIC,	MNULL,
    1,  0, -5, -2,  2,  1,  2,  4,  1,  0,	mfb(bp_eyes), "\
A small pair of goggles.  Distorts vision above water, but allows you to see\n\
much further under water.");

ARMOR("ski goggles",	30, 175,C_EYES,		PLASTIC,	MNULL,
    2,  1, -4, -2,  1,  1,  2,  6,  2,  0,	mfb(bp_eyes), "\
A large pair of goggles that completely seal off your eyes.  Excellent\n\
protection from environmental dangers.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("welding goggles", 8, 240,C_EYES,		GLASS,  	STEEL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    2,  4, -1, -3,  6,  2,  5,  6,  1,  0,	mfb(bp_eyes), "\
A dark pair of goggles.  They make seeing very difficult, but protects you\n\
from bright flashes.");

ARMOR("light amp goggles",1,920,C_EYES,		STEEL,		GLASS,
    3,  6,  1, -2,  2,  2,  3,  6,  2,  0,	mfb(bp_eyes), "\
A pair of goggles that amplify ambient light, allowing you to see in the\n\
dark.  You must be carrying a powered-on unified power supply, or UPS, to use\n\
them."
);

// Headwear encumberment should ONLY be 0 if it's ok to wear with another
// Headwear environmental protection (ENV) drops through to eyes

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("baseball cap",	30,  35,C_HAT,		COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    2,  1, -5,  0,  0,  0,  0,  2,  1,  0,	mfb(bp_head), "\
A Red Sox cap.  It provides a little bit of warmth.");

ARMOR("boonie hat",	10,  55,C_HAT,		PLASTIC,	MNULL,
    2,  1, -5,  0,  0,  0,  1,  2,  2,  0,	mfb(bp_head), "\
Also called a \"bucket hat.\"  Often used in the military.");

ARMOR("cotton hat",	45,  40,C_HAT,		COTTON,		MNULL,
    2,  1, -5,  0,  0,  0,  0,  0,  3,  0,	mfb(bp_head), "\
A snug-fitting cotton hat.  Quite warm.");

ARMOR("knit hat",	25,  50,C_HAT,		WOOL,		MNULL,
    2,  1, -5,  0,  0,  1,  0,  0,  4,  0,	mfb(bp_head), "\
A snug-fitting wool hat.  Very warm.");

ARMOR("hunting cap",	20,  80,C_HAT,		WOOL,		MNULL,
    3,  2, -5,  0,  0,  0,  1,  2,  6,  0,	mfb(bp_head), "\
A red plaid hunting cap with ear flaps.  Notably warm.");

ARMOR("fur hat",	15, 120,C_HAT,		WOOL,		MNULL,
    4,  2, -5,  0,  1,  2,  2,  0,  8,  0,	mfb(bp_head), "\
A hat made from the pelts of animals.  Extremely warm.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("hard hat",	50, 125,C_HAT,		PLASTIC,	MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    8,  4,  6,  0,  1,  4,  5,  0,  1,  0,	mfb(bp_head), "\
A hard plastic hat worn in constructions sites.  Excellent protection from\n\
cuts and percussion.");

ARMOR("bike helmet",	35, 140,C_HAT,		PLASTIC,	MNULL,
   12,  2,  4,  0,  1,  8,  2,  0,  2,  0,	mfb(bp_head), "\
A thick foam helmet.  Designed to protect against percussion.");

ARMOR("skid lid",	30, 190,C_HAT,		PLASTIC,	IRON,
   10,  5,  8,  0,  2,  6, 16,  0,  1,  0,	mfb(bp_head), "\
A small metal helmet that covers the head and protects against cuts and\n\
percussion.");

ARMOR("baseball helmet",45, 195,C_HAT,		PLASTIC,	IRON,
   14,  6,  7, -1,  2, 10, 10,  1,  1,  0,	mfb(bp_head), "\
A hard plastic helmet which covers the head and ears.  Designed to protect\n\
against a baseball to the head.");

ARMOR("army helmet",	40, 480,C_HAT,		PLASTIC,	IRON,
   16,  8, 10, -1,  2, 12, 28,  0,  2,  0,	mfb(bp_head), "\
A heavy helmet whic provides excellent protection from all sorts of damage.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("riot helmet",	25, 420,C_HAT,		PLASTIC,	IRON,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   20,  7,  8, -1,  2,  6, 28,  2,  2,  0,	mfb(bp_head)|mfb(bp_eyes)|
						mfb(bp_mouth), "\
A helmet with a plastic shield that covers your entire face.");

ARMOR("motorcycle helmet",40,325,C_HAT,		PLASTIC,	IRON,
   24,  8,  7, -1,  3,  8, 20,  1,  3,  0,	mfb(bp_head)|mfb(bp_mouth), "\
A helmet with covers your head and chin, leaving space in between for you to\n\
wear goggles.");

ARMOR("chitinous helmet", 1, 380,C_HAT,		FLESH,		MNULL,
   22,  1,  2, -2,  4, 10, 14,  4,  3,  0,	mfb(bp_head)|mfb(bp_eyes)|
						mfb(bp_mouth), "\
A helmet made from the exoskeletons of insects.  Covers the entire head; very\n\
light and durable.");

ARMOR("backpack",	38, 210,C_STORE,	PLASTIC,	MNULL,
   14,  2, -4,  0,  2,  0,  0,  0,  0, 80,	mfb(bp_torso), "\
Provides more storage than any other piece of clothing.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("purse",		40,  75,C_STORE,	LEATHER,	MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   10,  3,  2,  2,  3,  0,  0,  0,  0, 20,	mfb(bp_torso), "\
A bit encumbersome to wear, but provides lots of storage.");

ARMOR("messenger bag",	20, 110,C_STORE,	PLASTIC,	MNULL,
    8,  2,  1,  1,  3,  0,  0,  0,  0, 30,	mfb(bp_torso), "\
A bit encumbersome to wear, but provides lots of storage.");

ARMOR("fanny pack", 	10, 100,C_STORE,	PLASTIC,	MNULL,
    3,  1,  1,  2,  0,  0,  0,  0,  0,  6,	0, "\
Provides a bit of extra storage without encumbering you at all.");

ARMOR("holster",	 8,  90,C_STORE,	LEATHER,	MNULL,
    2,  2,  2, -1,  0,  0,  0,  0,  0,  3,	0, "\
Provides a bit of extra storage without encumbering you at all.");

ARMOR("bootstrap",	 3,  80,C_STORE, 	LEATHER,	MNULL,
    1,  1, -1, -1,  0,  0,  0,  0,  1,  2,	mfb(bp_legs), "\
A small holster worn on the ankle.");

// AMMUNITION
// Material should be the wrapper--even though shot is made of iron, because
//   it can survive a dip in water and be okay, its material here is PLASTIC.
// dmg is damage done, in an average hit.  Note that the average human has
//   80 health.  Headshots do 8x damage; vital hits do 2x-4x; glances do 0x-1x.
// Weight and price is per 100 rounds.
// AP is a reduction in the armor of the target.
// Accuracy is in quarter-degrees, and measures the maximum this ammo will
//   contribute to the angle of difference.  HIGH ACC IS BAD.
// Recoil is cumulitive between shots.  4 recoil = 1 accuracy.
// IMPORTANT: If adding a new AT_*** ammotype, add it to the ammo_name function
//   at the end of this file.
#define AMMO(name,rarity,price,ammo_type,color,mat,volume,wgt,dmg,AP,range,\
accuracy,recoil,count,des,flags) \
	index++;itypes.push_back(new it_ammo(index,rarity,price,name,des,'=',\
color,mat,volume,wgt,1,0,0,flags,ammo_type,dmg,AP,accuracy,recoil,range,count))

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("batteries",	50, 120,AT_BATT,	c_magenta,	IRON,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 1,  1,  0,  0,  0,  0,  0, 100, "\
A set of universal batteries.  Used to charge almost any electronic device.",
0);

AMMO("plutonium cell",	10,3000,AT_PLUT,	c_ltgreen,	STEEL,
	 1,  1,  0,  0,  0,  0,  0, 5, "\
A nuclear-powered battery.  Used to charge advanced and rare electronics.",
0);

AMMO("nails",		35,  60,AT_NAIL,	c_cyan,		IRON,
         1,  8,  4,  3,  3, 40,  4, 100, "\
A box of nails, mainly useful with a hammer.",
0);

AMMO("BB",		 8,  50,AT_BB,		c_pink,		STEEL,
	 1,  6,  2,  0, 12, 20,  0, 200, "\
A box of small steel balls.  They deal virtually no damage.",
0);

AMMO("wood crossbow bolt",8,500,AT_BOLT,	c_green,	WOOD,
	 1, 40, 16,  4, 10, 16,  0,  15, "\
A sharpened bolt carved from wood.  It's lighter than steel bolts, and does\n\
less damage and is less accurate.  Stands a good chance of remaining intact\n\
once fired.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("steel crossbow bolt",7,900,AT_BOLT,	c_green,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 1, 90, 26,  8, 14, 12,  0,  10, "\
A sharp bolt made from steel.  Deadly in skilled hands.  Stands an excellent\n\
chance of remaining intact once fired.",
0);

AMMO("birdshot",	 8, 500,AT_SHOT,	c_red,		PLASTIC,
	 2, 25, 18,  0,  5,  2, 18,  25, "\
Weak shotgun ammuntion.  Designed for hunting birds and other small game, its\n\
applications in combat are very limited.",
0);

AMMO("00 shot",		 8, 800,AT_SHOT,	c_red,		PLASTIC,
	 2, 28, 50,  0,  6,  1, 26,  25, "\
A shell filled with iron pellets.  Extremely damaging, plus the spread makes\n\
it very accurate at short range.  Favored by SWAT forces.",
0);

AMMO("shotgun slug",	 6, 900,AT_SHOT,	c_red,		PLASTIC,
	 2, 34, 50,  4, 12, 10, 28,  25, "\
A heavy metal slug used with shotguns to give them the range capabilities of\n\
a rifle.  Extremely damaging but rather innaccurate.  Works best in a shotgun\n\
with a rifled barrel.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO(".22 LR",		 9, 250,AT_22,		c_ltblue,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 2,  2, 11,  0, 13, 14,  6, 100, "\
One of the smallest calibers available, the .22 Long Rifle cartridge has\n\
maintained popularity for nearly two centuries.  Its minimal recoil, low cost\n\
and low noise are offset by its paltry damage.",
0);

AMMO(".22 CB",		 8, 180,AT_22,		c_ltblue,	STEEL,
	 2,  2,  7,  0, 10, 16,  4, 100, "\
Conical Ball .22 is a variety of .22 ammunition with a very small propellant\n\
charge, generally with no gunpowder, resulting in a subsonic round.  It is\n\
nearly silent, but is so weak as to be nearly useless.",
0);

AMMO(".22 rat-shot",	 2, 230,AT_22,		c_ltblue,	STEEL,
	 2,  2,  5,  0,  3,  2,  4, 100, "\
Rat-shot is extremely weak ammunition, designed for killing rats, snakes, or\n\
other small vermin while being unable to damage walls.  It has an extremely\n\
short range and is unable to injure all but the smallest creatures.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("9mm",		 8, 300,AT_9MM,		c_ltblue,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 2,  7, 18,  1, 14, 16, 13,  50, "\
9 millimeter parabellum is generally regarded as the most popular handgun\n\
cartridge, used by the majority of US police forces.  It is also a very\n\
popular round in sub-machine guns.",
0);

AMMO("9mm +P",		 8, 380,AT_9MM,		c_ltblue,	STEEL,
	 1,  7, 20,  2, 14, 15, 14,  25, "\
Attempts to improve the ballistics of 9mm ammunition lead to high pressure\n\
rounds.  Increased velocity resullts in superior accuracy and damage.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("9mm +P+",		 8, 440,AT_9MM,		c_ltblue,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 1,  7, 22,  6, 16, 14, 15,  10, "\
A step beyond the high-pressure 9mm +P round, the +P+ is a very high pressure\n\
loading which offers a degree of armor-penetrating ability.",
0);

AMMO(".38 Special",	 7, 400,AT_38,		c_ltblue,	STEEL,
	 2, 10, 20,  0, 14, 16, 12,  50, "\
The .38 Smith & Wesson Special enjoyed popularity among US police forces\n\
throughout the 20th century.  It is most commonly used in revolvers.",
0);

AMMO(".38 Super",	 7, 450,AT_38,		c_ltblue,	STEEL,
	 1,  9, 25,  2, 16, 14, 14,  25, "\
The .38 Super is a high-pressure load of the .38 Special caliber.  It is a\n\
popular choice in pistol competions for its high accuracy, while its stopping\n\
power keeps it popular for self-defense.",
0);

AMMO("10mm Auto",	 4, 420,AT_40,		c_blue,		STEEL,
	 2,  9, 26,  5, 14, 18, 20,  50, "\
Originally used by the FBI, the organization eventually abandoned the round\n\
due to its high recoil.  Although respected for its versatility and power, it\n\
has largely been supplanted by the downgraded .40 S&W.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO(".40 S&W",		 7, 450,AT_40,		c_blue,		STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 2,  9, 22,  1, 14, 16, 16,  50, "\
The .40 Smith & Wesson round was developed as an alternative to 10mm Auto for\n\
the FBI after they complained of high recoil.  It is as accurate as 9mm, but\n\
has greater stopping power, leading to widespread use in law enforcement.",
0);

AMMO(".44 Magnum",	 7, 580,AT_44,		c_blue,		STEEL,
	 2, 15, 36,  1, 16, 16, 22,  50, "\
Described (in 1971) by Dirty Harry as \"the most powerful handgun in the\n\
world,\" the .44 Magnum gained widespead popularity due to its depictions in\n\
the media.  In reality, its intense recoil makes it unsuitable in most cases.",
0);

AMMO(".45 ACP",		 7, 470,AT_45,		c_blue,		STEEL,
	 2, 10, 32,  1, 16, 18, 18,  50, "\
The .45 round was one of the most popular and powerful handgun rounds through\n\
the 20th century.  It features very good accuracy and stopping power, but\n\
suffers from moderate recoil and poor armor penetration.",
0);

AMMO(".45 FMJ",		 4, 480,AT_45,		c_blue,		STEEL,
	 1, 13, 26,  8, 16, 18, 18,  25, "\
Full Metal Jacket .45 rounds are designed to overcome the poor armor\n\
penetration of the standard ACP round.  However, they are less likely to\n\
expand upon impact, resulting in reduced damage overall.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO(".45 Super",	 5, 520,AT_45,		c_blue,		STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 1, 11, 34,  4, 18, 16, 20,  10, "\
The .45 Super round is an updated variant of .45 ACP.  It is overloaded,\n\
resulting in a great increase in muzzle velocity.  This translates to higher\n\
accuracy and range, a minor armor piercing capability, and greater recoil.",
0);

AMMO("5.7x28mm",	 3, 500,AT_57,		c_dkgray,	STEEL,
	 3,  2, 14, 15, 12, 12,  6, 100, "\
The 5.7x28mm round is a proprietary round developed by FN Hestal for use in\n\
their P90 SMG.  While it is a very small round, comparable in power to .22,\n\
it features incredible armor-piercing capabilities and very low recoil.",
0);

AMMO("4.6x30mm",	 2, 520,AT_46,		c_dkgray,	STEEL,
	 3,  1, 13, 18, 12, 12,  6, 100, "\
Designed by Heckler & Koch to compete with the 5.7x28mm round, 4.6x30mm is,\n\
like the 5.7, designed to minimize weight and recoil while increasing\n\
penetration of body armor.  Its low recoil makes it ideal for automatic fire.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("7.62x39mm M43",	 6, 500,AT_762,		c_dkgray,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 3,  7, 25,  4, 20, 19, 24,  80, "\
Designed during World War II by the Soviet Union, the popularity of the AK-47\n\
and the SKS contributed to the widespread adaption of the 7.62x39mm rifle\n\
round. However, due to its lack of yaw, this round deals less damage than most."
,
0);

AMMO("7.62x39mm M67",	 7, 650,AT_762,		c_dkgray,	STEEL,
	 3,  8, 28,  5, 20, 17, 25,  80, "\
The M67 variant of the popular 7.62x39mm rifle round was designed to improve\n\
yaw.  This causes the round to tumble inside a target, causing significantly\n\
more damage.  It is still outdone by shattering rounds.",
0);

AMMO(".223 Remington",	 8, 720,AT_223,		c_dkgray,	STEEL,
	 2,  2, 36,  1, 24, 13, 30,  40, "\
The .223 rifle round is a civilian variant of the 5.56 NATO round.  It is\n\
designed to tumble or fragment inside a target, dealing devastating damage.\n\
The lower pressure of the .223 compared to the 5.56 results in lower accuracy."
,
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("5.56 NATO",	 6, 950,AT_223,		c_dkgray,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 2,  4, 40,  2, 25, 10, 32,  40, "\
This rifle round has enjoyed widespread use in NATO countries, thanks to its\n\
very light weight and high damage.  It is designed to shatter inside a\n\
target, inflicting massive damage.",
0);

AMMO("5.56 incendiary",	 2,1140,AT_223,		c_dkgray,	STEEL,
	 2,  4, 28,  7, 25, 11, 32, 30, "\
A variant of the widely-used 5.56 NATO round, incendiary rounds are designed\n\
to burn hotly upon impact, piercing armor and igniting flammable substances.",
mfb(WF_AMMO_INCENDIARY));

AMMO(".270 Winchester",	 8, 900,AT_3006,	c_dkgray,	STEEL,
	 1,  7, 42,  2, 40, 12, 34,  20, "\
Based off the military .30-03 round, the .270 rifle round is compatible with\n\
most guns that fire .30-06 rounds.  However, it is designed for hunting, and\n\
is less powerful than the military rounds, with nearly no armor penetration.",
0);

AMMO(".30-06 AP",	 4,1050,AT_3006,	c_dkgray,	STEEL,
	 1, 12, 50, 16, 40,  7, 36,  10, "\
The .30-06 is a very powerful rifle round designed for long-range use.  Its\n\
stupendous accuracy and armor piercing capabilities make it one of the most\n\
deadly rounds available, offset only by its drastic recoil and noise.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO(".30-06 incendiary", 1,1180,AT_3006,	c_dkgray,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	  1, 12, 35, 20, 40,  8, 35,  5, "\
A variant of the powerful .30-06 sniper round, incendiary rounds are designed\n\
to burn hotly upon impact, piercing armor and igniting flammable substances.",
mfb(WF_AMMO_INCENDIARY));

AMMO(".308 Winchester",	 7, 920,AT_308,		c_dkgray,	STEEL,
	 1,  9, 36,  1, 35,  7, 33,  20, "\
The .308 Winchester is a rifle round, the commercial equivalent of the\n\
military 7.62x51mm round.  Its high accuracy and phenominal damage have made\n\
it the most poplar hunting round in the world.",
0);

AMMO("7.62x51mm",	 6,1040,AT_308,		c_dkgray,	STEEL,
	 1,  9, 44,  4, 35,  6, 34,  20, "\
The 7.62x51mm largely replaced the .30-06 round as the standard military\n\
rifle round.  It is lighter, but offers similar velocities, resulting in\n\
greater accuracy and reduced recoil.",
0);

//  NAME		   RAR PRC TYPE		COLOR		MAT
AMMO("7.62x51mm incendiary",6,1040,AT_308,	c_dkgray,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	  1,  9, 30, 12, 32,  6, 32,  10, "\
A variant of the powerful 7.62x51mm round, incendiary rounds are designed\n\
to burn hotly upon impact, piercing armor and igniting flammable substances.",
mfb(WF_AMMO_INCENDIARY));

AMMO("fusion pack",	 2,1200,AT_FUSION,	c_ltgreen,	PLASTIC,
	 1,  2, 12,  6, 20,  4,  0,  20, "\
In the middle of the 21st Century, military powers began to look towards\n\
energy based weapons.  The result was the standard fusion pack, capable of\n\
delivering bolts of superheaed gas at near light speed with no recoil.",
mfb(WF_AMMO_INCENDIARY));

// FUEL
// Fuel is just a special type of ammo; liquid
#define FUEL(name,rarity,price,ammo_type,color,dmg,AP,range,accuracy,recoil,\
             count,des,flags) \
	index++;itypes.push_back(new it_ammo(index,rarity,price,name,des,'~',\
color,LIQUID,1,1,0,0,0,flags,ammo_type,dmg,AP,accuracy,recoil,range,count))
FUEL("gasoline",	0, 400,   AT_GAS,	c_ltred,
//	DMG  AP RNG ACC REC COUNT
	 0,  0,  4,  0,  0,  1, "\
Gasoline is a highly flammable liquid.  When under pressure, it has the\n\
potential for violent explosion.",
mfb(WF_AMMO_FLAME));

// GUNS
// ammo_type matches one of the ammo_types above.
// dmg is ADDED to the damage of the corresponding ammo.  +/-, should be small.
// aim affects chances of hitting; low for handguns, hi for rifles, etc, small.
// Durability is rated 1-10; 10 near perfect, 1 it breaks every few shots
// Burst is the # of rounds fired, 0 if no burst ability.
// clip is how many shots we get before reloading.

#define GUN(name,rarity,price,color,mat1,mat2,skill,ammo,volume,wgt,melee_dam,\
to_hit,dmg,accuracy,recoil,durability,burst,clip,des,flags) \
	index++;itypes.push_back(new it_gun(index,rarity,price,name,des,'(',\
color,mat1,mat2,volume,wgt,melee_dam,0,to_hit,flags,skill,ammo,dmg,accuracy,\
recoil,durability,burst,clip))

//  NAME		RAR PRC COLOR		MAT1	MAT2
GUN("nail gun",		12, 100,c_ltblue,	IRON,	MNULL,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_pistol,	AT_NAIL, 4, 22, 12,  1,  0, 20,  0,  8,  5, 100, "\
A tool used to drive nails into wood or other material.  It could also be\n\
used as a ad-hoc weapon, or to practice your handgun skill up to level 1.",
0);

GUN("BB gun",		10, 100,c_ltblue,	IRON,	WOOD,
	sk_rifle,	AT_BB,	 8, 16,  9,  2,  0,  6, -5,  7,  0, 20, "\
Popular among children.  It's fairly accurate, but BBs deal nearly no damage.\n\
It could be used to practice your rifle skill up to level 1.",
0);

GUN("crossbow",		 2, 500,c_green,	IRON,	WOOD,
	sk_pistol,	AT_BOLT, 6,  9, 11,  1,  0, 18,  0,  6,  0,  1, "\
A slow-loading hand weapon that launches bolts.  Stronger people can reload\n\
it much faster.  Bolts fired from this weapon have a good chance of remaining\n\
intact for re-use.",
0);

GUN("pipe rifle: .22",	0,  400,c_ltblue,	IRON,	WOOD,
	sk_rifle,	AT_22,	 9, 13, 10,  2, -2, 15,  2,  6,  0,  1, "\
A home-made rifle.  It is simply a pipe attached to a stock, with a hammer to\n\
strike the single round it holds.",
0);

GUN("pipe rifle: 9mm",	0,  460,c_ltblue,	IRON,	WOOD,
	sk_rifle,	AT_9MM,	10, 16, 10,  2, -2, 15,  2,  6,  0,  1, "\
A home-made rifle.  It is simply a pipe attached to a stock, with a hammer to\n\
strike the single round it holds.",
0);

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("pipe SMG: 9mm",	0,  540,c_ltblue,	IRON,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_smg,		AT_9MM,  5,  8,  6, -1,  0, 30,  6,  5,  4, 10, "\
A home-made machine pistol.  It features a rudimentary blowback system, which\n\
allows for small bursts.",
0);

GUN("pipe SMG: .45",	0,  575,c_ltblue,	IRON,	WOOD,
	sk_smg,		AT_45,	 6,  9,  7, -1,  0, 30,  6,  5,  3,  8, "\
A home-made machine pistol.  It features a rudimentary blowback system, which\n\
allows for small bursts.",
0);

GUN("SIG Mosquito",	 5, 600,c_dkgray,	STEEL,	PLASTIC,
	sk_pistol,	AT_22,	 1,  6,  9,  1,  1, 28,  4,  8,  0, 10, "\
A popular, very small .22 pistol.  \"Ergonomically designed to give the best\n\
shooting experience.\" --SIG Sauer official website",
0);

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("S&W 22A",		 5, 650,c_dkgray,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_pistol,	AT_22,	 1, 10,  9,  1,  1, 25,  5,  7,  0, 10, "\
A popular .22 pistol.  \"Ideal for competitive target shooting or recreational\n\
shooting.\" --Smith & Wesson official website",
0);

GUN("Glock 19",		 7, 700,c_dkgray,	STEEL,	PLASTIC,
	sk_pistol,	AT_9MM,	 2,  5,  8,  1,  2, 24,  6,  6,  0, 15, "\
Possibly the most popular pistol in existance.  The Glock 19 is often derided\n\
for its plastic contruction, but it is easy to shoot.",
0);

GUN("USP 9mm",		 6, 780,c_dkgray,	STEEL,	PLASTIC,
	sk_pistol,	AT_9MM,	 2,  6,  8,  1,  1, 25,  5,  9,  0, 15, "\
A popular 9mm pistol, widely used among law enforcement.  Extensively tested\n\
for durability, it has been found to stay accurate even after subjected to\n\
extreme abuse.",
0);

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("S&W 619",		 4, 720,c_dkgray,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_pistol,	AT_38,	 2,  9,  9,  1,  2, 23,  4,  8,  0,  7, "\
A seven-round .38 revolver sold by Smith & Wesson.  It features a fixed rear\n\
sight and a reinforced frame.",
0);

GUN("Taurus Pro .38",	 4, 760,c_dkgray,	STEEL,	PLASTIC,
	sk_pistol,	AT_38,	 2,  6,  8,  1,  1, 22,  6,  7,  0, 10, "\
A popular .38 pistol.  Designed with numerous safety features and built from\n\
high-quality, durable materials.",
0);

GUN("SIG Pro .40",	 4, 750,c_dkgray,	STEEL,	PLASTIC,
	sk_pistol,	AT_40,	 2,  6,  8,  1,  1, 22,  8,  7,  0, 12, "\
Originally marketed as a lightweight and compact alternative to older SIG\n\
handguns, the Pro .40 is popular among European police forces.",
0);

GUN("S&W 610",		 2, 720,c_dkgray,	STEEL,	WOOD,
	sk_pistol,	AT_40,	 2, 10, 10,  1,  2, 23,  6,  8,  0,  6, "\
The Smith and Wesson 610 is a classic six-shooter revolver chambered for 10mm\n\
rounds, or for S&W's own .40 round.",
0);

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("Ruger Redhawk",	 3, 760,c_dkgray,	STEEL,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_pistol,	AT_44,	 2, 12, 10,  1,  2, 21,  6,  8,  0,  6, "\
One of the most powerful handguns in the world when it was released in 1979,\n\
the Redhawk offers very sturdy contruction, with an appearance that is\n\
reminiscent of \"Wild West\" revolvers.",
0);

GUN("Desert Eagle .44",	 2, 840,c_dkgray,	STEEL,	PLASTIC,
	sk_pistol,	AT_44,	 4, 17, 14,  1,  4, 35,  3,  7,  0, 10, "\
One of the most recognizable handguns due to its popularity in movies, the\n\
\"Deagle\" is better known for its menacing appearance than its performace.\n\
It's highly innaccurate, but its heavy weight reduces recoil.",
0);

GUN("USP .45",		 6, 800,c_dkgray,	STEEL,	PLASTIC,
	sk_pistol,	AT_45,	 2,  7,  9,  1,  1, 25,  8,  9,  0, 12, "\
A popular .45 pistol, widely used among law enforcement.  Extensively tested\n\
for durability, it has been found to stay accurate even after subjected to\n\
extreme abuse.",
0);

GUN("M1911",		 5, 880,c_ltgray,	STEEL,	PLASTIC,
	sk_pistol,	AT_45,	 3, 10, 12,  1,  6, 25,  9,  7,  0,  7, "\
The M1911 was the standard-issue sidearm from the US Military for most of the\n\
20th Century.  It remains one of the most popular .45 pistols today.",
0);

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("FN Five-Seven",	 2, 600,c_ltgray,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_pistol,	AT_57,	 2,  5,  6,  0,  2, 13,  6,  8,  0, 20, "\
Designed to work with FN's proprietary 5.7x28mm round, the Five-Seven is a\n\
lightweight pistol with a very high capacity, best used against armored\n\
opponents.",
0);

GUN("H&K UCP",		 2, 620,c_ltgray,	STEEL,	PLASTIC,
	sk_pistol,	AT_46,	 2,  5,  6,  0,  2, 12,  6,  8,  0, 20, "\
Designed to work with H&K's proprietary 4.6x30mm round, the UCP is a small\n\
pistol with a very high capacity, best used against armored opponents.",
0);

GUN("sawn-off shotgun",	 1, 350,c_red,	IRON,	WOOD,
	sk_shotgun,	AT_SHOT, 6, 10, 14,  2,  4, 40, 15,  4,  0,  2, "\
The barrels of shotguns are often sawed in half to make it more maneuverable\n\
and concealable.  This has the added effect of reducing accuracy greatly.",
mfb(WF_RELOAD_ONE));

GUN("single barrel shotgun",1,300,c_red,IRON,	WOOD,
	sk_shotgun,	AT_SHOT,10, 20, 14,  3,  0,  6,  5,  6,  0,  1, "\
An old shotgun, possibly antique.  It is little more than a barrel, a wood\n\
stock, and a hammer to strike the cartridge.  Its simple design keeps it both\n\
light and accurate.",
0);

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("double barrel shotgun",2,580,c_red,IRON,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_shotgun,	AT_SHOT,12, 26, 15,  3,  0,  7,  4,  7,  2,  2, "\
An old shotgun, possibly antique.  It is little more than a pair of barrels,\n\
a wood stock, and a hammer to strike the cartridge.",
mfb(WF_RELOAD_ONE));

GUN("Remington 870",	 9,1200,c_red,	STEEL,	PLASTIC,
	sk_shotgun,	AT_SHOT,16, 30, 17,  3,  5, 10,  0,  8,  3,  6, "\
One of the most popular shotguns on the market, the Remington 870 is used by\n\
hunters and law enforcement agencies alike thanks to its high accuracy and\n\
muzzle velocity.",
mfb(WF_RELOAD_ONE));

GUN("Mossberg 500",	 5,1150,c_red,	STEEL,	PLASTIC,
	sk_shotgun,	AT_SHOT,15, 30, 17,  3,  0, 13, -2,  9,  3,  8, "\
The Mossberg 500 is a popular series of pump-action shotguns, often acquired\n\
for military use.  It is noted for its high durability and low recoil.",
mfb(WF_RELOAD_ONE));

GUN("Saiga-12",		 3,1100,c_red,	STEEL,	PLASTIC,
	sk_shotgun,	AT_SHOT,15, 36, 17,  3,  0, 17,  2,  7,  4, 10, "\
The Saiga-12 shotgun is designed on the same Kalishnikov pattern as the AK47\n\
rifle.  It reloads with a magazine, rather than one shell at a time like most\n\
shotguns.",
0);

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("American-180",	 2, 800,c_cyan, STEEL,	MNULL,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_smg,		AT_22,  12, 23, 11,  0,  2, 20,  0,  6, 20,165, "\
The American-180 is a submachine gun developed in the 1960s which fires .22\n\
LR, unusual for an SMG.  Though the round is low-powered, the high rate of\n\
fire and large magazine makes the 180 a formidable weapon.",
0);

GUN("Uzi 9mm",		 8, 980,c_cyan,	STEEL,	MNULL,
	sk_smg,		AT_9MM,	 6, 29, 10,  1,  0, 25, -2,  7,  8, 32, "\
The Uzi 9mm has enjoyed immense popularity, selling more units than any other\n\
submachine gun.  It is widely used as a personal defense weapon, or as a\n\
primary weapon by elite frontline forces.",
0);

GUN("TEC-9",		10, 880,c_cyan,	STEEL,	MNULL,
	sk_smg,		AT_9MM,	 5, 12,  9,  1,  3, 24,  0,  6,  6, 32, "\
The TEC-9 is a machine pistol made of cheap polymers and machine stamped\n\
parts.  Its rise in popularity among criminals is largely due to its\n\
intimidating looks and low price.",
0);

GUN("Calico M960",	 6,1200,c_cyan,	STEEL,	MNULL,
	sk_smg,		AT_9MM,	 7, 19,  9,  1, -3, 28, -4,  6, 12, 50, "\
The Calico M960 is an automatic carbine with a unique circular magazine which\n\
allows for high capacities and reduced recoil.",
0);

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("H&K MP5",		12,1400,c_cyan,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_smg,		AT_9MM,	12, 26, 10,  2, -1, 18, -3,  8,  4, 30, "\
The Heckler & Koch MP5 is one of the most widely-used submachine guns in the\n\
world, and has been adopted by special police forces and militaries alike.\n\
Its high degree of accuracy and low recoil are universally praised.",
0);

GUN("MAC-10",		14, 920,c_cyan,	STEEL,	MNULL,
	sk_smg,		AT_45,	 4, 25,  8,  1, -4, 28,  0,  7, 20, 30, "\
The MAC-10 is a popular machine pistol originally designed for military use.\n\
For many years they were the most inexpensive automatic weapon in the US, and\n\
enjoyed great popularity among criminals less concerned with quality firearms."
,
0);

GUN("H&K UMP45",	12,1500,c_cyan,	STEEL,	PLASTIC,
	sk_smg,		AT_45,	13, 20, 11,  1,  0, 13, -3,  8,  4, 25, "\
Developed as a successor to the MP5 submachine gun, the UMP45 retains the\n\
earlier model's supreme accuracy and low recoil, but in the higher .45 caliber."
,
0);

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("TDI Vector",	 4,1800,c_cyan,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_smg,		AT_45,	13, 20,  9,  0, -2, 15,-14,  7,  8, 30, "\
The TDI Vector is a submachine gun with a unique in-line design which makes\n\
recoil very managable, even in the powerful .45 caliber.",
0);

GUN("FN P90",		 7,2000,c_cyan,	STEEL,	PLASTIC,
	sk_smg,		AT_57,	14, 22, 10,  1,  0, 22, -8,  8, 15, 50, "\
The first in a new genre of guns, termed \"personal defense weapons.\"  FN\n\
designed the P90 to use their proprietary 5.7x28mm ammunition.  It is made\n\
for firing bursts managably.",
0);

GUN("H&K MP7",		 5,1600,c_cyan,	STEEL,	PLASTIC,
	sk_smg,		AT_46,	 7, 17,	 7,  1,  0, 21,-10,  8, 20, 20, "\
Designed by Heckler & Koch as a competitor to the FN P90, as well as a\n\
successor to the extremely popular H&K MP5.  Using H&K's proprietary 4.6x30mm\n\
ammunition, it is designed for burst fire.",
0);

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("Marlin 39A",	14, 800,c_brown,IRON,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_rifle,	AT_22,	11, 26, 12,  3,  3, 10, -5,  8,  0, 10, "\
The oldest and longest-produced shoulder firearm in the world.  Though it\n\
fires the weak .22 round, it is highly accurate and damaging, and essentially\n\
has no recoil.",
0);

GUN("Ruger 10/22",	12, 820,c_brown,IRON,	WOOD,
	sk_rifle,	AT_22,	11, 23, 12,  3,  0,  8, -5,  8,  0, 10, "\
A popular and highly accurate .22 rifle.  At the time of its introduction in\n\
1964, it was one of the first modern .22 rifles designed for quality, and not\n\
as a gun for children.",
0);

GUN("Browning BLR",	 8,1200,c_brown,IRON,	WOOD,
	sk_rifle,	AT_3006,12, 28, 12,  3, -3,  6, -4,  7,  0,  4, "\
A very popular rifle for hunting and sniping.  Its low ammo capacity is\n\
offset by the very powerful .30-06 round it fires.",
0);

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("Remington 700",	14,1300,c_brown,IRON,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_rifle,	AT_3006,12, 34, 13,  3,  7,  9, -3,  8,  0,  4, "\
A very popular and durable hunting or sniping rifle.  Popular among SWAT\n\
and US Marine snipers.  Highly damaging, but perhaps not as accurate as the\n\
competing Browning BLR.",
mfb(WF_RELOAD_ONE));

GUN("SKS",		12,1600,c_brown,IRON,	WOOD,
	sk_rifle,	AT_762,	12, 34, 13,  3,  0,  5, -4,  8,  0, 10, "\
Developed by the Soviets in 1945, this rifle was quickly replaced by the\n\
full-auto AK47.  However, due to its superb accuracy and low recoil, this gun\n\
maintains immense popularity.",
0);

GUN("Ruger Mini-14",	12,1650,c_brown,IRON,	WOOD,
	sk_rifle,	AT_223,	12, 26, 12,  3,  4,  5, -4,  8,  0, 10, "\
A small, lightweight semi-auto carbine designed for military use.  Its superb\n\
accuracy and low recoil makes it more suitable than full-auto rifles for some\n\
situations.",
0);

GUN("Savage 111F",	10,1980,c_brown,STEEL,	PLASTIC,
	sk_rifle,	AT_308, 12, 26, 13,  3,  6,  5,-11,  9,  0,  3, "\
A very accurate rifle chambered for the powerful .308 round.  Its very low\n\
ammo capacity is offset by its accuracy and near-complete lack of recoil.",
mfb(WF_RELOAD_ONE));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("H&K G3",		15,2550,c_blue,	IRON,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_rifle,	AT_308,	16, 40, 13,  2,  8, 11,  4,  8,  7, 20, "\
An early battle rifle developed after the end of WWII.  The G3 is designed to\n\
unload large amounts of deadly ammunition, but it is less suitable over long\n\
ranges.",
0);

GUN("H&K G36",		17,2300,c_blue,	IRON,	PLASTIC,
	sk_rifle,	AT_223, 15, 32, 13,  2,  6,  8,  5,  8, 10, 30, "\
Designed as a replacement for the early H&K G3 battle rifle, the G36 is more\n\
accurate, and uses the much-lighter .223 round, allowing for a higher ammo\n\
capacity.",
0);

GUN("AK-47",		16,2100,c_blue,	IRON,	WOOD,
	sk_rifle,	AT_762,	16, 38, 14,  2,  0, 13,  4,  9,  4, 30, "\
One of the most recognizable assault rifles ever made, the AK-47 is renowned\n\
for its durability even under the worst conditions.",
0);

GUN("FN FAL",		16,2250,c_blue,	IRON,	WOOD,
	sk_rifle,	AT_308,	19, 36, 14,  2,  7, 15, -2,  8, 10, 20, "\
A Belgian-designed battle rifle, the FN FAL is not very accurate for a rifle,\n\
but its high fire rate and powerful .308 ammunition have made it one of the\n\
most widely-used battle rifles in the world.",
0);

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("Bushmaster ACR",	 4,2150,c_blue,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_rifle,	AT_223,	15, 27,	18,  2,  2, 10, -2,  8,  3, 30, "\
This carbine was developed for military use in the early 21st century.  It is\n\
damaging and accurate, though its rate of fire is a bit slower than competing\n\
.223 carbines.",
0);

GUN("AR-15",		 9,2200,c_blue,	STEEL,	PLASTIC,
	sk_rifle,	AT_223,	19, 28, 12,  2,  0,  6,  0,  7,  5, 30, "\
A widely used assault rifle and the father of popular rifles such as the M16.\n\
It is light and accurate, but not very durable.",
0);

GUN("M4A1",		 7,2400,c_blue,	STEEL,	PLASTIC,
	sk_rifle,	AT_223, 14, 24, 13,  2,  4,  7,  2,  6,  5, 30, "\
A popular carbine, long used by the US military.  Though accurate, small, and\n\
lightweight, it is infamous for its fragility, particularly in less-than-\n\
ideal terrain.",
0);

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("FN SCAR-L",	 6,2500,c_blue,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_rifle,	AT_223,	15, 29, 18,  2,  1,  6, -4,  8,  6, 30, "\
A modular assault rifle designed for use by US Special Ops units.  The 'L' in\n\
its name stands for light, as it uses the lightweight .223 round.  It is very\n\
accurate and low on recoil.",
0);

GUN("FN SCAR-H",	 5,2750,c_blue,	STEEL,	PLASTIC,
	sk_rifle,	AT_308,	16, 32, 20,  2,  1, 12, -4,  8,  5, 20, "\
A modular assault rifle designed for use by US Special Ops units.  The 'H' in\n\
its name stands for heavy, as it uses the powerful .308 round.  It is fairly\n\
accurate and low on recoil.",
0);

GUN("Steyr AUG",	 6,2900,c_blue, STEEL,	PLASTIC,
	sk_rifle,	AT_223, 14, 32, 17,  1, -3,  7, -8,  8,  3, 30, "\
The Steyr AUG is an Austrian assault rifle that uses a bullpup design.  It is\n\
used in the armed forces and police forces of many nations, and enjoys\n\
low recoil and high accuracy.",
0);

GUN("M249",		 1,3500,c_ltred,STEEL,	PLASTIC,
	sk_rifle,	AT_223,	32, 68, 27, -4, -6, 20,  6,  7, 20,200, "\
The M249 is a mountable machine gun used by the US Military and SWAT teams.\n\
Quite innaccurate and difficult to control, the M249 is designed to fire many\n\
rounds very quickly."
,
0);

//  NAME		RAR PRC COLOR	 MAT1	MAT2
GUN("V29 laser pistol",	 1,3800,c_magenta,STEEL,PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_pistol,	AT_FUSION,4, 6,  5,  1, -2, 20,  0,  8,  0, 20, "\
The V29 laser pistol was designed in the mid-21st century, and was one of the\n\
first firearms to use fusion as its ammunition.  It is larger than most\n\
traditional handguns, but displays no recoil whatsoever.",
0);

GUN("FTK-93 fusion gun", 1,5200,c_magenta,STEEL, PLASTIC,
	sk_rifle,	AT_FUSION,18,20, 10, 1, 40, 10,  0,  9,  0,  2, "\
A very powerful fusion rifle developed shortly before the influx of monsters.\n\
It can only hold two rounds at a time, but a special superheating unit causes\n\
its bolts to be extremely deadly.",
0);

//  NAME		RAR PRC COLOR	 MAT1	MAT2
GUN("simple flamethrower",1,800,c_pink,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_shotgun,	AT_GAS, 16,  8,   8, -1, -5,  6,  0,  6,  0, 12, "\
A simple, home-made flamethrower.  While its capacity is not superb, it is\n\
more than capable of igniting terrain and monsters alike.",
0);

GUN("flamethrower",	 1,1800,c_pink,	STEEL,	MNULL,
	sk_shotgun,	AT_GAS, 20, 14, 10, -2, 10,  4,  0,  8,  4, 100, "\
A large flamethrower with substantial gas reserves.  Very manacing and\n\
deadly.",
0);

// GUN MODS
// Accuracy is inverted from guns; high values are a bonus, low values a penalty
// The clip modification is a percentage of the original clip size.
// The final variable is a bitfield of acceptible ammo types.  Using 0 means
//   that any ammo type is acceptable (so long as the mod works on the class of
//   gun)
#define GUNMOD(name, rare, value, color, mat1, mat2, volume, weight, meleedam,\
               meleecut, meleehit, acc, damage, loudness, clip, recoil, burst,\
               newtype, pistol, shotgun, smg, rifle, a_a_t, des)\
  index++; itypes.push_back(new it_gunmod(index, rare, value, name, des, ':',\
                            color, mat1, mat2, volume, weight, meleedam,\
                            meleecut, meleehit, acc, 0, damage, loudness, clip,\
                            recoil, burst, newtype, a_a_t, pistol, shotgun,\
                            smg, rifle))


//	NAME      	 RAR  PRC  COLOR     MAT1   MAT2     VOL WGT DAM CUT HIT
GUNMOD("silencer",	 15,  480, c_dkgray, STEEL, PLASTIC,  2,  1,  3,  0,  2,
//	ACC DAM NOI CLP REC BST NEWTYPE,	PISTOL	SHOT	SMG	RIFLE
	-1, -4,-45,  0,  0,  0,	AT_NULL,	true,	false,	true,	true,
	0, "\
Using a silencer is almost an imperative in zombie-infested regions.  Gunfire\n\
is very noisy, and will attract predators.  Its only drawback is a reduced\n\
muzzle velocity, resulting in less accuracy and damage."
);

GUNMOD("enhanced grip",  12, 280, c_brown,  STEEL, PLASTIC,   1,  1,  0,  0, -1,
	 2,  0,  0,  0, -2,  0, AT_NULL,	false,	true,	true,	true,
	0, "\
A grip placed forward on the barrel allows for greater control and accuracy.\n\
Aside from increased weight, there are no drawbacks."
);

GUNMOD("barrel extension",10,400,  c_ltgray, STEEL, MNULL,    4,  1,  5,  0,  2,
	 6,  1,  0,  0,  5,  0,	AT_NULL,	false,	false,	true,	true,
	0, "\
A longer barrel increases the muzzle velocity of a firearm, contributing to\n\
both accuracy and damage.  However, the longer barrel tends to vibrate after\n\
firing, greatly increasing recoil."
);

//	NAME      	 RAR  PRC  COLOR     MAT1   MAT2     VOL WGT DAM CUT HIT
GUNMOD("shortened barrel", 6, 320, c_ltgray, STEEL, MNULL,    1,  1, -2,  0, -1,
//	ACC DAM NOI CLP REC BST NEWTYPE,	PISTOL	SHOT	SMG	RIFLE
	-5,  0,  2,  0, -6,  0, AT_NULL,	false,	false,	true,	true,
	0, "\
A shortened barrel results in markedly reduced accuracy, and a minor increase\n\
in noise, but also reduces recoil greatly as a result of the improved\n\
managability of the firearm."
);

GUNMOD("rifled barrel",    5, 220, c_ltgray, STEEL, MNULL,    2,  1,  3,  0,  1,
	10,-20,  0,  0,  0, -1, AT_NULL,	false,	true,	false,	false,
	0, "\
Rifling a shotgun barrel is mainly done in order to improve its accuracy when\n\
firing slugs.  The rifling makes the gun less suitable for shot, however."
);

GUNMOD("extended clip",	  8,  560, c_ltgray, STEEL, PLASTIC,  1,  1, -2,  0, -1,
	-1,  0,  0, 50,  0,  0, AT_NULL,	true,	true,	true,	true,
	0, "\
Increases the ammunition capacity of your firearm by 50%, but the added bulk\n\
reduces accuracy slightly."
);

//	NAME      	 RAR  PRC  COLOR     MAT1   MAT2     VOL WGT DAM CUT HIT
GUNMOD("double clip",	   4, 720, c_ltgray, STEEL, PLASTIC,  2,  2,  0,  0,  0,
//	ACC DAM NOI CLP REC BST NEWTYPE,	PISTOL	SHOT	SMG	RIFLE
	-2,  0,  0,100,  2,  0, AT_NULL,	false,	true,	true,	true,
	0, "\
Completely doubles the ammunition capacity of your firmarm, but the added\n\
bulk reduces accuracy and increases recoil."
);

GUNMOD("gyroscopic stablizer",4,680,c_blue,  STEEL, PLASTIC,  3,  2,  0,  0, -3,
	 2, -2,  0,-10, -8,  0, AT_NULL,	false,	false,	true,	true,
	0, "\
An advanced unit which straps onto the side of your firearm and reduces\n\
vibration, greatly reducing recoil and increasing accuracy.  However, it also\n\
takes up space in the magazine slot, reducing ammo capacity."
);

GUNMOD("rapid blowback",   3, 700, c_red,    STEEL, PLASTIC,  0,  1,  0,  0,  0,
	-3,  0,  4,  0,  0,  6, AT_NULL,	false,	false,	true,	true,
	0, "\
An improved blowback mechanism makes your firearm's automatic fire faster, at\n\
the cost of reduced accuracy and increased noise."
);

//	NAME      	 RAR  PRC  COLOR     MAT1   MAT2     VOL WGT DAM CUT HIT
GUNMOD("auto-fire mechanism",2,650,c_red,    STEEL, PLASTIC,  1,  2,  2,  0, -1,
//	ACC DAM NOI CLP REC BST NEWTYPE,	PISTOL	SHOT	SMG	RIFLE
	-2,  0,  2,  0,  2,  3, AT_NULL,	true,	false,	false,	false,
	(mfb(AT_22)|mfb(AT_9MM)|mfb(AT_38)|mfb(AT_40)|mfb(AT_57)|mfb(AT_46)), "\
A simple mechanism which converts a pistol to a fully-automatic weapon, with\n\
a burst size of three rounds.  However, it reduces accuracy, while increasing\n\
noise and recoil."
);

//	NAME      	 RAR  PRC  COLOR     MAT1   MAT2     VOL WGT DAM CUT HIT
GUNMOD(".45 caliber retool",3,480, c_green,  STEEL, MNULL,    2,  2,  3,  0, -1,
//	ACC DAM NOI CLP REC BST NEWTYPE,	PISTOL	SHOT	SMG	RIFLE
	-2,  0,  0,  0,  2,  0, AT_45,		true,	false,	true,	false,
	(mfb(AT_9MM)|mfb(AT_38)|mfb(AT_40)|mfb(AT_44)), "\
Replacing several key parts of a 9mm, .38, .40 or .44 firearm converts it to\n\
a .45 firearm.  The conversion results in reduced accuracy and increased\n\
recoil."
);

GUNMOD("9mm caliber retool",3,420, c_green,  STEEL, MNULL,    1,  1,  0,  0, -1,
	-1,  0,  0,  0,  0,  0, AT_9MM,		true,	false,	true,	false,
	(mfb(AT_38)|mfb(AT_40)|mfb(AT_44)|mfb(AT_45)), "\
Replacing several key parts of a .38, .40, .44 or .45 firearm converts it to\n\
a 9mm firearm.  The conversion results in a slight reduction in accuracy."
);

GUNMOD(".22 caliber retool",2,320, c_green,  STEEL, MNULL,    1,  1, -2,  0, -1,
	-1,  0,  0,  0,  0,  0, AT_22,		true,	false,	true,	true,
	(mfb(AT_9MM)|mfb(AT_38)|mfb(AT_40)|mfb(AT_57)|mfb(AT_46)|mfb(AT_762)|
	 mfb(AT_223)), "\
Replacing several key parts of a 9mm, .38, .40, 5.7mm, 4.6mm, 7.62mm or .223\n\
firearm converts it to a .22 firearm.  The conversion results in a slight\n\
reduction in accuracy."
);

//	NAME      	 RAR  PRC  COLOR     MAT1   MAT2     VOL WGT DAM CUT HIT
GUNMOD("5.7mm caliber retool",1,460,c_green, STEEL, MNULL,    1,  1, -3,  0, -1,
//	ACC DAM NOI CLP REC BST NEWTYPE,	PISTOL	SHOT	SMG	RIFLE
	-1,  0,  0,  0,  0,  0, AT_57,		true,	false,	true,	false,
	(mfb(AT_22)|mfb(AT_9MM)|mfb(AT_38)), "\
FN Hestal sells a conversion kit, used to convert .22, 9mm, or .38 firearms\n\
to their proprietary 5.7x28mm, a round designed for accuracy and armor\n\
penetration."
);

GUNMOD("4.6mm caliber retool",1,460,c_green, STEEL, MNULL,    1,  1, -3,  0, -1,
	-1,  0,  0,  0,  0,  0, AT_46,		true,	false,	true,	false,
	(mfb(AT_22)|mfb(AT_9MM)|mfb(AT_38)), "\
Heckler and Koch sells a conversion kit, used to convert .22, 9mm, or .38\n\
firearms to their proprietary 4.6x30mm, a round designed for accuracy and\n\
armor penetration."
);

//	NAME      	RAR  PRC  COLOR     MAT1   MAT2      VOL WGT DAM CUT HIT
GUNMOD(".308 caliber retool",2,520,c_green, STEEL, MNULL,     2,  1,  4,  0,  1,
//	ACC DAM NOI CLP REC BST NEWTYPE		PISTOL	SHOT	SMG	RIFLE
	-2,  0,  0,-20,  0,  0, AT_308,		false,	true,	false,	true,
	(mfb(AT_SHOT)|mfb(AT_762)|mfb(AT_223)|mfb(AT_3006)), "\
This kit is used to convert a shotgun or 7.62mm, .223 or .30-06 rifle to the\n\
popular and powerful .308 caliber.  The conversion results in reduced ammo\n\
capacity and a slight reduction in accuracy."
);

GUNMOD(".223 caliber retool",2,500,c_green, STEEL, MNULL,     2,  1,  4,  0,  1,
	-2,  0,  0,-10,  0,  0, AT_223,		false,	true,	false,	true,
	(mfb(AT_SHOT)|mfb(AT_762)|mfb(AT_3006)|mfb(AT_308)), "\
This kit is used to convert a shotgun or 7.62mm, .30-06, or .308 rifle to the\n\
popular, accurate, and damaging .223 caliber.  The conversion results in\n\
slight reductions in both accuracy and ammo capacity."
);

//	NAME      	RAR  PRC  COLOR     MAT1   MAT2      VOL WGT DAM CUT HIT
GUNMOD("battle rifle conversion",1,680,c_magenta,STEEL,MNULL, 4,  3,  6,  0, -1,
//	ACC DAM NOI CLP REC BST NEWTYPE		PISTOL	SHOT	SMG	RIFLE
	-6,  6,  4, 20,  4,  4, AT_NULL,	false,	false,	false,	true,
	0, "\
This is a complete conversion kit, designed to turn a rifle into a powerful\n\
battle rifle.  It reduces accuracy, and increases noise and recoil, but also\n\
increases damage, ammo capacity, and fire rate."
);

GUNMOD("sniper conversion",1,660, c_green,  STEEL, MNULL,     1,  2,  0,  0, -1,
	10,  8,  3,-15,  0,-99, AT_NULL,	false,	false,	false,	true,
	0, "\
This is a complete conversion kit, designed to turn a rifle into a deadly\n\
sniper rifle.  It decreases ammo capacity, and removes any automatic fire\n\
capabilities, but also increases accuracy and damage."
);

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
#define BOOK(name,rarity,price,color,mat1,mat2,volume,wgt,melee_dam,to_hit,\
type,level,req,fun,intel,time,des) \
	index++;itypes.push_back(new it_book(index,rarity,price,name,des,'?',\
color,mat1,mat2,volume,wgt,melee_dam,0,to_hit,0,type,level,req,fun,intel,time))
//	NAME			RAR PRC	COLOR		MAT1	MAT2

BOOK("Playboy",			20,  30,c_pink,		PAPER,	MNULL,
// VOL WGT DAM HIT	TYPE		LEV REQ FUN INT TIME
    1,  1, -3,  1,	sk_null,	 0,  0,  1,  0,  10, "\
You can read it for the articles.  Or not.");

BOOK("US Weekly",		40,  40,c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	sk_speech,	 1,  0,  1,  3,  8, "\
Weekly news about a bunch of famous people who're all (un)dead now.");

BOOK("TIME magazine",		35,  40,c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	sk_null,	 0,  0,  2,  7,  10, "\
Current events concerning a bunch of people who're all (un)dead now.");

BOOK("Top Gear magazine",	40,  45,c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	sk_mechanics,	 1,  0,  1,  2,  8, "\
Lots of articles about cars and mechanics.  You might learn a little.");

BOOK("Bon Appetit",		30,  45,c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	sk_cooking,	 1,  0,  1,  5,  8, "\
Exciting recipes and restaurant reviews.  Full of handy tips about cooking.");

BOOK("Guns n Ammo",		20,  48,c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	sk_gun,		 1,  0,  1,  2,  7, "\
Reviews of firearms, and various useful tips about their use.");

BOOK("romance novel",		30,  55,c_ltblue,	PAPER,	MNULL,
    4,  1, -2,  0,	sk_null,	 0,  0,  2,  4, 15, "\
Drama and mild smut.");

BOOK("spy novel",		28,  55,c_ltblue,	PAPER,	MNULL,
    4,  1, -2,  0,	sk_null,	 0,  0,  3,  5, 18, "\
A tale of intrigue and espionage amongst Nazis, no, Commies, no, Iraqis!");

//	NAME			RAR PRC	COLOR		MAT1	MAT2
BOOK("scifi novel",		20,  55,c_ltblue,	PAPER,	MNULL,
// VOL WGT DAM HIT	TYPE		LEV REQ FUN INT TIME
    3,  1, -3,  0,	sk_null,	 0,  0,  3,  6, 20, "\
Aliens, ray guns, and space ships.");

BOOK("drama novel",		40,  55,c_ltblue,	PAPER,	MNULL,
    4,  1, -2,  0,	sk_null,	 0,  0,  4,  7, 25, "\
A real book for real adults.");

BOOK("101 Wrestling Moves",	30, 180,c_green,	PAPER,	MNULL,
    2,  1, -4,  0, 	sk_unarmed,	 3,  0,  0,  3, 15, "\
It seems to be a wrestling manual, poorly photocopied and released on spiral-\n\
bound paper.  Still, there are lots of useful tips for unarmed combat.");

BOOK("Spetsnaz Knife Techniques",12,200,c_green,	PAPER,	MNULL,
    1,  1, -5,  0,	sk_cutting,	 4,  1,  0,  4, 18, "\
A classic Soviet text on the art of attacking with a blade.");

//	NAME			RAR PRC	COLOR		MAT1	MAT2
BOOK("Under the Hood",		35, 190,c_green,	PAPER,	MNULL,
// VOL WGT DAM HIT	TYPE		LEV REQ FUN INT TIME
    3,  1, -3,  0,	sk_mechanics,	 3,  0,  0,  5, 18, "\
An advanced mechanics manual, covering all sorts of topics.");

BOOK("Self-Esteem for Dummies",	50, 160,c_green,	PAPER,	MNULL,
    3,  1, -3,  0,	sk_speech,	 3,  0,  0,  5, 20, "\
Full of useful tips for showing confidence in your speech.");

BOOK("How to Succeed in Business",40,180,c_green,	PAPER,	MNULL,
    3,  1, -3,  0,	sk_barter,	 3,  0, -1,  6, 25, "\
Useful if you want to get a good deal when purchasing goods.");

BOOK("The Big Book of First Aid",40,200,c_green,	PAPER,	MNULL,
    5,  2, -2,  0,	sk_firstaid,	 3,  0,  0,  7, 20, "\
It's big and heavy, but full of great information about first aid.");

BOOK("How to Browse the Web",	20, 170,c_green,	PAPER,	MNULL,
    3,  1, -3,  0,	sk_computer,	 2,  0,  0,  5, 15, "\
Very beginner-level information about computers.");

//	NAME			RAR PRC	COLOR		MAT1	MAT2
BOOK("Cooking on a Budget",	35, 160,c_green,	PAPER,	MNULL,
// VOL WGT DAM HIT	TYPE		LEV REQ FUN INT TIME
    4,  1, -2,  0,	sk_cooking,	 3,  0,  0,  4, 10, "\
A nice cook book that goes beyond recipes and into the chemistry of food.");

BOOK("What's a Transistor?",	20, 200,c_green,	PAPER,	MNULL,
    3,  1, -3,  0,	sk_electronics,	 3,  0,  0,  7, 20, "\
A basic manual of electronics and circuit design.");

BOOK("Sew What?  Clothing!",	15, 190,c_green,	PAPER,	MNULL,
    3,  1, -3,  0,	sk_tailor,	 3,  0,  0,  4, 18, "\
A colorful book about tailoring.");

BOOK("How to Trap Anything",	12, 240,c_green,	PAPER,	MNULL,
    2,  1, -3,  0,	sk_traps,	 4,  0,  0,  4, 20, "\
A worn manual that describes how to set and disarm a wide variety of traps.");

BOOK("Computer Science 301",	 8, 500,c_blue,		PAPER,	MNULL,
    7,  4,  5,  1,	sk_computer,	 5,  2, -2, 11, 35, "\
A college textbook on computer science.");

BOOK("Advanced Electronics",	 6, 520,c_blue,		PAPER,	MNULL,
    7,  5,  5,  1,	sk_electronics,	 5,  2, -1, 11, 35, "\
A college textbook on circuit design.");

BOOK("Advanced Economics",	12, 480,c_blue,		PAPER,	MNULL,
    7,  4,  5,  1,	sk_barter,	 5,  3, -1,  9, 30, "\
A college textbook on economics.");

//	NAME			RAR PRC	COLOR		MAT1	MAT2
BOOK("Chemistry Textbook",	11, 495,c_blue,		PAPER,	MNULL,
// VOL WGT DAM HIT	TYPE		LEV REQ FUN INT TIME
    8,  6,  5,  1,	sk_cooking,	 6,  3, -1, 12, 35, "\
A college textbook on chemistry.");

BOOK("SICP",			 3, 780,c_blue,		PAPER,	MNULL,
    6,  5,  6,  0,	sk_computer,	 8,  4, -1, 13, 50, "\
A classic text, \"The Structure and Interpretation of Computer Programs.\"\n\
Written with examples in LISP, but applicable to any language.");

BOOK("Robots for Fun & Profit",  1, 920,c_blue,		PAPER,	MNULL,
    8,  8,  8,  1,	sk_electronics,	10,  5, -1, 14, 55, "\
A rare book on the design of robots, with lots of helpful step-by-step guides."
);


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
#define CONT(name,rarity,price,color,mat1,mat2,volume,wgt,melee_dam,to_hit,\
contains,flags,des) \
	index++;itypes.push_back(new it_container(index,rarity,price,name,des,\
')',color,mat1,mat2,volume,wgt,melee_dam,0,to_hit,0,contains,flags))
//	NAME		RAR PRC	COLOR		MAT1	MAT2

CONT("plastic bag",	50,  1,	c_ltgray,	PLASTIC,MNULL,
// VOL WGT DAM HIT	VOL	FLAGS
    1,  0, -8, -4,	24,	0, "\
A small, open plastic bag.  Essentially trash.");

CONT("plastic bottle",	70,  8,	c_ltcyan,	PLASTIC,MNULL,
    2,  0, -8,  1,	 2,	mfb(con_rigid)|mfb(con_wtight)|mfb(con_seals),"\
A plastic bottle with a resealable top.  Useful for refilling with water;\n\
stand in shallow water, and press ',' or 'g' to pick some up.");

CONT("glass bottle",	70, 12,	c_cyan,		GLASS,	MNULL,
    2,  1,  8,  1,	 2,	mfb(con_rigid)|mfb(con_wtight)|mfb(con_seals),"\
A glass bottle with a resealable  top.  Useful for refilling with water; just\n\
stand in shallow water, and press ',' or 'g' to pick some up.");

CONT("aluminum can",	70,  1,	c_ltblue,	STEEL,	MNULL,
    1,  0,  0,  0,	 1,	mfb(con_rigid)|mfb(con_wtight), "\
An aluminum can, like what soda comes in.");

CONT("tin can",		65,  2,	c_blue,		IRON,	MNULL,
    1,  0, -1,  1,	 1,	mfb(con_rigid)|mfb(con_wtight), "\
A tin can, like what beans come in.");

CONT("sm. cardboard box",50, 0,	c_brown,	PAPER,	MNULL,
    4,  0, -5,  1,	 4,	mfb(con_rigid), "\
A small cardboard box.  No bigger than a foot in any dimension.");

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
#define TOOL(name,rarity,price,sym,color,mat1,mat2,volume,wgt,melee_dam,\
melee_cut,to_hit,max_charge,def_charge,charge_per_use,charge_per_sec,fuel,\
revert,func,flags,des) \
	index++;itypes.push_back(new it_tool(index,rarity,price,name,des,sym,\
color,mat1,mat2,volume,wgt,melee_dam,melee_cut,to_hit,flags,max_charge,\
def_charge,charge_per_use,charge_per_sec,fuel,revert,func))

//	NAME		RAR PRC	SYM  COLOR	MAT1	MAT
TOOL("lighter",		60,  35,',', c_blue,	PLASTIC,IRON,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    0,  0,  0,  0,  0, 100,100, 1,  0, AT_NULL,	itm_null, &iuse::lighter, 0, "\
A lighter must be carried to use various drugs, like cigarettes, or to light\n\
things like molotov cocktails.  You can also use a lighter to light nearby\n\
items on fire.");

TOOL("sewing kit",	30,120, ',', c_red,	PLASTIC,IRON,
    2,  0, -3,  0, -1,  50, 50, 1,  0, AT_NULL, itm_null, &iuse::sew, 0, "\
Use a sewing kit on an article of clothing to attempt to repair or reinforce\n\
that clothing.  This uses your tailoring skill.");

TOOL("scissors",	50,  45,',', c_ltred,	IRON,	PLASTIC,
    1,  1,  0,  8, -1,   0,  0, 0,  0, AT_NULL, itm_null, &iuse::scissors,
mfb(WF_SPEAR), "\
Use scissors to cut items made from cotton (mostly clothing) into rags.");

TOOL("hammer",		35, 70, ';', c_brown,	IRON,	WOOD,
    2,  5, 17,  0,  1,   0,  0, 0,  0, AT_NULL, itm_null, &iuse::hammer, 0, "\
Use a hammer, with nails and two by fours in your inventory, to board up\n\
adjacent doors and windows.");

//	NAME		RAR PRC	SYM  COLOR	MAT1	MAT
TOOL("fire extinguisher",20,700,';', c_red,	IRON,	MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
   16, 20, 26,  0, -1,  80, 80, 1,  0, AT_NULL, itm_null, &iuse::extinguisher,0,
"Use a fire extinguisher to put out adjacent fires.");

TOOL("flashlight (off)",40, 380,';', c_blue,	PLASTIC, IRON,
    3,  2,  1,  0,  2, 100,100, 0,  0, AT_BATT, itm_null, &iuse::light_off,0,"\
Using this flashlight will turn it on, assuming it is charged with batteries.\n\
A turned-on flashlight will provide light during the night or while\n\
underground.");

TOOL("flashlight (on)",  0, 380,';', c_blue,	PLASTIC, IRON,
    3,  2,  1,  0,  2, 100,100, 0, 15, AT_BATT,itm_flashlight,&iuse::light_on,0,
"This flashlight is turned on, and continually draining its batteries.  It\n\
provides light during the night or while underground.  Use it to turn it off.");

TOOL("hotplate",	10, 250,';', c_green,	IRON,	PLASTIC,
    5,  6,  8,  0, -1, 40, 20,  0,  0, AT_BATT, itm_null, &iuse::none,0,"\
A small heating element.  Indispensible for cooking and chemisty.");

//	NAME		RAR PRC	SYM  COLOR	MAT1	MAT
TOOL("soldering iron",	70, 200,',', c_ltblue,	IRON,	MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    3,  1,  2,  6,  0, 50, 20,  0,  0, AT_BATT, itm_null, &iuse::none,
mfb(WF_SPEAR), "\
A piece of metal that can get very hot.  Necessary for electronics crafting.");

TOOL("water purifier",   5,1200,';', c_ltblue,	PLASTIC, IRON,
   12, 20,  2,  0, -3, 100,100, 1,  0, AT_BATT,itm_null,&iuse::water_purifier,0,
"Using this item on a container full of water will purify the water.  Water\n\
taken from uncertain sources like a river may be dirty.");

TOOL("two-way radio",	10, 800,';', c_yellow,	PLASTIC, IRON,
    2,  3, 10,  0,  0, 500,500, 1,  0, AT_BATT, itm_null,&iuse::two_way_radio,0,
"Using this allows you to send out a signal; either a general SOS, or if you\n\
are in contact with a faction, to send a direct call to them.");

TOOL("radio (off)",	20, 420,';', c_yellow,	PLASTIC, IRON,
    4,  2,  4,  0, -1, 500,500, 0,  0, AT_BATT,	itm_null, &iuse::radio_off, 0,"\
Using this radio turns it on.  It will pick up any nearby signals being\n\
broadcast and play them audibly.");

//	NAME		RAR PRC	SYM  COLOR	MAT1	MAT
TOOL("radio (on)",	 0, 420,';', c_yellow,	PLASTIC, IRON,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    4,  2,  4,  0, -1, 500,500, 0,  8, AT_BATT, itm_radio,&iuse::radio_on, 0,"\
This radio is turned on, and continually draining its batteries.  It is\n\
playing the broadcast being sent from any nearby radio towers.");

TOOL("crowbar",		18, 130,';', c_ltblue,	IRON,	MNULL,
    4,  9, 16,  3,  2,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::crowbar, 0,"\
A prying tool.  Use it to open locked doors without destroying them, or to\n\
lift manhole covers.");

TOOL("hoe",		30,  90,'/', c_brown,	IRON,	WOOD,
   14, 14, 12, 10,  3,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::makemound,
	mfb(WF_STAB), "\
A farming implement.  Use it to turn tillable land into a slow-to-cross pile\n\
of dirt.");

TOOL("shovel",		40, 100,'/', c_brown,	IRON,	WOOD,
   16, 18, 15,  5,  3,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::dig, 0, "\
A digging tool.  Use it to dig pits adjacent to your location.");

TOOL("chainsaw (off)",	 7, 350,'/', c_red,	IRON,	PLASTIC,
   12, 40, 10,  0, -4,1000, 0,  0,  0, AT_GAS,	itm_null, &iuse::chainsaw_off,0,
"Using this item will, if loaded with gas, cause it to turn on, making a very\n\
powerful, but slow, unwieldy, and noisy, melee weapon.");

//	NAME		RAR VAL	SYM  COLOR	MAT1	MAT
TOOL("chainsaw (on)",	 0, 350,'/', c_red,	IRON,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
   12, 40,  4, 70, -5,1000, 0,  0,  1, AT_GAS,	itm_chainsaw_off,
	&iuse::chainsaw_on, mfb(WF_MESSY), "\
This chainsaw is on, and is continuously draining gasoline.  Use it to turn\n\
it off.");

TOOL("jackhammer",	 2, 890,';', c_magenta,	IRON,	MNULL,
   13, 54, 20,  6, -4, 120,  0,10,  0, AT_GAS,	itm_null, &iuse::jackhammer,0,"\
This jackhammer runs on gasoline.  Use it (if loaded) to blast a hole in\n\
adjacent solid terrain.");

TOOL("bubblewrap",	50,  40,';', c_ltcyan,	PLASTIC,MNULL,
    2,  0, -8,  0,  0,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::set_trap,0,"\
A sheet of plastic covered with air-filled bubbles.  Use it to set it on the\n\
ground, creating a trap that will warn you with noise when something steps on\n\
it.");

TOOL("bear trap",	 5, 120,';', c_cyan,	IRON,	MNULL,
    4, 12,  9,  1, -2,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::set_trap,0,"\
A spring-loaded pair of steel jaws.  Use it to set it on the ground, creating\n\
a trap that will ensnare and damage anything that steps on it.  If you are\n\
carrying a shovel, you will have the option of burying it.");

//	NAME		RAR VAL	SYM  COLOR	MAT1	MAT
TOOL("nailboard trap",	 0, 30, ';', c_brown,	WOOD,	MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
   18, 18, 12,  6, -3,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::set_trap,0,"\
Several pieces of wood, nailed together, with nails sticking straight up.  If\n\
an unsuspecting victim steps on it, they'll get nails through the foot.");

TOOL("tripwire trap",	 0, 35, ';', c_ltgray,	PAPER,	MNULL,
    1,  0,-10,  0, -1,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::set_trap,0,"\
A tripwire trap must be placed across a doorway or other thin passage.  Its\n\
purpose is to trip up bypassers, causing them to stumble and possibly hurt\n\
themselves minorly.");

TOOL("crossbow trap",	 0,600, ';', c_green,	IRON,	WOOD,
    7, 10,  4,  0, -2,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::set_trap,0,"\
A simple tripwire is attached to the trigger of a loaded crossbow.  When\n\
pulled, the crossbow fires.  Only a single round can be used, after which the\n\
trap is disabled.");

TOOL("shotgun trap",	 0,450, ';', c_red,	IRON,	WOOD,
    7, 11, 12,  0, -2,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::set_trap,0,"\
A simple tripwire is attached to the trigger of a loaded sawn-off shotgun.\n\
When pulled, the shotgun fires.  Two rounds are used; the first time the\n\
trigger is pulled, one or two may be used.");

//	NAME		RAR VAL	SYM  COLOR	MAT1	MAT
TOOL("blade trap",	 0,500, ';', c_ltgray,	IRON,	MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
   13, 21,  4, 16, -4,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::set_trap,0,"\
A machete is attached laterally to a motor, with a tripwire controlling its\n\
throttle.  When the tripwire is pulled, the blade is swung around with great\n\
force.  The trap forms a 3x3 area of effect.");

TOOL("land mine",	 3,2400,';', c_red,	IRON,	MNULL,
    3,  6, 10,  0, -1,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::set_trap,0,"\
An explosive that is triggered when stepped upon.  It must be partially\n\
buried to be effective, and so you will need a shovel to use it.");

TOOL("geiger ctr (off)", 8, 300,';', c_green,	PLASTIC,STEEL,
    2,  2,  2,  0,  0,100,100,  1,  0, AT_BATT,	itm_null, &iuse::geiger,0,"\
A tool for measuring radiation.  Using it will prompt you to choose whether\n\
to scan yourself or the terrain, or to turn it on, which will provide\n\
continuous feedback on ambient radiation.");

TOOL("geiger ctr (on)",	0, 300, ';', c_green,	PLASTIC,STEEL,
    2,  2,  2,  0,  0,100,100,  0, 10, AT_BATT, itm_geiger_off,&iuse::geiger,0,
"A tool for measuring radiation.  It is in continuous scan mode, and will\n\
produce quiet clicking sounds in the presence of ambient radiation. Using it\n\
allows you to turn it off, or scan yourself or the ground.");

//	NAME		RAR VAL	SYM  COLOR	MAT1	MAT
TOOL("teleporter",      10,6000,';', c_magenta,	PLASTIC,STEEL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    3, 12,  4,  0, -1, 20, 20,  1,  0, AT_PLUT,	itm_null, &iuse::teleport,0,"\
An arcane device, powered by plutonium fuel cells.  Using it will cause you\n\
to teleport a short distance away.");

TOOL("goo canister",     8,3500,';', c_dkgray,  STEEL,	MNULL,
    6, 22,  7,  0,  1,  1,  1,  1,  0, AT_NULL,	itm_null, &iuse::can_goo,0,"\
\"Warning: contains highly toxic and corrosive materials.  Contents may be\n\
 sentient.  Open at your own risk.\"");

TOOL("pipe bomb",	 4, 150,'*', c_white,	IRON,	MNULL,
    2,  3, 11,  0,  1,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::pipebomb,0,"\
A section of a pipe filled with explosive materials.  Use this item to light\n\
the fuse, which gives you 3 turns before it detonates.  You will need a\n\
lighter.  It is somewhat unreliable, and may fail to detonate.");

TOOL("active pipe bomb", 0,   0,'*', c_white,	IRON,	MNULL,
    2,  3, 11,  0,  1,  3,  3,  0,  1, AT_NULL,	itm_null, &iuse::pipebomb_act,0,
"This pipe bomb's fuse is lit, and it will explode any second now.  Throw it\n\
immediately!");

//	NAME		RAR VAL	SYM  COLOR	MAT1	MAT
TOOL("grenade",		 3, 400,'*', c_green,	IRON,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    1,  1, 10,  0, -1,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::grenade,0,"\
Use this item to pull the pin, turning it into an active grenade.  You will\n\
then have five turns before it explodes; throwing it would be a good idea.");

TOOL("active grenade",	 0,   0,'*', c_green,	IRON,	PLASTIC,
    1,  1, 10,  0, -1,  5,  5,  0,  1, AT_NULL, itm_null, &iuse::grenade_act,0,
"This grenade is active, and will explode any second now.  Better throw it!");

TOOL("EMP grenade",	 2, 600,'*', c_cyan,	STEEL,	PLASTIC,
    1,  1,  8,  0, -1,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::EMPbomb,0,"\
Use this item to pull the pin, turning it into an active EMP grenade.  You\n\
will then have three turns before it detonates, creating an EMP field which\n\
damages robots and drains bionic energy.");

TOOL("active EMP grenade",0,  0,'*', c_cyan,	STEEL,	PLASTIC,
    1,  1,  8,  0, -1,  3,  3,  0,  1, AT_NULL,	itm_null, &iuse::EMPbomb_act,0,
"This EMP grenade is active, and wiill shortly detonate, creating a large EMP\n\
field which damages robots and drains bionic energy.");

TOOL("teargas canister",3,  600,'*', c_yellow,	STEEL, MNULL,
    1,  1,  6,  0, -1,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::gasbomb,0,"\
Use this item to pull the pin.  Five turns after you do that, it will begin\n\
to expell a highly toxic gas for several turns.  This gas damages and slows\n\
those who enter it, as well as obscuring vision and scent.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("active teargas",	0,    0,'*', c_yellow,	STEEL, MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    1,  1,  6,  0, -1,  5,  5,  0,  1, AT_NULL, itm_null, &iuse::gasbomb_act,0,
"This canister of teargas has had its pin removed, indicating that it is (or\n\
will shortly be) expelling highly toxic gas.");

TOOL("smoke bomb",	5,  180,'*', c_dkgray,	STEEL,	MNULL,
    1,  1,  5,  0, -1,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::smokebomb,0,"\
Use this item to pull the pin.  Five turns after you do that, it will begin\n\
to expell a thick black smoke.  This smoke will slow those who enter it, as\n\
well as obscuring vision and scent.");

TOOL("active smoke bomb",0,  0, '*', c_dkgray,	STEEL,	MNULL,
    1,  1,  5,  0, -1,  0,  0,  0,  1, AT_NULL, itm_null,&iuse::smokebomb_act,0,
"This smoke bomb has had its pin removed, indicating that it is (or will\n\
shortly be) expelling thick smoke.");

TOOL("molotov cocktail",0,  200,'*', c_ltred,	GLASS,	COTTON,
    2,  2,  8,  0,  1,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::molotov,0,"\
A bottle of flammable liquid with a rag inserted.  Use this item to light the\n\
rag; you will, of course, need a lighter in your inventory to do this.  After\n\
lighting it, throw it to cause fires.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("molotov cocktail (lit)",0,0,'*', c_ltred,	GLASS,	COTTON,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    2,  2,  8,  0,  1,  1,  1,  0,  0, AT_NULL,	itm_null, &iuse::molotov_lit,0,
"A bottle of flammable liquid with a flaming rag inserted.  Throwing it will\n\
cause the bottle to break, spreading fire.  The flame may go out shortly if\n\
you do not throw it.  Dropping it while lit is not safe.");

TOOL("dynamite",	5,  700,'*', c_red,	PLASTIC,MNULL,
    6, 10,  4,  0, -3,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::dynamite,0,"\
Several sticks of explosives with a fuse attached.  Use this item to light\n\
the fuse; you will, of course, need a lighter in your inventory to do this.\n\
Shortly after lighting the fuse, this item will explode, so get away!");

TOOL("dynamite (lit)",	5,    0,'*', c_red,	PLASTIC,MNULL,
    6, 10,  4,  0, -3,  0,  0,  0,  1, AT_NULL,	itm_null, &iuse::dynamite_act,0,
"The fuse on this dynamite is lit and hissing.  It'll explode any moment now.");

TOOL("mininuke",	1, 1800,'*', c_ltgreen,	STEEL,	PLASTIC,
    3,  4,  8,  0, -2,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::mininuke,0,"\
An extremely powerful weapon--essentially a hand-held nuclear bomb.  Use it\n\
to activate the timer.  Ten turns later it will explode, leaving behind a\n\
radioactive crater.  The explosion is large enough to take out a house.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("mininuke (active)",0,   0,'*', c_ltgreen,	STEEL,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    3,  4,  8,  0, -2,  0,  0,  0,  1, AT_NULL, itm_null, &iuse::mininuke_act,0,
"This miniature nuclear bomb has a light blinking on the side, showing that\n\
it will soon explode.  You should probably get far away from it.");

TOOL("zombie pheromone",1,  400,'*', c_yellow,	FLESH,	PLASTIC,
    1,  1, -5,  0, -1,  3,  3,  1,  0, AT_NULL,	itm_null, &iuse::pheromone,0,"\
This is some kind of disgusting ball of rotting meat.  Squeezing it causes a\n\
small cloud of pheromones to spray into the air, causing nearby zombies to\n\
become friendly for a short period of time.");

TOOL("portal generator",2, 6600, ';', c_magenta, STEEL,	PLASTIC,
    2, 10,  6,  0, -1,  5,  5,  1,  0, AT_NULL,	itm_null, &iuse::portal,0,"\
A rare and arcane device, covered in alien markings.");

TOOL("inactive manhack",1, 1200, ',', c_ltgreen, STEEL, PLASTIC,
    1,  3,  6,  6, -3,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::manhack,0,"\
An inactive manhack.  Manhacks are fist-sized robots which fly through the\n\
air.  They are covered with whirring blades and attack by throwing themselves\n\
against their target.  Use this item to activate the manhack.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("inactive turret",  1,2000,';',c_ltgreen,	STEEL,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    12, 12, 8,  0, -3,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::turret,0,"\
An inactive turret.  Using this item involves turning it on and placing it\n\
on the ground, where it will attach itself.  The turret will then identify\n\
you as a friendly, and attack all enemies with an SMG.");

TOOL("UPS (off)",	 1,2800,';',c_ltgreen,	STEEL,	PLASTIC,
    4,  6, 10,  0, -1,1000, 0,  0,  0, AT_BATT, itm_null, &iuse::UPS_off,0,"\
A unified power supply, or UPS, is a device developed jointly by military and\n\
scientific interests for use in combat and the field.  The UPS is designed to\n\
power armor, goggles, etc., but drains batteries quickly.");

TOOL("UPS (on)",	 0,2800,';',c_ltgreen,	STEEL,	PLASTIC,
    4,  6, 10,  0, -1,1000, 0,  0,  2, AT_BATT,	itm_UPS_off, &iuse::UPS_on,0,"\
A unified power supply, or UPS, is a device developed jointly by military and\n\
scientific interests for use in combat and the field.  The UPS is designed to\n\
power armor, goggles, etc., but drains batteries quickly.");

TOOL("tazer",		 3,1400,';',c_ltred,	IRON,	PLASTIC,
    1,  3,  6,  0, -1, 500, 0,100, 0, AT_BATT, itm_null, &iuse::tazer,0,"\
A high-powered stun gun.  Use this item to attempt to electrocute an adjacent\n\
enemy, damaging and temporarily paralyzing them.  Because the shock can\n\
actually jump through the air, it is difficult to miss.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("mp3 player (off)",18, 800,';',c_ltblue,	IRON,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    1,  1,  0,  0,  0, 100,100, 0,  0, AT_BATT, itm_null, &iuse::mp3,0,"\
This battery-devouring device is loaded up with someone's music collection.\n\
Fortunately, there's lots of songs you like, and listening to it will raise\n\
your morale slightly.  Use it to turn it on.");

TOOL("mp3 player (on)",	 0, 800,';',c_ltblue,	IRON,	PLASTIC,
    1,  1,  0,  0,  0, 100,100, 0,  1, AT_BATT, itm_mp3, &iuse::mp3_on,0,"\
This mp3 player is turned on and playing some great tunes, raising your\n\
morale steadily while on your person.  It runs through batteries quickly; you\n\
can turn it off by using it.  It also obscures your hearing.");


// BIONICS
// These are the modules used to install new bionics in the player.  They're
// very simple and straightforward; a difficulty, followed by a NULL-terminated
// list of options.
#define BIO(name, rarity, price, color, difficulty, des, ...) \
	index++;itypes.push_back(new it_bionic(index,rarity,price,name,des,':',\
color, STEEL, PLASTIC, 10, 18, 8, 0, 0, 0, difficulty, __VA_ARGS__))
//  Name			RAR PRICE	COLOR		DIFFICULTY

BIO("CBM: Internal Battery",	24, 3800,	c_green,	 1, "\
Compact Bionics Module which upgrades your power capacity by 10 units. Having\n\
at least one of these is a prerequisite to using powered bionics.  You will\n\
also need a power supply, found in another CBM.",
    NULL); // This is a special case, which increases power capacity by 10

BIO("CBM: Power Sources",	18, 5000,	c_yellow,	 4, "\
Compact Bionics Module containing the materials necessary to install any one\n\
of several power sources.  Having a power source is necessary to use powered\n\
bionics.",
    bio_batteries, bio_metabolics, bio_solar, bio_furnace, bio_ethanol, NULL);

BIO("CBM: Utilities",		20, 2000,	c_ltgray,	 2, "\
Compact Bionics Module containing a variety of utilities.  Popular among\n\
civilians, especially specialist laborers like mechanics.",
    bio_tools, bio_storage, bio_flashlight, bio_lighter, bio_magnet, NULL);

BIO("CBM: Neurological",	 8, 6000,	c_pink,		 8, "\
Compact Bionics Module containing a few upgrades to one's central nervous\n\
system or brain.  Due to the difficulty associated with what is essentially\n\
brain surgery, these are best installed by a highly skill professional.",
    bio_memory, bio_painkiller, bio_alarm, NULL);

BIO("CBM: Sensory",		10, 4500,	c_ltblue,	 5, "\
Compact Bionics Module containing a few upgrades to one's sensory systems,\n\
particularly sight.  Fairly difficult to install.",
    bio_ears, bio_eye_enhancer, bio_night_vision, bio_infrared, NULL);

BIO("CBM: Aquatic",		 5, 3000,	c_blue,		 3, "\
Compact Bionics Module with a couple of upgrades designed with those who are\n\
often underwater; popular among diving enthusiasts and Navy SEAL teams.",
    bio_membrane, bio_gills, NULL);

BIO("CBM: Combat Augs",		10, 4500,	c_red,		 3, "\
Compact Bionics Module containing several augmentations designed to aid in\n\
combat.  While none of these are weapons, all are very useful for improving\n\
one's battle awareness.",
    bio_targeting, bio_night_vision, bio_infrared, bio_ground_sonar, NULL);

BIO("CBM: Hazmat",		12, 4800,	c_ltgreen,	 3, "\
Compact Bionics Module that allows you to install various augmentations\n\
designed to protect the user in the event of exposure to hazardous materials.",
    bio_purifier, bio_climate, bio_heatsink, bio_blood_filter, NULL);

BIO("CBM: Nutritional",		 7, 3200,	c_green,	 4, "\
Compact Bionics Module with several upgrades to one's digestive system, aimed\n\
at making the consumption of food a lower priority.",
    bio_recycler, bio_digestion, bio_evap, bio_water_extractor, NULL);

BIO("CBM: Desert Survival",	 4, 4000,	c_brown,	 3, "\
Compact Bionics Module designed for those who will spend  significant time in\n\
dry, hot areas like a desert.  Geared towards providing a source of water.",
    bio_climate, bio_recycler, bio_evap, bio_water_extractor, NULL);

BIO("CBM: Melee Combat",	 6, 5800,	c_red,		 3, "\
Compact Bionics Module containing a few upgrades designed for melee combat.\n\
Useful for those who like up-close combat.",
    bio_shock, bio_heat_absorb, bio_claws, NULL);

BIO("CBM: Armor",		12, 6800,	c_cyan,		 5, "\
Compact Bionics Module containing the supplies necessary to install one of\n\
several high-strength armors.  Very popular.",
    bio_carbon, bio_armor_head, bio_armor_torso, bio_armor_arms, bio_armor_legs,
    NULL);

BIO("CBM: Espionage",		 5, 7500,	c_magenta,	 4, "\
Compact Bionics Module often used by high-tech spies.  Its contents are\n\
geared towards avoiding detection and bypassing security.",
    bio_face_mask, bio_scent_mask, bio_cloak, bio_alarm, bio_fingerhack,
    bio_lockpick, NULL);

BIO("CBM: Defensive Systems",	 9, 8000,	c_ltblue,	 5, "\
Compact Bionics Module containing a few augmentations designed to defend the\n\
user in case of attack.",
    bio_carbon, bio_ads, bio_ods, NULL);

BIO("CBM: Medical",		12, 8200,	c_ltred,	 6, "\
Compact Bionics Module containing several upgrades designed to provide the\n\
user with medical attention in the field.",
    bio_painkiller, bio_nanobots, bio_blood_anal, bio_blood_filter, NULL);

BIO("CBM: Construction",	10, 3500,	c_dkgray,	 3, "\
Compact Bionics Module which is very popular among construction workers.  It\n\
contains several upgrades designed to make the user a living tool.",
    bio_tools, bio_resonator, bio_hydraulics, bio_magnet, NULL);

BIO("CBM: Super-Soldier",	 1,14000,	c_white,	10, "\
A very rare Compact Bionics Module, designed by the military to create a kind\n\
of super-soldier.  Due to the highly advanced technology used, this module is\n\
very difficult to install.",
    bio_time_freeze, bio_teleport, NULL);

BIO("CBM: Ranged Combat",	 7, 9200,	c_red,		 6, "\
Compact Bionics Module containing a few devices designed for ranged combat.\n\
Good for those who want a gun on occasion, but do not wish to carry lots of\n\
heavy ammunition and weapons.",
    bio_blaster, bio_laser, bio_emp, NULL);

#define MACGUFFIN(name, price, sym, color, mat1, mat2, volume, wgt, dam, cut,\
                  to_hit, readable, function, description) \
index++; itypes.push_back(new it_macguffin(index, 0, price, name, description,\
	sym, color, mat1, mat2, volume, wgt, dam, cut, to_hit, 0, readable,\
	function))
MACGUFFIN("paper note", 0, '?', c_white, PAPER, MNULL, 1, 0, 0, 0, 0,
	true, &iuse::mcg_note, "\
A hand-written paper note.");

 if (itypes.size() > num_items)
  debugmsg("%d items, %d itypes", itypes.size(), num_all_items);


MELEE("Null 2 - num_items",0,0,'#',c_white,MNULL,MNULL,0,0,0,0,0,0,"");

// BIONIC IMPLANTS
// Sometimes a bionic needs to set you up with a dummy weapon, or something
// similar.  For the sake of clarity, no matter what the type of item, place
// them all here.

//    NAME		RARE SYM COLOR		MAT1	MAT2
MELEE("adamantite claws",0,0,'{', c_pink,	STEEL,	MNULL,
//	VOL WGT DAM CUT HIT
	 2,  0,  8, 16,  4, mfb(WF_STAB), "\
Short and sharp claws made from a high-tech metal.");

//  NAME		RARE  TYPE	COLOR		MAT
AMMO("Fusion blast",	 0,0, AT_FUSION,c_dkgray,	MNULL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 0,  0, 40,  0, 10,  1,  0,  5, "", mfb(WF_AMMO_INCENDIARY));
//  NAME		RARE	COLOR		MAT1	MAT2

GUN("fusion blaster",	 0,0,c_magenta,	STEEL,	PLASTIC,
//	SKILL		AMMO	   VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_rifle,	AT_FUSION, 12,  0,  0,  0,  0,  4,  0, 10,  0,  1, "",0);
 if (itypes.size() != num_all_items)
  debugmsg("%d items, %d itypes (+bio)", itypes.size(), num_all_items - 1);

}

std::string ammo_name(ammotype t)
{
 switch (t) {
  case AT_NAIL:   return "nails";
  case AT_BB:	  return "BBs";
  case AT_BOLT:	  return "bolts";
  case AT_SHOT:	  return "shot";
  case AT_22:	  return ".22";
  case AT_9MM:	  return "9mm";
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
  case AT_GAS:	  return "gasoline";
  case AT_BATT:   return "batteries";
  case AT_PLUT:   return "plutonium";
  case AT_FUSION: return "fusion cell";
  default:	  return "XXX";
 }
}

itype_id default_ammo(ammotype guntype)
{
 switch (guntype) {
 case AT_NAIL:	return itm_nail;
 case AT_BB:	return itm_bb;
 case AT_BOLT:	return itm_bolt_wood;
 case AT_SHOT:	return itm_shot_00;
 case AT_22:	return itm_22_lr;
 case AT_9MM:	return itm_9mm;
 case AT_38:	return itm_38_special;
 case AT_40:	return itm_10mm;
 case AT_44:	return itm_44magnum;
 case AT_45:	return itm_45_acp;
 case AT_57:	return itm_57mm;
 case AT_46:	return itm_46mm;
 case AT_762:	return itm_762_m43;
 case AT_223:	return itm_223;
 case AT_308:	return itm_308;
 case AT_3006:	return itm_270;
 case AT_BATT:	return itm_battery;
 case AT_FUSION:return itm_laser_pack;
 case AT_PLUT:	return itm_plut_cell;
 case AT_GAS:	return itm_gasoline;
 }
 return itm_null;
}

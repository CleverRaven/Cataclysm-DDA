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
#define TECH(t) itypes[index]->techniques = t

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
// Corpse - a special item
 itypes.push_back(
  new itype(1, 0, 0, "corpse", "A dead body.", '%', c_white, MNULL, MNULL, 0, 0,
            0, 0, 1, 0));
// Fire - only appears in crafting recipes
 itypes.push_back(
  new itype(2, 0, 0, "nearby fire",
            "Some fire - if you are reading this it's a bug!",
            '$', c_red, MNULL, MNULL, 0, 0, 0, 0, 0, 0));
// Integrated toolset - ditto
 itypes.push_back(
  new itype(3, 0, 0, "integrated toolset",
            "A fake item. If you are reading this it's a bug!",
            '$', c_red, MNULL, MNULL, 0, 0, 0, 0, 0, 0));
// For smoking crack or meth
 itypes.push_back(
  new itype(4, 0, 0, "something to smoke that from, and a lighter",
            "A fake item. If you are reading this it's a bug!",
            '$', c_red, MNULL, MNULL, 0, 0, 0, 0, 0, 0));
 int index = 4;

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
	25,  0,  0,  0,  0,  0,  1,  0,&iuse::none,	ADD_NULL, "\
Water, the stuff of life, the best thirst-quencher available.");

//     NAME		RAR PRC	COLOR     CONTAINER
DRINK("clean water",	90, 50,	c_ltcyan, itm_bottle_plastic,
//	QUE NUT SPO STM HTH ADD CHG FUN use_func	addiction type
	25,  0,  0,  0,  0,  0,  1,  0,&iuse::none,	ADD_NULL, "\
Fresh clean water, truly the best thing to quench your thirst.");

DRINK("sewage sample",	 5,  5, c_ltgreen, itm_bottle_plastic,
	 5,  0,  0,  0,-10,  0,  1,-20,&iuse::sewage,	ADD_NULL, "\
A sample of sewage from a treatment plant. Gross.");

DRINK("salt water",	20,  5,	c_ltcyan, itm_bottle_plastic,
	-30, 0,  0,  0,  1,  0,  1, -1,&iuse::none,	ADD_NULL, "\
Water with salt added. Not good for drinking.");

DRINK("orange juice",	50, 38,	c_yellow, itm_bottle_plastic,
	35,  4,120,  0,  2,  0,  1,  3,&iuse::none,	ADD_NULL, "\
Fresh squeezed from real oranges! Tasty and nutritious.");

DRINK("apple cider",	50, 38, c_brown,  itm_bottle_plastic,
	35,  6,144,  0,  3,  0,  1,  2,&iuse::none,	ADD_NULL, "\
Pressed from fresh apples. Tasty and nutritious.");

DRINK("energy drink",	55, 45,	c_magenta,itm_can_drink,
	15,  1,  0,  8, -2,  2,  1,  5,&iuse::caff,	ADD_CAFFEINE, "\
Popular among those who need to stay up late working.");

//     NAME		RAR PRC	COLOR     CONTAINER
DRINK("cola",		70, 35,	c_brown,  itm_can_drink,
//	QUE NUT SPO STM HTH ADD CHG FUN use_func	addiction type
	18,  3,  0,  6, -1,  2,  1,  5,&iuse::caff,	ADD_CAFFEINE, "\
Things go better with cola. Sugar water with caffeine added.");

DRINK("root beer",	65, 30,	c_brown,  itm_can_drink,
	18,  3,  0,  1, -1,  0,  1,  3,&iuse::none,	ADD_NULL, "\
Like cola, but without caffeine. Still not that healthy.");

DRINK("milk",		50, 35,	c_white,  itm_bottle_glass,
	25,  8,  8,  0,  1,  0,  1,  0,&iuse::none,	ADD_NULL, "\
Baby cow food, appropriated for adult humans. Spoils rapidly.");

DRINK("V8",		15, 35,	c_red,    itm_can_drink,
	 6, 28,240,  0,  1,  0,  1,  0,&iuse::none,	ADD_NULL, "\
Contains up to 8 vegetables! Nutritious and tasty.");

//     NAME		RAR PRC	COLOR     CONTAINER
DRINK("broth",		15, 35, c_yellow, itm_can_food,
//	QUE NUT SPO STM HTH ADD CHG FUN use_func	addiction type
	10, 15,160,  0,  0,  0,  1,  1,&iuse::none,	ADD_NULL, "\
Vegetable stock. Tasty and fairly nutritious.");

DRINK("soup",		15, 60, c_red,    itm_can_food,
	10, 60,120,  0,  2,  0,  1,  2,&iuse::none,	ADD_NULL, "\
A nutritious and delicious hearty vegetable soup.");

DRINK("whiskey",	16, 85,	c_brown,  itm_bottle_glass,
	-12, 4,  0,-12, -2,  5, 7, 15,&iuse::alcohol,	ADD_ALCOHOL, "\
Made from, by, and for real Southern colonels!");

//     NAME		RAR PRC	COLOR     CONTAINER
DRINK("vodka",		20, 78,	c_ltcyan, itm_bottle_glass,
//	QUE NUT SPO STM HTH ADD CHG FUN use_func	addiction type
	-10, 2,  0,-12, -2,  5, 7, 15,&iuse::alcohol,	ADD_ALCOHOL, "\
In Soviet Russia, vodka drinks you!");

DRINK("gin",		20, 78,	c_ltcyan, itm_bottle_glass,
	-10, 2,  0,-12, -2,  5, 7, 15,&iuse::alcohol,	ADD_ALCOHOL, "\
Smells faintly of elderberries, but mostly booze.");

DRINK("rum",		14, 85,	c_ltcyan, itm_bottle_glass,
	-12, 2,  0,-10, -2,  5, 7, 15,&iuse::alcohol,	ADD_ALCOHOL, "\
Drinking this might make you feel like a pirate. Or not.");

DRINK("tequila",	12, 88,	c_brown,  itm_bottle_glass,
	-12, 2,  0,-12, -2,  6, 7, 18,&iuse::alcohol,	ADD_ALCOHOL, "\
Don't eat the worm! Wait, there's no worm in this bottle.");

DRINK("triple sec",	12, 55,	c_brown,  itm_bottle_glass,
	-8, 2,  0,-10, -2,  4, 7, 10,&iuse::alcohol,	ADD_ALCOHOL, "\
An orange flavored liquor used in many mixed drinks.");

DRINK("long island iced tea",	8, 100,	c_brown,  itm_bottle_glass,
	-10, 2,  0,-10, -2,  5, 6, 20,&iuse::alcohol,	ADD_ALCOHOL, "\
A blend of incredibly strong-flavored liquors that somehow tastes\n\
like none of them.");

DRINK("beer",           60, 35, c_brown,  itm_can_drink,
         16, 4,  0, -4, -1,  2,  1, 10, &iuse::alcohol,   ADD_ALCOHOL, "\
Best served cold, in a glass, and with a lime - but you're not that lucky.");

DRINK("bleach",		20, 18,	c_white,  itm_carboy_plastic,
	-96, 0,  0,  0, -8,  0,  1,-30,&iuse::blech,	ADD_NULL, "\
Don't drink it. Mixing it with ammonia produces toxic gas.");

//     NAME		RAR PRC	COLOR     CONTAINER
DRINK("ammonia",	24, 30,	c_yellow, itm_carboy_plastic,
//	QUE NUT SPO STM HTH ADD CHG FUN use_func	addiction type
	-96, 0,  0,  0, -2,  0,  1,-30,&iuse::blech,	ADD_NULL, "\
Don't drink it. Mixing it with bleach produces toxic gas.");

DRINK("mutagen",	 8,1000,c_magenta,itm_flask_glass,
	  0, 0,  0,  0, -2,  0,  1,  0,&iuse::mutagen_3,ADD_NULL, "\
A rare substance of uncertain origins. Causes you to mutate.");

DRINK("purifier",	12, 6000,c_pink,  itm_flask_glass,
	  0, 0,  0,  0,  1,  0,  1,  0,&iuse::purifier,	ADD_NULL, "\
A rare stem-cell treatment, which causes mutations and other genetic defects\n\
to fade away.");

DRINK("tea",		1, 50,	c_green, itm_bottle_plastic,
//	QUE NUT SPO STM HTH ADD CHG FUN use_func	addiction type
	40,  3,  0,  0,  0,  0,  1, 6,&iuse::none,	ADD_NULL, "\
Tea, the beverage of gentlemen everywhere.");

DRINK("coffee",		1, 50,	c_brown, itm_bottle_plastic,
	40,  3,  0,  12,  0,  0,  1, 6,&iuse::caff,	ADD_CAFFEINE, "\
Coffee. The morning ritual of the pre-apocalypse world.");

//     NAME		RAR PRC	COLOR     CONTAINER
DRINK("blood",		 20,  0, c_red, itm_flask_glass,
//	QUE NUT SPO STM HTH ADD CHG FUN use_func	addiction type
	  5,  5,  0,  0, -8,  0,  1,-50,&iuse::none,	ADD_NULL, "\
Blood, possibly that of a human. Disgusting!");

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
Freshly butchered meat. You could eat it raw, but cooking it is better.");

FOOD("chunk of veggy",	30, 60,	c_green,	VEGGY,	itm_null,
    1,  2,  0, 20, 80,  0,  1,  0,  1,  0,	&iuse::none, ADD_NULL, "\
A raw chunk of vegetable. Fine for eating raw, tastier when cooked.");

FOOD("tainted meat",	60,  4,	c_red,		FLESH,	itm_null,
    1,  2,  0, 20,  4,  0,  0,  0,  1,-10,	&iuse::poison, ADD_NULL, "\
Meat that's obviously unhealthy. You could eat it, but it will poison you.");

FOOD("tainted veggy",	35,  5,	c_green,	VEGGY,	itm_null,
    1,  2,  0, 20, 10,  0,  1,  0,  1,  0,	&iuse::poison, ADD_NULL, "\
Vegetable that looks poisonous. You could eat it, but it will poison you.");

FOOD("cooked meat",	 0, 75, c_red,		FLESH,	itm_null,
    1,  2,  0, 50, 24,  0,  0,  0,  1,  8,	&iuse::none,	ADD_NULL, "\
Freshly cooked meat. Very nutritious.");

FOOD("cooked veggy",	 0, 70, c_green,	VEGGY,	itm_null,
    1,  2,  0, 40, 50,  0,  1,  0,  1,  0,	&iuse::none,	ADD_NULL, "\
Freshly cooked vegetables. Very nutritious.");

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("apple",		70, 16,	c_red,		VEGGY,  itm_null,
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func    addiction type
    1,  1,  3, 16,160,  0,  4,  0,  1,  3,	&iuse::none, ADD_NULL, "\
An apple a day keeps the doctor away.");

FOOD("orange",		65, 18,	c_yellow,	VEGGY,	itm_null,
    1,  1,  8, 14, 96,  0,  0,  0,  1,  3,	&iuse::none, ADD_NULL, "\
Sweet citrus fruit. Also comes in juice form.");

FOOD("lemon",		50, 12, c_yellow,	VEGGY,	itm_null,
    1,  1,  5,  5,180,  0,  0,  0,  1, -4,	&iuse::none, ADD_NULL, "\
Very sour citrus. Can be eaten if you really want.");

FOOD("potato chips",	65, 12,	c_magenta,	VEGGY,	itm_bag_plastic,
    2,  1, -2,  8,  0,  0,  0,  0,  3,  6,	&iuse::none, ADD_NULL, "\
Betcha can't eat just one.");

FOOD("potato chips",	65, 12,	c_magenta,	VEGGY,	itm_bag_plastic,
    2,  1, -2,  8,  0,  0,  0,  0,  3,  3,	&iuse::none, ADD_NULL, "\
A bag of cheap, own-brand chips.");

FOOD("potato chips",	65, 12,	c_magenta,	VEGGY,	itm_bag_plastic,
    2,  1, -2,  8,  0,  0,  0,  0,  3,  12,	&iuse::none, ADD_NULL, "\
Oh man, you love these chips! Score!");

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
Mushrooms are tasty, but be careful. Some can poison you, while others are\n\
hallucinogenic.");

FOOD("mushroom",	 3, 14,	c_brown,	VEGGY,	itm_null,
    1,  0,  0, 10,  0,  0,  0,  0,  1,  0,	&iuse::poison, ADD_NULL, "\
Mushrooms are tasty, but be careful. Some can poison you, while others are\n\
hallucinogenic.");

FOOD("mushroom",	 1, 14,	c_brown,	VEGGY,	itm_null,
    1,  0,  0, 10,  0, -4,  0,  0,  1,  6,	&iuse::hallu, ADD_NULL, "\
Mushrooms are tasty, but be careful. Some can poison you, while others are\n\
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
Juicy red tomato. It gained popularity in Italy after being brought back\n\
from the New World.");

FOOD("broccoli",	 9, 40,	c_green,	VEGGY,	itm_null,
    2,  2,  0, 30,160,  0,  1,  0,  1,  0,	&iuse::none, ADD_NULL, "\
It's a bit tough, but quite delicious.");

FOOD("zucchini",	 7, 30,	c_ltgreen,	VEGGY,	itm_null,
    2,  1,  0, 20,120,  0,  1,  0,  1,  0,	&iuse::none, ADD_NULL, "\
A tasty summer squash.");

FOOD("frozen dinner",	50, 80,	c_magenta,	FLESH,	itm_box_small,
    5,  4, -2, 60, 60,  0, -2,  0,  1, -3,	&iuse::none, ADD_NULL, "\
Now with ONE POUND of meat and ONE POUND of carbs! Not as appetizing or\n\
nutritious as it would be if heated up.");

FOOD("cooked TV dinner", 0,  0, c_magenta,	FLESH,  itm_null,
    5,  4, -2, 72, 12,  0, -2,  0,  1,  5,	&iuse::none,	ADD_NULL, "\
Now with ONE POUND of meat and ONE POUND of carbs! Nice and heated up. It's\n\
tastier and more filling, but will also spoil quickly.");

FOOD("raw spaghetti",	40, 12,	c_yellow,	VEGGY,	itm_box_small,
    6,  2,  0,  6,  0,  0,  0,  0,  1, -8,	&iuse::none, ADD_NULL, "\
It could be eaten raw if you're desperate, but is much better cooked.");

FOOD("cooked spaghetti", 0, 28,	c_yellow,	VEGGY,	itm_box_small,
   10,  3,  0, 60, 20,  0,  0,  0,  1,  2,	&iuse::none, ADD_NULL, "\
Fresh wet noodles. Very tasty.");

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
Meat encased in little dough satchels. Tastes fine raw.");

FOOD("red sauce",	20, 24,	c_red,		VEGGY,	itm_can_food,
    2,  3,  0, 20,  0,  0,  0,  0,  1,  1,	&iuse::none, ADD_NULL, "\
Tomato sauce, yum yum.");

FOOD("pesto",		15, 20,	c_ltgreen,	VEGGY,	itm_can_food,
    2,  3,  0, 18,  0,  0,  1,  0,  1,  4,	&iuse::none, ADD_NULL, "\
Olive oil, basil, garlic, pine nuts. Simple and delicious.");

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("beans",		40, 55,	c_cyan,		VEGGY,	itm_can_food,
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func    addiction type
    1,  2,  0, 40,  0,  0,  0,  0,  1,  0,	&iuse::none, ADD_NULL, "\
Canned beans. A staple for hobos.");

FOOD("corn",		35, 40,	c_cyan,		VEGGY,	itm_can_food,
    1,  2,  5, 30,  0,  0,  0,  0,  1, -2,	&iuse::none, ADD_NULL, "\
Canned corn in water. Eat up!");

FOOD("SPAM",		30, 50,	c_cyan,		FLESH,	itm_can_food,
    1,  2, -3, 48,  0,  0,  0,  0,  1, -8,	&iuse::none, ADD_NULL, "\
Yuck, not very tasty, but it is quite filling.");

FOOD("pineapple",	30, 50,	c_cyan,		VEGGY,	itm_can_food,
    1,  2,  5, 26,  0,  0,  1,  0,  1,  7,	&iuse::none, ADD_NULL, "\
Canned pineapple rings in water. Quite tasty.");

FOOD("coconut milk",	10, 45,	c_cyan,		VEGGY,	itm_can_food,
    1,  2,  5, 30,  0,  0,  0,  0,  1,  0,	&iuse::none, ADD_NULL, "\
A dense, sweet creamy sauce, often used in curries.");

FOOD("sardines",	14, 25,	c_cyan,		FLESH,	itm_can_food,
    1,  1, -8, 14,  0,  0,  0,  0,  1,  0,	&iuse::none, ADD_NULL, "\
Salty little fish. They'll make you thirsty.");

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("tuna fish",	35, 35,	c_cyan,		FLESH,	itm_can_food,
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func    addiction type
    1,  2,  0, 24,  0,  0,  0,  0,  1,  0,	&iuse::none, ADD_NULL, "\
Now with 95 percent less dolphins!");

FOOD("cat food",	20,  8,	c_cyan,		FLESH,	itm_can_food,
    1,  2,  0, 10,  0,  0, -1,  0,  1,-24,	&iuse::none, ADD_NULL, "\
Blech, so gross. Save it for when you're about to die of starvation.");

FOOD("honey comb",	10, 35,	c_yellow,	VEGGY,	itm_null,
    1,  1,  0, 20,  0,  0, -2,  0,  1,  9,	&iuse::honeycomb, ADD_NULL, "\
A large chunk of wax, filled with honey. Very tasty.");

FOOD("wax",     	10, 35,	c_yellow,	VEGGY,	itm_null,
    1,  1,  0, 4,  0,  0, -2,  0,  1,  -5,	&iuse::none, ADD_NULL, "\
A large chunk of beeswax.\n\
Not very tasty or nourishing, but ok in an emergency.");

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
Eating this would be pretty gross. It causes you to mutate.");

FOOD("leg",		 4,250,	c_brown,	FLESH,	itm_null,
   12, 24,  0, 16,  0,  0, -8,  0,  1, -20,	&iuse::mutagen, ADD_NULL, "\
Eating this would be pretty gross. It causes you to mutate.");

FOOD("ant egg",		 5, 80,	c_white,	FLESH,	itm_null,
    4,  2, 10, 100, 0,  0, -1,  0,  1, -10,	&iuse::none,	ADD_NULL, "\
A large ant egg, the size of a softball. Extremely nutritious, but gross.");

FOOD("marloss berry",	 2,600, c_pink,		VEGGY,	itm_null,
    1,  1, 20, 40,  0,  0,-10,  0,  1, 30,	&iuse::marloss,	ADD_NULL, "\
This looks like a blueberry the size of your fist, but pinkish in color. It\n\
has a strong but delicious aroma, but is clearly either mutated or of alien\n\
origin.");

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("flour",		20, 25, c_white,	POWDER, itm_box_small,
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func       addiction type
    2,  4,  0,  1,  0,  0,  0,  0,  8, -5,	&iuse::none,	ADD_NULL, "\
This white flour is useful for baking.");

FOOD("sugar",		20, 25, c_white,	POWDER, itm_box_small,
    2,  3,  0,  3,  0,  4, -3,  0,  8,  1,	&iuse::none,	ADD_NULL, "\
Sweet, sweet sugar. Bad for your teeth and surprisingly not very tasty\n\
on its own.");

FOOD("salt",		20, 18, c_white,	POWDER, itm_box_small,
    2,  2,-10,  0,  0,  0,  0,  0,  8, -8,	&iuse::none,	ADD_NULL, "\
Yuck, you surely wouldn't want to eat this. It's good for preserving meat\n\
and cooking with, though.");

FOOD("raw potato",	10, 10, c_brown,	VEGGY,  itm_null,
    1,  1,  0,  8,240,  0, -2,  0,  1, -3,	&iuse::none,	ADD_NULL, "\
Mildly toxic and not very tasty raw. When cooked, it is delicious.");

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("baked potato",	 0, 30, c_brown,	VEGGY,  itm_null,
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func       addiction type
    1,  1,  0, 20, 48,  0,  1,  0,  1,  3,	&iuse::none,	ADD_NULL, "\
A delicious baked potato. Got any sour cream?");

FOOD("bread",		14, 50, c_brown,	VEGGY,  itm_bag_plastic,
    4,  1,  0, 20,240,  0,  1,  0,  4,  1,	&iuse::none,	ADD_NULL, "\
Healthy and filling.");

FOOD("fruit pie",	20, 80, c_yellow,	VEGGY,  itm_box_small,
    6,  3,  5, 16, 72,  2,  1,  0,  6,  3,	&iuse::none,	ADD_NULL, "\
A delicious baked pie with a sweet fruit filling.");

//   NAME		RAR PRC	COLOR		MAT1	CONTAINER
FOOD("pizza",		 8, 80, c_ltred,	VEGGY,	itm_box_small,
// VOL WGT QUE NUT SPO STM HTH ADD CHG FUN	 use_func       addiction type
    8,  4,  0, 18, 48,  0,  0,  0,  8,  6,	&iuse::none,	ADD_NULL, "\
A vegetarian pizza, with delicious tomato sauce and a fluffy crust.  Its\n\
smell brings back great memories.");

FOOD("MRE - beef",		50,100, c_green,	FLESH,	itm_null,
    2,  1,  0, 50,  0,  0,  1,  0,  1, -4,	&iuse::none,	ADD_NULL, "\
Meal Ready to Eat. A military ration. Though not very tasty, it is very\n\
filling and will not spoil.");

FOOD("MRE - vegetable",		50,100, c_green,	VEGGY,	itm_null,
    2,  1,  0, 40,  0,  0,  1,  0,  1, -4,	&iuse::none,	ADD_NULL, "\
Meal Ready to Eat.  A military ration.  Though not very tasty, it is very\n\
filling and will not spoil.");

FOOD("tea leaves",	10, 13,	c_green,	VEGGY,	itm_bag_plastic,
    2,  1, 0,  2,  0,  0,  0,  0,  5, -1,	&iuse::none, ADD_NULL, "\
Dried leaves of a tropical plant. You cam boil them into tea, or you\n\
can just eat them raw. They aren't too filling though.");

FOOD("coffee powder",	15, 13,	c_brown,	VEGGY,	itm_bag_plastic,
    2,  1, 0,  0,  0,  8,  0,  0,  4, -5,	&iuse::caff, ADD_CAFFEINE, "\
Ground coffee beans. You can boil it into a mediocre stimulant,\n\
or swallow it raw for a lesser stimulative boost.");

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
Simple cloth bandages. Used for healing small amounts of damage.");

MED("first aid",	35,350,	c_red,		itm_null,
	PLASTIC,  0,  0,  0,  2,  0,&iuse::firstaid,	ADD_NULL, "\
A full medical kit, with bandages, anti-biotics, and rapid healing agents.\n\
Used for healing large amounts of damage.");

MED("vitamins",		75, 45,	c_cyan,		itm_null,
	PLASTIC,  0,  3,  0, 20,  0,&iuse::none,	ADD_NULL, "\
Take frequently to improve your immune system.");

MED("aspirin",		85, 30,	c_cyan,		itm_null,
	PLASTIC,  0, -1,  0, 20,  0,&iuse::pkill_1,	ADD_NULL, "\
Low-grade painkiller. Best taken in pairs.");

MED("caffeine pills",	25, 60,	c_cyan,		itm_null,
	PLASTIC,  8,  0,  3, 10,  0,&iuse::caff,	ADD_CAFFEINE, "\
No-doz pills. Useful for staying up all night.");

//  NAME		RAR PRC	COLOR		TOOL
MED("sleeping pills",	15, 50,	c_cyan,		itm_null,
//	MATERIAL STM HTH ADD CHG FUN use_func		addiction type
	PLASTIC, -8,  0, 40, 10,  0,&iuse::sleep,	ADD_SLEEP, "\
Prescription sleep aids. Will make you very tired.");

MED("iodine tablets",	 5,140, c_yellow,	itm_null,
	PLASTIC,  0, -1,  0, 10,  0,&iuse::iodine,	ADD_NULL, "\
Iodine tablets are used for recovering from irradiation. They are not\n\
spectacularly effective, but are better than nothing.");

MED("Dayquil",		70, 75,	c_yellow,	itm_null,
	PLASTIC,  0,  1,  0, 10,  0,&iuse::flumed,	ADD_NULL, "\
Daytime flu medication. Will halt all flu symptoms for a while.");

MED("Nyquil",		70, 85,	c_blue,		itm_null,
	PLASTIC, -7,  1, 20, 10,  0,&iuse::flusleep,	ADD_SLEEP, "\
Nighttime flu medication. Will halt all flu symptoms for a while, plus make\n\
you sleepy.");

MED("inhaler",		14,200,	c_ltblue,	itm_null,
	PLASTIC,  1,  0,  0, 20,  0,&iuse::inhaler,	ADD_NULL, "\
Vital medicine for those with asthma. Those without asthma can use it for a\n\
minor stimulant boost.");

//  NAME		RAR PRC	COLOR		TOOL
MED("codeine",		15,400,	c_cyan,		itm_null,
//	MATERIAL STM HTH ADD CHG FUN use_func		addiction type
	PLASTIC, -2,  0, 10, 10, 10,&iuse::pkill_2,	ADD_PKILLER, "\
A weak opiate, prescribed for light to moderate pain.");

MED("oxycodone",	 7,900,	c_cyan,		itm_null,
	PLASTIC, -4, -1, 16, 10, 18,&iuse::pkill_3,	ADD_PKILLER, "\
A strong opiate, prescribed for moderate to intense pain.");

MED("tramadol",		11,300,	c_cyan,		itm_null,
	PLASTIC,  0,  0,  6, 10,  6,&iuse::pkill_l,	ADD_PKILLER, "\
A long-lasting opiate, prescribed for moderate pain. Its painkiller effects\n\
last for several hours, but are not as strong as oxycodone.");

MED("Xanax",		10,600,	c_cyan,		itm_null,
	PLASTIC, -4,  0,  0, 10, 20,&iuse::xanax,	ADD_NULL, "\
Anti-anxiety medication. It will reduce your stimulant level steadily, and\n\
will temporarily cancel the effects of anxiety, like the Hoarder trait.");

//  NAME		RAR PRC	COLOR		TOOL
MED("Adderall",		10,450,	c_cyan,		itm_null,
//	MATERIAL STM HTH ADD CHG FUN use_func		addiction type
	PLASTIC, 24,  0, 10, 10, 10,&iuse::none,	ADD_SPEED, "\
A strong stimulant prescribed for ADD. It will greatly increase your\n\
stimulant level, but is quite addictive.");

MED("Thorazine",	 7,500,	c_cyan,		itm_null,
	PLASTIC,  0,  0,  0, 10,  0,&iuse::thorazine,	ADD_NULL, "\
Anti-psychotic medication. Used to control the symptoms of schizophrenia and\n\
similar ailments. Also popular as a way to come down from a bad trip.");

MED("Prozac",		10,650,	c_cyan,		itm_null,
	PLASTIC, -4,  0,  0, 15,  0,&iuse::prozac,	ADD_NULL, "\
A strong anti-depressant. Useful if your morale level is very low.");

MED("cigarettes",	90,120,	c_dkgray,	itm_lighter,
	VEGGY,    1, -1, 40, 10,  5,&iuse::cig,		ADD_CIG, "\
These will boost your dexterity, intelligence, and perception for a short\n\
time. They are quite addictive.");

//  NAME		RAR PRC	COLOR
MED("marijuana",	20,250,	c_green,	itm_lighter,
//	MATERIAL STM HTH ADD CHG FUN use_func		addiction type
	VEGGY,   -8,  0,  0,  5, 18,&iuse::weed,	ADD_NULL, "\
Really useful only for relaxing. Will reduce your attributes and reflexes.");

MED("cocaine",		 8,420,	c_white,	itm_null,
	POWDER,  20, -2, 30,  8, 25,&iuse::coke,	ADD_COKE, "\
A strong, illegal stimulant. Highly addictive.");

MED("methamphetamine",	 2,800, c_ltcyan,	itm_apparatus,
	POWDER,  10, -4, 50,  6, 30,&iuse::meth,	ADD_SPEED, "\
A very strong illegal stimulant. Extremely addictive and bad for you, but\n\
also extremely effective in boosting your alertness.");

//  NAME		RAR PRC	COLOR
MED("heroin",		 1,1000,c_brown,	itm_syringe,
//	MATERIAL STM HTH ADD CHG FUN use_func		addiction type
	POWDER, -10, -3, 60,  4, 45,&iuse::pkill_4,	ADD_PKILLER, "\
A very strong illegal opiate. Unless you have an opiate tolerance, avoid\n\
heroin, as it will be too strong for you.");

MED("cigars",		 5,120,	c_dkgray,	itm_lighter,
	VEGGY,    1, -1, 40, 10, 15,&iuse::cig,		ADD_CIG, "\
A gentleman's vice. Cigars are what separates a gentleman from a savage.");

MED("antibiotics",	25,900, c_pink,		itm_null,
	PLASTIC,   0, -2,  0, 15,  0,&iuse::none,	ADD_NULL, "\
Medication designed to stop the spread of, and kill, bacterial infections.");

MED("Poppy Sleep",	25,900, c_pink,		itm_null,
	PLASTIC,   0, -2,  0, 5,  0,&iuse::sleep,	ADD_NULL, "\
Sleeping pills made by refining mutated poppy seeds.");

MED("Poppy Painkillers",25,900, c_pink,		itm_null,
	PLASTIC,   0, -2,  0, 10,  0,&iuse::pkill_2,	ADD_NULL, "\
Painkillers made by refining mutated poppy seeds..");

MED("crack",		 8,420,	c_white,	itm_apparatus,
	POWDER,  40, -2, 80,  4, 50,&iuse::crack,	ADD_CRACK, "\
Refined cocaine, incredibly addictive.");

MED("Grack Cocaine",      8,420, c_white,        itm_apparatus,
        POWDER,  200, -2, 80,  4, 50,&iuse::grack,       ADD_CRACK, "\
Grack Cocaine, the strongest substance known to the multiverse\n\
this potent substance is refined from the sweat of the legendary\n\
gracken");


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
Just a piece of butcher's paper. Good for starting fires.");

MELEE("syringe",	 8, 25, ',', c_ltcyan,	PLASTIC,MNULL,
	 1,  0, -4,  6, -2, mfb(IF_SPEAR), "\
A medical syringe. Used for administering heroin and other drugs.");

MELEE("rag",		72, 10, ';', c_dkgray,	COTTON,	MNULL,
	 1,  1,-10,  0,  0, 0, "\
A small piece of cloth. Useful for making molotov cocktails and not much else."
);

MELEE("fur pelt",	 0, 10, ',', c_brown,	WOOL,	FLESH,
	 1,  1, -8,  0,  0, 0, "\
A small bolt of fur from an animal. Can be made into warm clothing.");

MELEE("leather patch",	 0, 20, ',', c_red,	LEATHER,FLESH,
	 2,  1, -2,  0, -1, 0, "\
A smallish patch of leather, could be used to make tough clothing.");

//    NAME		RAR PRC SYM  COLOR	MAT1	MAT2
MELEE("superglue",	30, 18, ',', c_white,	PLASTIC,MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  0, -2,  0, -2, 0, "\
A tube of strong glue. Used in many crafting recipes.");

MELEE("science ID card", 2,600, ',', c_pink,	PLASTIC,MNULL,
	 0,  0, -8,  1, -3, 0, "\
This ID card once belonged to a scientist of some sort. It has a magnetic\n\
stripe on the back; perhaps it can be used on a control panel.");

MELEE("military ID card",3,1200,',', c_pink,	PLASTIC,MNULL,
	 0,  0, -8,  1, -3, 0, "\
This ID card once belonged to a military officer with high-level clearance.\n\
It has a magnetic stripe on the back; perhaps it can be used on a control\n\
panel.");

MELEE("electrohack",	 3,400, ',', c_green,	PLASTIC,STEEL,
	 2,  2,  5,  0,  1, 0, "\
This device has many ports attached, allowing to to connect to almost any\n\
control panel or other electronic machine (but not computers). With a little\n\
skill, it can be used to crack passwords and more.");

MELEE("string - 6 in",	 2,  5, ',', c_ltgray,	COTTON,	MNULL,
	 0,  0,-20,  0,  1, 0, "\
A small piece of cotton string.");

MELEE("string - 3 ft",	40, 30, ',', c_ltgray,	COTTON,	MNULL,
	 1,  0, -5,  0,  1, 0, "\
A long piece of cotton string. Use scissors on it to cut it into smaller\n\
pieces.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("rope - 6 ft",	 4, 45, ',', c_yellow,	WOOD,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 2,  4,  1,  0,  1, mfb(IF_WRAP), "\
A short piece of nylon rope. Too small to be of much use.");

MELEE("rope - 30 ft",	35,100, ',', c_yellow,	WOOD,	MNULL,
	10, 20,  1,  0, -10, 0, "\
A long nylon rope. Useful for keeping yourself safe from falls.");

MELEE("steel chain",	20, 80, '/', c_cyan,	STEEL,	MNULL,
	 4,  8, 12,  0,  2, mfb(IF_WRAP), "\
A heavy steel chain. Useful as a weapon, or for crafting. It has a chance\n\
to wrap around your target, allowing for a bonus unarmed attack.");
TECH( mfb(TEC_GRAB) );

MELEE("processor board",15,120, ',', c_ltcyan,	IRON,	PLASTIC,
	 1,  0, -3,  0, -1, 0, "\
A central processor unit, useful in advanced electronics crafting.");

MELEE("RAM",		22, 90, ',', c_ltcyan,	IRON,	PLASTIC,
	 1,  0, -5,  0, -1, 0, "\
A stick of memory. Useful in advanced electronics crafting.");

MELEE("power converter",16,170, ',', c_ltcyan,	IRON,	PLASTIC,
	 4,  2,  5,  0, -1, 0, "\
A power supply unit. Useful in lots of electronics recipes.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("amplifier circuit",8,200,',', c_ltcyan,	IRON,	PLASTIC,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  0, -5,  0, -1, 0, "\
A circuit designed to amplify the strength of a signal. Useful in lots of\n\
electronics recipes.");

MELEE("transponder circuit",5,240,',',c_ltcyan,	IRON,	PLASTIC,
	 1,  0, -5,  0, -1, 0, "\
A circuit designed to repeat a signal. Useful for crafting communications\n\
equipment.");

MELEE("signal receiver",10,135, ',', c_ltcyan,	IRON,	PLASTIC,
	 1,  0, -4,  0, -1, 0, "\
A module designed to receive many forms of signals. Useful for crafting\n\
communications equipment.");

MELEE("antenna",	18, 80, ',', c_ltcyan,	STEEL,	MNULL,
	 1,  0, -6,  0,  2, 0, "\
A simple thin aluminum shaft. Useful in lots of electronics recipes.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("chunk of steel", 30, 10, ',', c_ltblue,	STEEL,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 2,  6, 12,  0, -2, 0, "\
A misshapen chunk of steel. Makes a decent weapon in a pinch, and is also\n\
useful for some crafting recipes.");

//    NAME      RAR PRC SYM COLOR   MAT1    MAT2
MELEE("lump of steel", 30, 20, ',', c_ltblue,  STEEL,  MNULL,
//  VOL WGT DAM CUT HIT FLAGS
     2,  80, 18,  0, -4, 0, "\
A misshapen heavy piece of steel. Useful for some crafting recipes.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("scrap metal", 30, 10, ',', c_ltblue,	STEEL,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  1,  0,  0, -2, 0, "\
An assortment of small bits of metal and scrap\n\
useful in all kinds of crafting");

MELEE("rubber hose",	15, 80, ',', c_green,	PLASTIC,MNULL,
	 3,  2,  4,  0,  3, mfb(IF_WRAP), "\
A flexible rubber hose. Useful for crafting.");

MELEE("sheet of glass",	 5,135, ']', c_ltcyan,	GLASS,	MNULL,
	50, 20, 16,  0, -5, 0, "\
A large sheet of glass. Easily shattered. Useful for re-paning windows.");

MELEE("manhole cover",	 1, 20, ']', c_dkgray,	IRON,	MNULL,
	45,250, 20,  0,-10, 0, "\
A heavy iron disc which generally covers a ladder into the sewers. Lifting\n\
it from the manhole is impossible without a crowbar.");
TECH( mfb(TEC_WBLOCK_3) );

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("rock",		40,  0, '*', c_ltgray,	STONE,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  3, 12,  0, -2, 0, "\
A rock the size of a baseball. Makes a decent melee weapon, and is also good\n\
for throwing at enemies.");

MELEE("heavy stick",	95,  0, '/', c_brown,	WOOD,	MNULL,
	 6, 10, 12,  0,  3, 0, "\
A sturdy, heavy stick. Makes a decent melee weapon, and can be cut into two\n\
by fours for crafting.");

MELEE("broom",		30, 24, '/', c_blue,	PLASTIC,MNULL,
	10,  8,  6,  0,  1, 0, "\
A long-handled broom. Makes a terrible weapon unless you're chasing cats.");
TECH( mfb(TEC_WBLOCK_1) );

MELEE("sledge hammer",	 6, 120,'/', c_brown,	WOOD,	IRON,
	18, 38, 40,  0,  0, 0, "\
A large, heavy hammer. Makes a good melee weapon for the very strong, but is\n\
nearly useless in the hands of the weak.");
TECH( mfb(TEC_BRUTAL)|mfb(TEC_WIDE) );

MELEE("hatchet",	10,  95,';', c_ltgray,	IRON,	WOOD,
	 6,  7, 12, 12,  1, 0, "\
A one-handed hatchet. Makes a great melee weapon, and is useful both for\n\
cutting wood, and for use as a hammer.");

MELEE("nail board",	 5,  80,'/', c_ltred,	WOOD,	MNULL,
	 6,  6, 16,  6,  1, mfb(IF_STAB), "\
A long piece of wood with several nails through one end; essentially a simple\n\
mace. Makes a great melee weapon.");

MELEE("nail bat",	60, 160,'/', c_ltred,	WOOD,	MNULL,
	12, 10, 28,  6,  3, mfb(IF_STAB), "\
A baseball bat with several nails driven through it, an excellent melee weapon.");

MELEE("X-Acto knife",	10,  40,';', c_dkgray,	IRON,	PLASTIC,
	 1,  0,  0, 14, -4, mfb(IF_SPEAR), "\
A small, very sharp knife.  Causes decent damage but is difficult to hit\n\
with. Its small tip allows for a precision strike in hands of the skill."
);
TECH(mfb(TEC_PRECISE));

MELEE("scalpel",	48,  40,',', c_cyan,	STEEL,	MNULL,
	 1,  0,  0, 18, -4, mfb(IF_SPEAR), "\
A small, very sharp knife, used in surgery. Its small tip allows for a\n\
precision strike in the hands of the skilled.");
TECH(mfb(TEC_PRECISE));

MELEE("pot",		25,  45,')', c_ltgray,	IRON,	MNULL,
	 8,  6,  9,  0,  1, 0, "\
Useful for boiling water when cooking spaghetti and more.");

MELEE("frying pan",	25,  50,')', c_ltgray,	IRON,	MNULL,
	 6,  6, 14,  0,  2, 0, "\
A cast-iron pan. Makes a decent melee weapon, and is used for cooking.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("butter knife",	90,  15,';', c_ltcyan,	STEEL, 	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  2,  2,  1, -2, 0, "\
A dull knife, absolutely worthless in combat.");

MELEE("two by four", 	60,  80,'/', c_ltred,	WOOD,	MNULL,
	 6,  6, 14,  0,  1, 0, "\
A plank of wood. Makes a decent melee weapon, and can be used to board up\n\
doors and windows if you have a hammer and nails.");
TECH( mfb(TEC_WBLOCK_1) );

MELEE("muffler",	30,  30,'/', c_ltgray,	IRON,	MNULL,
	20, 20, 19,  0, -3, 0, "\
A muffler from a car. Very unwieldy as a weapon. Useful in a few crafting\n\
recipes.");
TECH( mfb(TEC_WBLOCK_2) );

MELEE("pipe",		20,  75,'/', c_dkgray,	STEEL,	MNULL,
	 4, 10, 13,  0,  3, 0, "\
A steel pipe, makes a good melee weapon. Useful in a few crafting recipes.");

MELEE("baseball bat",	60, 160,'/', c_ltred,	WOOD,	MNULL,
	12, 10, 28,  0,  3, 0, "\
A sturdy wood bat. Makes a great melee weapon.");
TECH( mfb(TEC_WBLOCK_1) );

MELEE("aluminium bat",	60, 160,'/', c_ltred,	STEEL,	MNULL,
	10, 6, 24,  0,  3, 0, "\
An aluminium baseball bat, smaller and lighter than a wooden bat\n\
and a little less damaging as a result.");
TECH( mfb(TEC_WBLOCK_1) );

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("machete",	 5, 280,'/', c_blue,	IRON,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 8, 14,  6, 28,  2, 0, "\
This huge iron knife makes an excellent melee weapon.");
TECH( mfb(TEC_WBLOCK_1) );

MELEE("katana",		 2, 980,'/', c_ltblue,	STEEL,	MNULL,
	16, 16,  4, 45,  1, mfb(IF_STAB), "\
A rare sword from Japan. Deadly against unarmored targets, and still very\n\
effective against the armored.");
TECH( mfb(TEC_RAPID)|mfb(TEC_WBLOCK_2) );

MELEE("wood spear",	 5,  40,'/', c_ltred,	WOOD,	MNULL,
	 5,  3,  4, 18,  1, mfb(IF_SPEAR), "\
A simple wood pole with one end sharpened.");
TECH( mfb(TEC_WBLOCK_1) | mfb(TEC_RAPID) );

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("steel spear",      5,  140,'/', c_ltred,   WOOD,   STEEL,
//	VOL WGT DAM CUT HIT FLAGS
         6,  6,  2, 28,  1, mfb(IF_SPEAR), "\
A simple wood pole made deadlier by the knife tied to it.");
TECH( mfb(TEC_WBLOCK_1) | mfb(TEC_RAPID) );

MELEE("expandable baton",8, 175,'/', c_blue,	STEEL,	MNULL,
	 1,  4, 12,  0,  2, 0, "\
A telescoping baton that collapses for easy storage. Makes an excellent\n\
melee weapon.");
TECH( mfb(TEC_WBLOCK_1) );

MELEE("bee sting",	 5,  70,',', c_dkgray,	FLESH,	MNULL,
	 1,  0,  0, 18, -1, mfb(IF_SPEAR), "\
A six-inch stinger from a giant bee. Makes a good melee weapon.");
TECH( mfb(TEC_PRECISE) );

MELEE("wasp sting",	 5,  90,',', c_dkgray,	FLESH,	MNULL,
	 1,  0,  0, 22, -1, mfb(IF_SPEAR), "\
A six-inch stinger from a giant wasp. Makes a good melee weapon.");
TECH( mfb(TEC_PRECISE) );

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("chunk of chitin",10,  15,',', c_red,	FLESH,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 1,  0,  1,  0, -2, 0, "\
A piece of an insect's exoskeleton. It is light and very durable.");

MELEE("biollante bud",   1, 400,',', c_magenta,	VEGGY,	MNULL,
	 1,  0, -8,  0, -3, 0, "\
An unopened biollante flower, brilliant purple in color. It may still have\n\
its sap-producing organ intact.");

MELEE("empty canister",  5,  20,'*', c_ltgray,	STEEL,	MNULL,
	 1,  1,  2,  0, -1, 0, "\
An empty canister, which may have once held tear gas or other substances.");

MELEE("gold bar",	10,3000,'/', c_yellow,	STEEL,	MNULL,
	 2, 60, 14,  0, -1, 0, "\
A large bar of gold. Before the apocalypse, this would've been worth a small\n\
fortune; now its value is greatly diminished.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("coal pallet",	20, 600,'/', c_dkgray,	STONE,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 72,100, 8,  0, -5, 0, "\
A large block of semi-processed coal.");

MELEE("petrified eye",   1,2000,'*', c_dkgray,	STONE,	MNULL,
	 2,  8, 10,  0, -1, 0, "\
A fist-sized eyeball with a cross-shaped pupil. It seems to be made of\n\
stone, but doesn't look like it was carved.");

MELEE("spiral stone",   20, 200,'*', c_pink,	STONE,	MNULL,
	 1,  3, 14,  0, -1, 0, "\
A rock the size of your fist. It is covered with intricate spirals; it is\n\
impossible to tell whether they are carved, naturally formed, or some kind of\n\
fossil.");

MELEE("rapier",		 3, 980,'/', c_ltblue,	STEEL,	MNULL,
	 6,  9, 5, 28,  2, mfb(IF_STAB), "\
Preferred weapon of gentlemen and swashbucklers. Light and quick, it makes\n\
any battle a stylish battle.");
TECH( mfb(TEC_RAPID)|mfb(TEC_WBLOCK_1)|mfb(TEC_PRECISE) );

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("walking cane",   10, 160,'/', c_ltred,	WOOD,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	  8,  7, 10,  0,  2, 0, "\
Handicapped or not, you always walk in style. Consisting of a metal\n\
headpiece and a wooden body, this makes a great bashing weapon in a pinch.");
TECH( mfb(TEC_WBLOCK_1) );

MELEE("binoculars",	20, 300,';', c_ltgray,	PLASTIC,GLASS,
	  2,  3,  6,  0, -1, 0, "\
A tool useful for seeing long distances. Simply carrying this item in your\n\
inventory will double the distance that is mapped around you during your\n\
travels.");

MELEE("USB drive",	 5, 100,',', c_white,	PLASTIC,MNULL,
	  0,  0,  0,  0,  0, 0, "\
A USB thumb drive. Useful for holding software.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("awl pike",        5,2000,'/', c_ltcyan,	IRON,	WOOD,
//	VOL WGT DAM CUT HIT FLAGS
        14, 18,  8, 50,  2, mfb(IF_SPEAR), "\
A medieval weapon consisting of a wood shaft, tipped with an iron spike.\n\
Though large and heavy compared to other spears, its accuracy and damage\n\
are unparalled.");

MELEE("broadsword",	30,1200,'/',c_cyan,	IRON,	MNULL,
	 7, 11,  8, 35,  2, mfb(IF_STAB), "\
An early modern sword seeing use in the 16th, 17th ane 18th centuries.\n\
Called 'broad' to contrast with the slimmer rapiers.");

MELEE("mace",		20,1000,'/',c_dkgray,	IRON,	WOOD,
	10, 18, 36,  0,  1, 0, "\
A medieval weapon consisting of a wood handle with a heavy iron end. It\n\
is heavy and slow, but its crushing damage is devastating.");
TECH( mfb(TEC_SWEEP) );

MELEE("morningstar",	10,1200,'/',c_dkgray, 	IRON,	WOOD,
	11, 20, 32,  4,  1, mfb(IF_SPEAR), "\
A medieval weapon consisting of a wood handle with a heavy, spiked iron\n\
ball on the end. It deals devastating crushing damage, with a small\n\
amount of piercing to boot.");
TECH( mfb(TEC_SWEEP) );

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("pool cue",	 4, 80,'/', c_red,	WOOD,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	14,  5, 12,  0,  3, 0, "\
A hard-wood stick designed for hitting colorful balls around a felt\n\
table. Truly, the coolest of sports.");
TECH( mfb(TEC_WBLOCK_1) );

MELEE("pool ball",	40, 30,'*', c_blue,	STONE,	MNULL,
	 1,  3, 12,  0, -3, 0, "\
A colorful, hard ball. Essentially a rock.");

MELEE("candlestick",	20,100,'/', c_yellow,	SILVER,	MNULL,
	 1,  5, 12,  0,  1,  0, "\
A gold candlestick.");

MELEE("spike",           0, 0,';',  c_cyan,     STEEL,  MNULL,
	 2,  2,  2, 10, -2, mfb(IF_STAB),"\
A large and slightly misshapen spike, could do some damage\n\
mounted on a vehicle.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
MELEE("blade",	 5, 280,'/', c_blue,	IRON,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 8, 14,  6, 10, -2, 0, "\
A large and slightly misshapen blade, could do some damage\n\
mounted on a vehicle.");

MELEE("wire",   50, 200,';', c_blue,    STEEL,  MNULL,
         4,  2,  0,  0, -2, 0, "\
A length of thin, relatively stiff, steel wire. Like the\n\
the sort you find in wire fences.");

MELEE("barbed wire",   20, 200,';', c_blue,    STEEL,  MNULL,
         4,  2,  0,  0, -2, 0, "\
A length of stiff wire, covered in sharp barbs.");

MELEE("rebar",		20,  75,'/', c_ltred,	IRON,	MNULL,
	 6, 10, 13,  0,  2, 0, "\
A length of rebar, makes a nice melee weapon, and could be\n\
handy in constructing tougher walls and such.");

MELEE("log",                    20,  100,'/', c_brown,  WOOD,   MNULL,
         40, 20, 10, 0, -10, 0, "\
A large log, cut from a tree. Useful in building, or further\n\
processing into planks.");

MELEE("splintered wood", 	60,  80,'/', c_ltred,	WOOD,	MNULL,
	 6,  6, 9,  0,  1, 0, "\
A splintered piece of wood, useless as anything but kindling");

MELEE("skewer",                 10,  10,',', c_brown,   WOOD,   MNULL,
         0,  1, 0,  0,  -10, 0, "\
A thin wooden skewer. Squirrel on a stick, anyone?");

MELEE("crack pipe",             37,  35, ',',c_ltcyan,  GLASS,  MNULL,
         1,  1, 0,  0,  -10, 0, "\
A fine glass pipe, with a bulb on the end, used for partaking of\n\
certain illicit substances.");

MELEE("burnt out torch",	95,  0, '/', c_brown,	WOOD,	MNULL,
	 6, 10, 12,  0,  3, 0, "\
A torch which has consumed all its fuel, can be recrafted\n\
into another torch");

MELEE("spring", 50, 10, ',', c_ltgray,  STEEL,  MNULL,
         3,  0, -1,  0,  0, 0, "\
A large heavy duty spring. Expands with significant force\n\
when compressed.");

MELEE("lawnmower", 25, 100, ';', c_red, STEEL,  IRON,
         25, 40, -3, 10, 0, 0, "\
A motorized pushmower, it seems to be broken. You could\n\
take it apart if you had a wrench.");

MELEE("lawnmower blade", 0, 100, '/', c_ltgray, IRON, MNULL,
	 7, 5,  4, 15,  -1, mfb(IF_STAB), "\
The blade of a lawnmower. It's not incredibly sharp, but\n\
it could still do some serious damage.");

MELEE("lawnmower machete", 0, 100, '/', c_ltgray, IRON, MNULL,
         7, 5,  4, 15,   1, mfb(IF_STAB), "\
A lawnmower blade that's been fashioned into a makeshift\n\
machete, mainly by adding a handle for easier wielding.");

MELEE("lawnmower halberd", 0, 100, '/', c_ltgray, IRON, MNULL,
         10, 7, 4, 15,   2, mfb(IF_STAB), "\
A lawnmower blade affixed to a long stick, in the right\n\
hands, this thing could do some massive damage.");

MELEE("curtain",           0, 100, ';', c_dkgray, COTTON, MNULL,
         20, 2, 0, 0,    -1, 0, "\
A large fabric curtain, could be attached to a window or\n\
cut up for plenty of rags.");

//  NAME        RAR PRC SYM  COLOR  MAT1    MAT
MELEE("damaged tent",17, 65, ';', c_green,	IRON,	MNULL,
	 10,  20,  4,  0, -3, 0, "\
A small tent, just big enough to fit a person comfortably.\n\
This tent is broken and cannot be deployed");

MELEE("Heating element", 20, 10, ',', c_cyan,   IRON,   MNULL,
         0,   1,   0,  0,  0, 0, "\
A heating element, like the ones used in hotplates or kettles.");

MELEE("Television",      40, 0,  ';', c_dkgray,   PLASTIC, GLASS,
        10,  12,  5, 0, -5, 0, "\
A large cathode ray tube television, full of delicious\n\
electronics.");

MELEE("Pilot light", 20, 10, ',', c_cyan,   IRON,   PLASTIC,
         0,   1,   0,  0,  0, 0, "\
A pilot light from a gas-burning device, this particular one\n\
is a simple piezo electric igniter.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("steel frame",  25, 35, ']', c_cyan,  STEEL,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    60,  240,  20,  0,  -5, 0, "\
A large frame made of steel. Useful for crafting.");
TECH( mfb(TEC_WBLOCK_3) );

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("wheel",  15, 50, ']', c_dkgray,  STEEL,   PLASTIC,
//  VOL WGT DAM CUT HIT FLAGS
    10,  80,  8,  0,  -4, 0, "\
A wheel, perhaps from some car.");
TECH( mfb(TEC_WBLOCK_3) );

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("large wheel",  6, 80, ']', c_dkgray,  STEEL,   PLASTIC,
//  VOL WGT DAM CUT HIT FLAGS
    20,  200,  12,  0,  -5, 0, "\
A large wheel, from some big car.");
TECH( mfb(TEC_WBLOCK_3) );

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("seat",  8, 250, '0', c_red,  PLASTIC,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    30,  80,  4,  0,  -4, 0, "\
A soft car seat covered with leather.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("vehicle controls",  3, 400, '$', c_ltcyan,  PLASTIC,   STEEL,
//  VOL WGT DAM CUT HIT FLAGS
    12,  30,  2,  0,  -4, 0, "\
A set of various vehicle controls. Useful for crafting.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("100CC combustion engine",  4, 100, ':', c_ltcyan,  IRON,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    6,  70,  4,  0,  -1, 0, "\
A small go kart combustion engine. Useful for crafting.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("1L combustion engine",  5, 150, ':', c_ltcyan,  IRON,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    6,  160,  8,  0,  -2, 0, "\
A small, yet powerful 2-cylinder combustion engine. Useful for crafting.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("2.5L combustion engine",  4, 180, ':', c_ltcyan,  IRON,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    14,  400,  12,  0,  -3, 0, "\
A powerful 4-cylinder combustion engine. Useful for crafting.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("6L combustion engine",  2, 250, ':', c_ltcyan,  IRON,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    25,  1100,  15,  0,  -5, 0, "\
A large and very powerful 8-cylinder combustion engine. Useful for\n\
crafting.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("electric motor",  2,120, ',', c_ltcyan,  IRON,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    6,  80,  4,  0,  0, 0, "\
A powerful electric motor. Useful for crafting.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("large electric motor",  1,220, ':', c_ltcyan,  IRON,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    15,  650,  9,  0,  -3, 0, "\
A large and very powerful electric motor. Useful for crafting.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("plasma engine",  1, 900, ':', c_ltcyan,  STEEL,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    12,  350,  7,  0,  -2, 0, "\
High technology engine, working on hydrgen fuel.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("foot crank",  10, 90, ':', c_ltgray,  IRON,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    2,  10,  10,  0,  -1, 0, "\
The pedal and gear assembly from a bicycle.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("metal tank",  10, 40, '}', c_ltcyan,  STEEL,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    18,  25,  3,  0,  -2, 0, "\
A metal tank for holding liquids. Useful for crafting.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("storage battery",  6, 80, ':', c_ltcyan,  IRON,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    8,  220,  6,  0,  -2, 0, "\
A large storage battery. Useful for crafting.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("minireactor",  1, 900, ':', c_ltcyan,  STEEL,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    6,  250,  11,  0,  -4, 0, "\
A small portable plutonium reactor. Handle with great care!");

//      NAME          RAR PRC SYM COLOR        MAT1    MAT2
MELEE("solar panel",  3, 900, ']', c_yellow,  GLASS,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    12,  4,  1,  0,  -4, 0, "\
Electronic device which can convert solar radiation into electric\n\
power. Useful for crafting.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("steel plating",  30, 120, ']', c_ltcyan,  STEEL,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    12,  600,  6,  0,  -1, 0, "\
A piece of armor plating made of steel.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("superalloy plating",  10, 185, ']', c_ltcyan,  STEEL,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    12,  350,  6,  0,  -1, 0, "\
A piece of armor plating made of sturdy superalloy.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("spiked plating",  15, 185, ']', c_ltcyan,  STEEL,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    14,  600,  6,  3,  -1, 0, "\
A piece of armor plating made of steel. It is covered by menacing\n\
spikes.");

//      NAME           RAR PRC SYM COLOR        MAT1    MAT2
MELEE("hard plating",  30, 160, ']', c_ltcyan,  STEEL,   MNULL,
//  VOL WGT DAM CUT HIT FLAGS
    12,  1800,  6,  0,  -1, 0, "\
A piece of very thick armor plating made of steel.");

// ARMOR
#define ARMOR(name,rarity,price,color,mat1,mat2,volume,wgt,dam,to_hit,\
encumber,dmg_resist,cut_resist,env,warmth,storage,covers,des)\
	index++;itypes.push_back(new it_armor(index,rarity,price,name,des,'[',\
color,mat1,mat2,volume,wgt,dam,0,to_hit,0,covers,encumber,dmg_resist,cut_resist,\
env,warmth,storage))

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("sneakers",	80, 100,C_SHOES,	LEATHER,	MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    5,  4, -2,  0,  0,  0,  2,  0,  2,  0,	mfb(bp_feet), "\
Guaranteed to make you run faster and jump higher!");

ARMOR("boots",		70, 120,C_SHOES,	LEATHER,	MNULL,
    7,  6,  1, -1,  1,  1,  4,  2,  4,  0,	mfb(bp_feet), "\
Tough leather boots. Very durable.");

ARMOR("steeltoed boots",50, 135,C_SHOES,	LEATHER,	STEEL,
    7,  9,  4, -1,  1,  4,  4,  3,  2,  0,	mfb(bp_feet), "\
Leather boots with a steel toe. Extremely durable.");

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
Simple sandals. Very difficult to run in.");

ARMOR("dress shoes",	50,  45,C_SHOES,	LEATHER,	MNULL,
    5,  3,  1,  1,  1,  0,  3,  0,  1,  0,	mfb(bp_feet), "\
Fancy patent leather shoes. Not designed for running in.");

ARMOR("heels",		50,  50,C_SHOES,	LEATHER,	MNULL,
    4,  2,  6, -2,  4,  0,  0,  0,  0,  0,	mfb(bp_feet), "\
A pair of high heels. Difficult to even walk in.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("sneakers",	10, 100,C_SHOES,	LEATHER,	MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    5,  4, -2,  0, -2,  0,  2,  0,  2,  0,	mfb(bp_feet), "\
Guaranteed to make you run faster and jump higher!\n\
These sneakers are a perfect fit for you.");

ARMOR("boots",		5, 120,C_SHOES,	LEATHER,	MNULL,
    7,  6,  1, -1,  0,  1,  4,  2,  4,  0,	mfb(bp_feet), "\
Tough leather boots. Very durable.\n\
These boots are a perfect fit for you.");

ARMOR("steeltoed boots",5, 135,C_SHOES,	LEATHER,	STEEL,
    7,  9,  4, -1,  0,  4,  4,  3,  2,  0,	mfb(bp_feet), "\
Leather boots with a steel toe. Extremely durable.\n\
These boots are a perfect fit for you.");

ARMOR("winter boots",   5, 140,C_SHOES,	PLASTIC,	WOOL,
    8,  7,  0, -1,  1,  0,  2,  1,  7,  0,	mfb(bp_feet), "\
Cumbersome boots designed for warmth.\n\
These boots are a perfect fit for you.");

ARMOR("dress shoes",	5,  45,C_SHOES,	LEATHER,	MNULL,
    5,  3,  1,  1,  0,  0,  3,  0,  1,  0,	mfb(bp_feet), "\
Fancy patent leather shoes. Not designed for running in.\n\
These shoes are a perfect fit for you.");

ARMOR("heels",		1,  50,C_SHOES,	LEATHER,	MNULL,
    4,  2,  6, -2,  2,  0,  0,  0,  0,  0,	mfb(bp_feet), "\
A pair of high heels. Difficult to even walk in.\n\
These high heels are a perfect fit for you.");

ARMOR("jeans",		90, 180,C_PANTS,	COTTON,		MNULL,
    5,  4, -4,  1,  1,  0,  1,  0,  1,  2,	mfb(bp_legs), "\
A pair of blue jeans with two deep pockets.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("pants",		75, 185,C_PANTS,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    5,  5, -4,  1,  1,  0,  1,  0,  2,  4,	mfb(bp_legs), "\
A pair of khaki pants. Slightly warmer than jeans.");

ARMOR("leather pants",	60, 210,C_PANTS,	LEATHER,	MNULL,
    6,  8, -2,  1,  2,  1,  7,  0,  5,  2,	mfb(bp_legs), "\
A pair of black leather pants. Very tough, but encumbersome and without much\n\
storage.");

ARMOR("cargo pants",	70, 280,C_PANTS,	COTTON,		MNULL,
    6,  6, -3,  0,  1,  0,  2,  0,  3, 12,	mfb(bp_legs), "\
A pair of pants lined with pockets, offering lots of storage.");

ARMOR("army pants",	30, 315,C_PANTS,	COTTON,		MNULL,
    6,  7, -2,  0,  1,  0,  3,  0,  4, 14,	mfb(bp_legs), "\
A tough pair of pants lined with pockets. Favored by the military.");

ARMOR("skirt",		75, 120,C_PANTS,	COTTON,		MNULL,
    2,  2, -5,  0, -1,  0,  0,  0,  0,  1,	mfb(bp_legs), "\
A short, breezy cotton skirt. Easy to move in, but only has a single small\n\
pocket.");

ARMOR("jeans",          20, 180, C_PANTS,       COTTON,         MNULL,
    5,  4, -4,  1, -1,  0,  1,  0,  1,  2,      mfb(bp_legs), "\
A pair of blue jeans with two deep pockets.\n\
These jeans are a perfect fit for you.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("pants",		20, 185,C_PANTS,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    5,  5, -4,  1,  -1,  0,  1,  0,  2,  4,	mfb(bp_legs), "\
A pair of khaki pants.  Slightly warmer than jeans.\n\
These pants are a perfect fit for you.");

ARMOR("cargo pants",	20, 280,C_PANTS,	COTTON,		MNULL,
    6,  6, -3,  0,   0,  0,  2,  0,  3, 14,	mfb(bp_legs), "\
A pair of pants lined with pockets, offering lots of storage.\n\
These cargo pants are a perfect fit for you.");

ARMOR("army pants",	10, 315,C_PANTS,	COTTON,		MNULL,
    6,  7, -2,  0,   0,  0,  3,  0,  4, 16,	mfb(bp_legs), "\
A tough pair of pants lined with pockets. Favored by the military.\n\
These army pants are a perfect fit for you.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("jumpsuit",	20, 200,C_BODY,		COTTON,		PLASTIC,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    6,  6, -3, -3,  0,  0,  1,  0,  1, 8,	mfb(bp_legs)|mfb(bp_torso), "\
A thin, short-sleeved jumpsuit; similar to those\n\
worn my prisoners. Provides decent storage and is\n\
not very encumbering.");

ARMOR("dress",		70, 180,C_BODY,		COTTON,		MNULL,
    8,  6, -5, -5,  3,  0,  1,  0,  2,  0,	mfb(bp_legs)|mfb(bp_torso), "\
A long cotton dress. Difficult to move in and lacks any storage space.");

ARMOR("chitinous armor", 1,1200,C_BODY,		FLESH,		MNULL,
   70, 10,  2, -5,  2,  8, 14,  0,  1,  0,	mfb(bp_legs)|mfb(bp_torso), "\
Leg and body armor made from the exoskeletons of insects. Light and durable.");

ARMOR("suit",		60, 180,C_BODY,		COTTON,		MNULL,
   10,  7, -5, -5,  1,  0,  1,  0,  2,  10,	mfb(bp_legs)|mfb(bp_torso)|mfb(bp_arms), "\
A full-body cotton suit. Makes the apocalypse a truly gentlemanly\n\
experience.");

ARMOR("hazmat suit",	10,1000,C_BODY,		PLASTIC,	MNULL,
   20, 8, -5,  -8,  4,  0,  0,10,  2, 12,	mfb(bp_legs)|mfb(bp_torso)|mfb(bp_arms), "\
A hazardous materials suit. Though quite bulky and cumbersome, wearing it\n\
will provide excellent protection against ambient radiation.");

ARMOR("plate mail",	 2, 700,C_BODY,		IRON,		MNULL,
   70,140,  8, -5,  5, 16, 20,  0,  2,  0,	mfb(bp_torso)|mfb(bp_legs)|mfb(bp_arms), "\
An extremely heavy ornamental suit of armor.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("t shirt",	80,  80,C_TORSO,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    3,  2, -5,  0,  1,  0,  0,  0,  1,  0,	mfb(bp_torso), "\
A short-sleeved cotton shirt.");

ARMOR("polo shirt",	65,  95,C_TORSO,	COTTON,		MNULL,
    3,  2, -5,  0,  1,  0,  1,  0,  1,  0,	mfb(bp_torso), "\
A short-sleeved cotton shirt, slightly thicker than a t-shirt.");

ARMOR("dress shirt",	60, 115,C_TORSO,	COTTON,		MNULL,
    3,  3, -5,  0,  1,  0,  1,  0,  1,  1,	mfb(bp_torso)|mfb(bp_arms), "\
A white button-down shirt with long sleeves. Looks professional!");

ARMOR("tank top",	50,  75,C_TORSO,	COTTON,		MNULL,
    1,  1, -5,  0,  0,  0,  0,  0,  0,  0,	mfb(bp_torso), "\
A sleeveless cotton shirt. Very easy to move in.");

ARMOR("sweatshirt",	75, 110,C_TORSO,	COTTON,		MNULL,
    9,  5, -5,  0,  1,  1,  1,  0,  3,  0,	mfb(bp_torso)|mfb(bp_arms), "\
A thick cotton shirt. Provides warmth and a bit of padding.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("sweater",	75, 105,C_TORSO,	WOOL,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    8,  5, -5,  0,  1,  1,  0,  0,  3,  0,	mfb(bp_torso)|mfb(bp_arms), "\
A wool shirt. Provides warmth.");

ARMOR("hoodie",		65, 130,C_TORSO,	COTTON,		MNULL,
   10,  5, -5,  0,  1,  1,  2,  0,  3,  9,	mfb(bp_torso)|mfb(bp_arms), "\
A sweatshirt with a hood and a \"kangaroo pocket\" in front for storage.");

ARMOR("light jacket",	50, 105,C_TORSO,	COTTON,		MNULL,
    6,  4, -5,  0,  1,  0,  2,  0,  2,  4,	mfb(bp_torso)|mfb(bp_arms), "\
A thin cotton jacket. Good for brisk weather.");

ARMOR("jean jacket",	35, 120,C_TORSO,	COTTON,		MNULL,
    7,  5, -3,  0,  1,  0,  4,  0,  2,  3,	mfb(bp_torso)|mfb(bp_arms), "\
A jacket made from denim. Provides decent protection from cuts.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("blazer",		35, 120,C_TORSO,	WOOL,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   10,  6, -4,  0,  2,  0,  3,  0,  3,  2,	mfb(bp_torso)|mfb(bp_arms), "\
A professional-looking wool blazer. Quite encumbersome.");

ARMOR("leather jacket",	30, 150,C_TORSO,	LEATHER,	MNULL,
   14, 14, -2,  1,  2,  1,  9,  1,  4,  4,	mfb(bp_torso)|mfb(bp_arms), "\
A jacket made from thick leather. Encumbersome, but offers excellent\n\
protection from cuts.");

ARMOR("kevlar vest",	30, 800,C_TORSO,	KEVLAR,		MNULL,
   24, 24,  6, -3,  2,  4, 22,  0,  4,  4,	mfb(bp_torso), "\
A heavy bulletproof vest. The best protection from cuts and bullets.");

ARMOR("rain coat",	50, 100,C_TORSO,	PLASTIC,	COTTON,
    9,  8, -4,  0,  2,  0,  3,  1,  2,  7,	mfb(bp_torso)|mfb(bp_arms), "\
A plastic coat with two very large pockets. Provides protection from rain.");

ARMOR("wool poncho",	15, 120,C_TORSO,	WOOL,		MNULL,
    7,  3, -5, -1,  0,  1,  2,  1,  3,  0,	mfb(bp_torso), "\
A simple wool garment worn over the torso. Provides a bit of protection.");

//     NAME		RARE	COLOR		MAT1		MAT2
ARMOR("trenchcoat",	25, 225,C_TORSO,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   10,  6, -5, -1,  1,  0,  1,  1,  2, 24,	mfb(bp_torso)|mfb(bp_arms), "\
A thin cotton trenchcoat, lined with pockets. Great for storage.");

//     NAME		RARE	COLOR		MAT1		MAT2
ARMOR("leather trenchcoat",	25, 225,C_TORSO,	LEATHER,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   16,  10, -5, -1,  2,  1,  9,  1,  3, 24,	mfb(bp_torso)|mfb(bp_arms), "\
A thick leather trenchcoat, lined with pockets. Great for storage.");


ARMOR("winter coat",	50, 160,C_TORSO,	COTTON,		MNULL,
   12,  6, -5, -2,  3,  3,  1,  1,  8, 12,	mfb(bp_torso)|mfb(bp_arms), "\
A padded coat with deep pockets. Very warm.");

ARMOR("fur coat",	 5, 550,C_TORSO,	WOOL,		FLESH,
   18, 12, -5, -5,  2,  4,  2,  2, 10,  4,	mfb(bp_torso)|mfb(bp_arms), "\
A fur coat with a couple small pockets. Extremely warm.");

ARMOR("peacoat",	30, 180,C_TORSO,	COTTON,		MNULL,
   16, 10, -4, -3,  2,  1,  2,  0,  7, 10,	mfb(bp_torso)|mfb(bp_arms), "\
A heavy cotton coat. Encumbersome, but warm and with deep pockets.");

ARMOR("utility vest",	15, 200,C_TORSO,	COTTON,		MNULL,
    4,  3, -3,  0,  0,  0,  1,  0,  1, 14,	mfb(bp_torso), "\
A light vest covered in pockets and straps for storage.");

ARMOR("belt rig",	10, 200,C_TORSO,	COTTON,		MNULL,
    4,  4, -3,  0,  0,  0,  1,  0,  1, 18,	mfb(bp_torso), "\
A light vest covered in webbing, pockets and straps.\n\
This variety is favoured by the military.");

ARMOR("lab coat",	20, 155,C_TORSO,	COTTON,		MNULL,
   11,  7, -3, -2,  1,  1,  2,  0,  1, 14,	mfb(bp_torso)|mfb(bp_arms), "\
A long white coat with several large pockets.");

// Fitted clothing

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("t shirt",	20,  80,C_TORSO,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    3,  2, -5,  0,  0,  0,  0,  0,  1,  0,	mfb(bp_torso), "\
A short-sleeved cotton shirt.\n\
This t-shirt is a perfect fit for you.");

ARMOR("polo shirt",	65,  95,C_TORSO,	COTTON,		MNULL,
    3,  2, -5,  0,  0,  0,  1,  0,  1,  0,	mfb(bp_torso), "\
A short-sleeved cotton shirt, slightly thicker than a t-shirt.\n\
This polo shirt is a perfect fit for you.");

ARMOR("hoodie",		10, 130,C_TORSO,	COTTON,		MNULL,
   10,  5, -5,  0,  0,  1,  2,  0,  3,  9,	mfb(bp_torso)|mfb(bp_arms), "\
A sweatshirt with a hood and a \"kangaroo pocket\" in front for storage.\n\
This hoodie is a perfect fit for you.");

ARMOR("sweatshirt",	75, 110,C_TORSO,	COTTON,		MNULL,
    9,  5, -5,  0,  0,  1,  1,  0,  3,  0,	mfb(bp_torso)|mfb(bp_arms), "\
A thick cotton shirt. Provides warmth and a bit of padding.\n\
This sweatshirt is a perfect fit for you.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("sweater",	75, 105,C_TORSO,	WOOL,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    8,  5, -5,  0,  0,  1,  0,  0,  3,  0,	mfb(bp_torso)|mfb(bp_arms), "\
A wool shirt. Provides warmth.\n\
This sweater is a perfect fit for you.");

ARMOR("light jacket",	50, 105,C_TORSO,	COTTON,		MNULL,
    6,  4, -5,  0,  0,  0,  2,  0,  2,  4,	mfb(bp_torso)|mfb(bp_arms), "\
A thin cotton jacket. Good for brisk weather.\n\
This jacket is a perfect fit for you.");

ARMOR("leather jacket", 5, 150,C_TORSO,        LEATHER,        MNULL,
   14, 14, -2,  1,  1,  1,  9,  1,  4,  4,      mfb(bp_torso)|mfb(bp_arms), "\
A jacket made from thick leather. Encumbersome, but offers excellent\n\
protection from cuts.\n\
This jacket is a perfect fit for you.");

//     NAME		RARE	COLOR		MAT1		MAT2
ARMOR("trenchcoat",	25, 225,C_TORSO,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   10,  6, -5, -1,  0,  0,  1,  1,  3, 24,	mfb(bp_torso)|mfb(bp_arms), "\
A long coat lines with pockets. Great for storage.\n\
This trenchcoat is a perfect fit for you.");

//     NAME		RARE	COLOR			MAT1		MAT2
ARMOR("leather trenchcoat",	25, 225,C_TORSO,        LEATHER, 	MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   16,  10, -5, -1,  1,  1,  9,  1,  3, 24,	mfb(bp_torso)|mfb(bp_arms), "\
A thick leather trenchcoat, lined with pockets. Great for storage.\n\
This trenchcoat is a perfect fit for you");

// arm guards
//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("soft arm sleeves",	40,  65,C_ARMS,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    0,  0, -5,  1,  0,  1,  1,  1,  2,  0,	mfb(bp_arms), "\
A pair of soft neoprene arm sleeves, often used in contact sports.");

ARMOR("hard arm guards",	20,  130,C_ARMS,	COTTON,		PLASTIC,
    1,  0, -5,  1,  1,  2,  2,  1,  2,  0,	mfb(bp_arms), "\
A pair of neoprene arm sleeves covered with molded plastic sheaths.");

ARMOR("chitin arm guards",	10,  200,C_ARMS,	FLESH,		MNULL,
    2,  0, -5,  1,  1,  3,  3,  2,  1,  0,	mfb(bp_arms), "\
A pair of arm guards made from the exoskeletons of insects. Light and durable.");

ARMOR("metal arm guards",	10,  400,C_ARMS,	IRON,		MNULL,
    1,  1, -5,  1,  1,  4,  4,  1,  0,  0,	mfb(bp_arms), "\
A pair of arm guards hammered out from metal. Very stylish.");


//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("light gloves",	35,  65,C_GLOVES,	COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    0,  0, -5,  1,  1,  0,  0,  0,  1,  0,	mfb(bp_hands), "\
A pair of thin cotton gloves. Often used as a liner beneath other gloves.");

ARMOR("mittens",	30,  40,C_GLOVES,	WOOL,		MNULL,
    0,  0, -5,  1,  8,  0,  1,  0,  5,  0,	mfb(bp_hands), "\
A pair of warm mittens. They are extremely encumbersome.");

ARMOR("wool gloves",	33,  50,C_GLOVES,	WOOL,		MNULL,
    1,  0, -5,  1,  3,  0,  1,  0,  3,  0,	mfb(bp_hands), "\
A thick pair of wool gloves. Encumbersome but warm.");

ARMOR("winter gloves",	40,  65,C_GLOVES,	COTTON,		MNULL,
    1,  0, -5,  1,  5,  1,  1,  0,  4,  0,	mfb(bp_hands), "\
A pair of padded gloves. Encumbersome but warm.");

ARMOR("leather gloves",	45,  85,C_GLOVES,	LEATHER,	MNULL,
    1,  1, -3,  2,  1,  0,  3,  0,  3,  0,	mfb(bp_hands), "\
A thin pair of leather gloves. Good for doing manual labor.");

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
A simple piece of cotton that straps over the mouth. Provides a small amount\n\
of protection from air-borne illness and dust.");

ARMOR("bandana",	35,  28,C_MOUTH,	COTTON, 	MNULL,
    1,  0, -4, -1,  0,  0,  0,  1,  2,  0,	mfb(bp_mouth), "\
A cotton bandana, worn over the mouth for warmth and minor protection from\n\
dust and other contaminants.");

ARMOR("scarf",		45,  40,C_MOUTH,	WOOL,   	MNULL,
    2,  3, -5, -3,  1,  1,  0,  2,  3,  0,	mfb(bp_mouth), "\
A long wool scarf, worn over the mouth for warmth.");

ARMOR("filter mask",	30,  80,C_MOUTH,	PLASTIC,	MNULL,
    3,  6,  1,  1,  2,  1,  1,  7,  2,  0,	mfb(bp_mouth), "\
A mask that straps over your mouth and nose and filters air. Protects from\n\
smoke, dust, and other contaminants quite well.");

ARMOR("gas mask",	10, 240,C_MOUTH,	PLASTIC,	MNULL,
    6,  8,  0, -3,  4,  1,  2, 16,  4,  0,	mfb(bp_mouth)|mfb(bp_eyes), "\
A full gas mask that covers the face and eyes. Provides excellent protection\n\
from smoke, teargas, and other contaminants.");

// Eyewear - Encumberment is its effect on your eyesight.
// Environment is the defense to your eyes from noxious fumes etc.


//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("eyeglasses",	90, 150,C_EYES,		GLASS,		PLASTIC,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    1,  0, -3, -2,  0,  0,  1,  1,  0,  0,	mfb(bp_eyes), "\
A pair of glasses for the near-sighted. Useless for anyone else.");

ARMOR("reading glasses",90,  80,C_EYES,		GLASS,		PLASTIC,
    1,  0, -3, -2,  0,  0,  1,  1,  0,  0,	mfb(bp_eyes), "\
A pair of glasses for the far-sighted. Useless for anyone else.");

ARMOR("safety glasses", 40, 100,C_EYES,		PLASTIC,	MNULL,
    1,  0, -5, -2,  0,  2,  4,  1,  0,  0,	mfb(bp_eyes), "\
A pair of plastic glasses, used in workshops, sports, chemistry labs, and\n\
many other places. Provides great protection from damage.");

ARMOR("swim goggles",	50, 110,C_EYES,		PLASTIC,	MNULL,
    1,  0, -5, -2,  2,  1,  2,  4,  1,  0,	mfb(bp_eyes), "\
A small pair of goggles. Distorts vision above water, but allows you to see\n\
much further under water.");

ARMOR("ski goggles",	30, 175,C_EYES,		PLASTIC,	MNULL,
    2,  1, -4, -2,  1,  1,  2,  6,  2,  0,	mfb(bp_eyes), "\
A large pair of goggles that completely seal off your eyes. Excellent\n\
protection from environmental dangers.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("welding goggles", 8, 240,C_EYES,		GLASS,  	STEEL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    2,  4, -1, -3,  6,  2,  5,  6,  1,  0,	mfb(bp_eyes), "\
A dark pair of goggles. They make seeing very difficult, but protects you\n\
from bright flashes.");

ARMOR("light amp goggles",1,920,C_EYES,		STEEL,		GLASS,
    3,  6,  1, -2,  2,  2,  3,  6,  2,  0,	mfb(bp_eyes), "\
A pair of goggles that amplify ambient light, allowing you to see in the\n\
dark.  You must be carrying a powered-on unified power supply, or UPS, to use\n\
them.");

ARMOR("monocle",	 2, 200,C_EYES,		GLASS,		PLASTIC,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    1,  0, -3, -2,  0,  0,  1,  1,  0,  0,	mfb(bp_eyes), "\
An essential article of the gentleman's apparel. Also negates near-sight.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("sunglasses",	90, 150,C_EYES,		GLASS,		PLASTIC,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    1,  0, -3, -2,  0,  0,  1,  1,  0,  0,	mfb(bp_eyes), "\
A pair of sunglasses, good for keeping the glare out of your eyes.");

// Headwear encumberment should ONLY be 0 if it's ok to wear with another
// Headwear environmental protection (ENV) drops through to eyes

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("baseball cap",	30,  35,C_HAT,		COTTON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    2,  1, -5,  0,  0,  0,  0,  2,  1,  0,	mfb(bp_head), "\
A Red Sox cap. It provides a little bit of warmth.");

ARMOR("boonie hat",	10,  55,C_HAT,		PLASTIC,	MNULL,
    2,  1, -5,  0,  0,  0,  1,  2,  2,  0,	mfb(bp_head), "\
Also called a \"bucket hat.\" Often used in the military.");

ARMOR("cotton hat",	45,  40,C_HAT,		COTTON,		MNULL,
    2,  1, -5,  0,  0,  0,  0,  0,  3,  0,	mfb(bp_head), "\
A snug-fitting cotton hat. Quite warm.");

ARMOR("knit hat",	25,  50,C_HAT,		WOOL,		MNULL,
    2,  1, -5,  0,  0,  1,  0,  0,  4,  0,	mfb(bp_head), "\
A snug-fitting wool hat. Very warm.");

ARMOR("hunting cap",	20,  80,C_HAT,		WOOL,		MNULL,
    3,  2, -5,  0,  0,  0,  1,  2,  6,  0,	mfb(bp_head), "\
A red plaid hunting cap with ear flaps. Notably warm.");

ARMOR("fur hat",	15, 120,C_HAT,		WOOL,		MNULL,
    4,  2, -5,  0,  1,  2,  2,  0,  8,  0,	mfb(bp_head), "\
A hat made from the pelts of animals. Extremely warm.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("hard hat",	50, 125,C_HAT,		PLASTIC,	MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    8,  4,  6,  0,  1,  4,  5,  0,  1,  0,	mfb(bp_head), "\
A hard plastic hat worn in constructions sites. Excellent protection from\n\
cuts and percussion.");
TECH( mfb(TEC_WBLOCK_1) );

ARMOR("bike helmet",	35, 140,C_HAT,		PLASTIC,	MNULL,
   12,  2,  4,  0,  1,  8,  2,  0,  2,  0,	mfb(bp_head), "\
A thick foam helmet. Designed to protect against percussion.");
TECH( mfb(TEC_WBLOCK_1) );

ARMOR("skid lid",	30, 190,C_HAT,		PLASTIC,	IRON,
   10,  5,  8,  0,  2,  6, 16,  0,  1,  0,	mfb(bp_head), "\
A small metal helmet that covers the head and protects against cuts and\n\
percussion.");
TECH( mfb(TEC_WBLOCK_1) );

ARMOR("baseball helmet",45, 195,C_HAT,		PLASTIC,	IRON,
   14,  6,  7, -1,  2, 10, 10,  1,  1,  0,	mfb(bp_head), "\
A hard plastic helmet which covers the head and ears. Designed to protect\n\
against a baseball to the head.");
TECH( mfb(TEC_WBLOCK_1) );

ARMOR("army helmet",	40, 480,C_HAT,		PLASTIC,	IRON,
   16,  8, 10, -1,  2, 12, 28,  0,  2,  0,	mfb(bp_head), "\
A heavy helmet which provides excellent protection from all sorts of damage.");
TECH( mfb(TEC_WBLOCK_1) );

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("riot helmet",	25, 420,C_HAT,		PLASTIC,	IRON,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   20,  7,  8, -1,  2,  6, 28,  2,  2,  0,	mfb(bp_head)|mfb(bp_eyes)|
						mfb(bp_mouth), "\
A helmet with a plastic shield that covers your entire face.");
TECH( mfb(TEC_WBLOCK_1) );

ARMOR("motorcycle helmet",40,325,C_HAT,		PLASTIC,	IRON,
   24,  8,  7, -1,  3,  8, 20,  1,  3,  0,	mfb(bp_head)|mfb(bp_mouth), "\
A helmet with covers your head and chin, leaving space in between for you to\n\
wear goggles.");
TECH( mfb(TEC_WBLOCK_1) );

ARMOR("chitinous helmet", 1, 380,C_HAT,		FLESH,		MNULL,
   22,  1,  2, -2,  4, 10, 14,  4,  3,  0,	mfb(bp_head)|mfb(bp_eyes)|
						mfb(bp_mouth), "\
A helmet made from the exoskeletons of insects. Covers the entire head; very\n\
light and durable.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("great helm",	  1,400,C_HAT,		IRON,		MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    20, 15, 10,  0,  4, 10, 15,  1,  1,  0,	mfb(bp_head)|mfb(bp_eyes)|
						mfb(bp_mouth), "\
A medieval helmet which provides excellent protection to the entire head, at\n\
the cost of great encumbrance.");
TECH( mfb(TEC_WBLOCK_1) );

ARMOR("top hat",	10,  55,C_HAT,		PLASTIC,	MNULL,
    2,  1, -5,  0,  0,  0,  1,  1,  1,  0,	mfb(bp_head), "\
The only hat for a gentleman. Look exquisite while laughing in the face\n\
of danger!");

ARMOR("backpack",	38, 210,C_STORE,	PLASTIC,	MNULL,
   14,  2, -4,  0,  2,  0,  0,  0,  0, 80,	mfb(bp_torso), "\
Provides more storage than any other piece of clothing.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("purse",		40,  75,C_STORE,	LEATHER,	MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
   10,  3,  2,  2,  2,  0,  0,  0,  0, 20,	mfb(bp_torso), "\
A bit encumbersome to wear, but provides lots of storage.");

ARMOR("messenger bag",	20, 110,C_STORE,	PLASTIC,	MNULL,
    8,  2,  1,  1,  1,  0,  0,  0,  0, 30,	mfb(bp_torso), "\
A bit encumbersome to wear, but provides lots of storage.");

ARMOR("fanny pack", 	10, 100,C_STORE,	PLASTIC,	MNULL,
    3,  1,  1,  2,  0,  0,  0,  0,  0,  6,	0, "\
Provides a bit of extra storage without encumbering you at all.");

ARMOR("holster",	 8,  90,C_STORE,	LEATHER,	MNULL,
    2,  2,  2, -1,  0,  0,  0,  0,  0,  3,	0, "\
Provides a bit of extra storage without encumbering you at all.");

//     NAME		RAR PRC	COLOR		MAT1		MAT2
ARMOR("bootstrap",	 3,  80,C_STORE, 	LEATHER,	MNULL,
// VOL WGT DAM HIT ENC RES CUT ENV WRM STO	COVERS
    1,  1, -1, -1,  0,  0,  0,  0,  1,  2,	mfb(bp_legs), "\
A small holster worn on the ankle.");

ARMOR("gold ring",	12, 600,C_DECOR,	SILVER,		MNULL,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,	0,	"\
A flashy gold ring. You can wear it if you like, but it won't provide\n\
any effects.");

ARMOR("silver necklace",14, 500,C_DECOR,	SILVER,		MNULL,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,	0,	"\
A nice silver necklace. You can wear it if you like, but it won't provide\n\
any effects.");

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
accuracy,recoil,count,des,effects) \
	index++;itypes.push_back(new it_ammo(index,rarity,price,name,des,'=',\
color,mat,volume,wgt,1,0,0,effects,ammo_type,dmg,AP,accuracy,recoil,range,count))

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("batteries",	50, 120,AT_BATT,	c_magenta,	IRON,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 1,  1,  0,  0,  0,  0,  0, 100, "\
A set of universal batteries. Used to charge almost any electronic device.",
0);

AMMO("thread",          40, 50, AT_THREAD,      c_magenta,      COTTON,
         1,  1,  0,  0,  0,  0,  0, 50, "\
A small quantity of thread that could be used to refill a sewing kit.",
0);

AMMO("duct tape",       60, 20, AT_NULL,    c_ltgray,       PLASTIC,
         2,  2,  0,  0,  0,  0,  0, 200, "\
A roll of incredibly strong tape. Its uses are innumerable.",
0);

AMMO("copper wire",       60, 20, AT_NULL,    c_ltgray,       PLASTIC,
         2,  2,  0,  0,  0,  0,  0, 200, "\
Plastic jacketed copper cable of the type used in small electronics.",
0);

AMMO("plutonium cell",	10,1500,AT_PLUT,	c_ltgreen,	STEEL,
	 1,  1,  0,  0,  0,  0,  0, 5, "\
A nuclear-powered battery. Used to charge advanced and rare electronics.",
0);

AMMO("nails",		35,  60,AT_NAIL,	c_cyan,		IRON,
         1,  8,  4,  3,  3, 40,  4, 100, "\
A box of nails, mainly useful with a hammer.",
0);

AMMO("BB",		 8,  50,AT_BB,		c_pink,		STEEL,
	 1,  6,  2,  0, 12, 20,  0, 200, "\
A box of small steel balls. They deal virtually no damage.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("wood arrow",       7,100,AT_ARROW,        c_green,        WOOD,
//	VOL WGT DMG  AP RNG ACC REC COUNT
         2, 60,  8,  1, 10, 18,  0,  10, "\
A sharpened arrow carved from wood. It's light-weight, does little damage,\n\
and is so-so on accuracy. Stands a good chance of remaining intact once\n\
fired.",
0);

AMMO("carbon fiber arrow",5,300,AT_ARROW,       c_green,        PLASTIC,
         2, 30, 12,  2, 15, 14,  0,   8, "\
High-tech carbon fiber shafts and 100 grain broadheads. Very light weight,\n\
fast, and notoriously fragile.",
0);

AMMO("wood crossbow bolt",8,100,AT_BOLT,	c_green,	WOOD,
	 1, 40, 10,  1, 10, 16,  0,  15, "\
A sharpened bolt carved from wood. It's lighter than a steel bolt, but does\n\
less damage and is less accurate. Stands a good chance of remaining intact\n\
once fired.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("steel crossbow bolt",7,400,AT_BOLT,	c_green,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 1, 90, 20,  3, 14, 12,  0,  10, "\
A sharp bolt made from steel. Deadly in skilled hands. Stands an excellent\n\
chance of remaining intact once fired.",
0);

AMMO("birdshot",	 8, 500,AT_SHOT,	c_red,		PLASTIC,
	 2, 25, 18,  0,  5,  2, 18,  25, "\
Weak shotgun ammunition. Designed for hunting birds and other small game, its\n\
applications in combat are very limited.",
0);

AMMO("00 shot",		 8, 800,AT_SHOT,	c_red,		PLASTIC,
	 2, 28, 50,  0,  6,  1, 26,  25, "\
A shell filled with iron pellets. Extremely damaging, plus the spread makes\n\
it very accurate at short range. Favored by SWAT forces.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("shotgun slug",	 6, 900,AT_SHOT,	c_red,		PLASTIC,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 2, 34, 50,  8, 12, 10, 28,  25, "\
A heavy metal slug used with shotguns to give them the range capabilities of\n\
a rifle. Extremely damaging but rather innaccurate. Works best in a shotgun\n\
with a rifled barrel.",
0);

AMMO("explosive slug",   0,1200,AT_SHOT,	c_red,		PLASTIC,
	 2, 30, 10,  0, 12, 12, 20,   5, "\
A shotgun slug loaded with concussive explosives. While the slug itself will\n\
not do much damage to its target, it will explode on contact.",
mfb(AMMO_EXPLOSIVE));

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO(".22 LR",		 9, 250,AT_22,		c_ltblue,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 2,  2, 11,  0, 13, 14,  6, 100, "\
One of the smallest calibers available, the .22 Long Rifle cartridge has\n\
maintained popularity for nearly two centuries. Its minimal recoil, low cost\n\
and low noise are offset by its paltry damage.",
0);

AMMO(".22 CB",		 8, 180,AT_22,		c_ltblue,	STEEL,
	 2,  2,  5,  0, 10, 16,  4, 100, "\
Conical Ball .22 is a variety of .22 ammunition with a very small propellant\n\
charge, generally with no gunpowder, resulting in a subsonic round. It is\n\
nearly silent, but is so weak as to be nearly useless.",
0);

AMMO(".22 rat-shot",	 2, 230,AT_22,		c_ltblue,	STEEL,
	 2,  2,  4,  0,  3,  2,  4, 100, "\
Rat-shot is extremely weak ammunition, designed for killing rats, snakes, or\n\
other small vermin while being unable to damage walls. It has an extremely\n\
short range and is unable to injure all but the smallest creatures.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("9mm",		 8, 300,AT_9MM,		c_ltblue,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 2,  7, 18,  2, 14, 16, 13,  50, "\
9 millimeter parabellum is generally regarded as the most popular handgun\n\
cartridge, used by the majority of US police forces. It is also a very\n\
popular round in sub-machine guns.",
0);

AMMO("9mm +P",		 8, 380,AT_9MM,		c_ltblue,	STEEL,
	 1,  7, 20,  4, 14, 15, 14,  25, "\
Attempts to improve the ballistics of 9mm ammunition lead to high pressure\n\
rounds. Increased velocity resullts in superior accuracy and damage.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("9mm +P+",		 8, 440,AT_9MM,		c_ltblue,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 1,  7, 22, 12, 16, 14, 15,  10, "\
A step beyond the high-pressure 9mm +P round, the +P+ is a very high pressure\n\
loading which offers a degree of armor-penetrating ability.",
0);

AMMO(".38 Special",	 7, 400,AT_38,		c_ltblue,	STEEL,
	 2, 10, 20,  0, 14, 16, 12,  50, "\
The .38 Smith & Wesson Special enjoyed popularity among US police forces\n\
throughout the 20th century. It is most commonly used in revolvers.",
0);

AMMO(".38 Super",	 7, 450,AT_38,		c_ltblue,	STEEL,
	 1,  9, 25,  4, 16, 14, 14,  25, "\
The .38 Super is a high-pressure load of the .38 Special caliber. It is a\n\
popular choice in pistol competions for its high accuracy, while its stopping\n\
power keeps it popular for self-defense.",
0);

AMMO("10mm Auto",	 4, 420,AT_40,		c_blue,		STEEL,
	 2,  9, 26, 10, 14, 18, 20,  50, "\
Originally used by the FBI, the organization eventually abandoned the round\n\
due to its high recoil. Although respected for its versatility and power, it\n\
has largely been supplanted by the downgraded .40 S&W.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO(".40 S&W",		 7, 450,AT_40,		c_blue,		STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 2,  9, 22,  2, 14, 15, 16,  50, "\
The .40 Smith & Wesson round was developed as an alternative to 10mm Auto for\n\
the FBI after they complained of high recoil. It is as accurate as 9mm, but\n\
has greater stopping power, leading to widespread use in law enforcement.",
0);

AMMO(".44 Magnum",	 7, 580,AT_44,		c_blue,		STEEL,
	 2, 15, 36,  2, 16, 16, 22,  50, "\
Described (in 1971) by Dirty Harry as \"the most powerful handgun in the\n\
world,\" the .44 Magnum gained widespead popularity due to its depictions in\n\
the media. In reality, its intense recoil makes it unsuitable in most cases.",
0);

AMMO(".45 ACP",		 7, 470,AT_45,		c_blue,		STEEL,
	 2, 10, 32,  2, 16, 18, 18,  50, "\
The .45 round was one of the most popular and powerful handgun rounds through\n\
the 20th century. It features very good accuracy and stopping power, but\n\
suffers from moderate recoil and poor armor penetration.",
0);

AMMO(".45 FMJ",		 4, 480,AT_45,		c_blue,		STEEL,
	 1, 13, 26, 20, 16, 18, 18,  25, "\
Full Metal Jacket .45 rounds are designed to overcome the poor armor\n\
penetration of the standard ACP round. However, they are less likely to\n\
expand upon impact, resulting in reduced damage overall.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO(".45 Super",	 5, 520,AT_45,		c_blue,		STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 1, 11, 34,  8, 18, 16, 20,  10, "\
The .45 Super round is an updated variant of .45 ACP. It is overloaded,\n\
resulting in a great increase in muzzle velocity. This translates to higher\n\
accuracy and range, a minor armor piercing capability, and greater recoil.",
0);

AMMO("5.7x28mm",	 3, 500,AT_57,		c_dkgray,	STEEL,
	 3,  2, 14, 30, 14, 12,  6, 100, "\
The 5.7x28mm round is a proprietary round developed by FN Hestal for use in\n\
their P90 SMG. While it is a very small round, comparable in power to .22,\n\
it features incredible armor-piercing capabilities and very low recoil.",
0);

AMMO("4.6x30mm",	 2, 520,AT_46,		c_dkgray,	STEEL,
	 3,  1, 13, 35, 14, 12,  6, 100, "\
Designed by Heckler & Koch to compete with the 5.7x28mm round, 4.6x30mm is,\n\
like the 5.7, designed to minimize weight and recoil while increasing\n\
penetration of body armor. Its low recoil makes it ideal for automatic fire.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("7.62x39mm M43",	 6, 500,AT_762,		c_dkgray,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 3,  7, 25,  8, 30, 19, 24,  80, "\
Designed during World War II by the Soviet Union, the popularity of the AK-47\n\
and the SKS contributed to the widespread adaption of the 7.62x39mm rifle\n\
round. However, due to its lack of yaw, this round deals less damage than most."
,
0);

AMMO("7.62x39mm M67",	 7, 650,AT_762,		c_dkgray,	STEEL,
	 3,  8, 28, 10, 30, 17, 25,  80, "\
The M67 variant of the popular 7.62x39mm rifle round was designed to improve\n\
yaw. This causes the round to tumble inside a target, causing significantly\n\
more damage. It is still outdone by shattering rounds.",
0);

AMMO(".223 Remington",	 8, 620,AT_223,		c_dkgray,	STEEL,
	 2,  2, 36,  2, 36, 13, 30,  40, "\
The .223 rifle round is a civilian variant of the 5.56 NATO round. It is\n\
designed to tumble or fragment inside a target, dealing devastating damage.\n\
The lower pressure of the .223 compared to the 5.56 results in lower accuracy."
,
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO("5.56 NATO",	 6, 650,AT_223,		c_dkgray,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 2,  4, 40,  8, 38, 10, 32,  40, "\
This rifle round has enjoyed widespread use in NATO countries, thanks to its\n\
very light weight and high damage. It is designed to shatter inside a\n\
target, inflicting massive damage.",
0);

AMMO("5.56 incendiary",	 2, 840,AT_223,		c_dkgray,	STEEL,
	 2,  4, 28, 18, 36, 11, 32, 30, "\
A variant of the widely-used 5.56 NATO round, incendiary rounds are designed\n\
to burn hotly upon impact, piercing armor and igniting flammable substances.",
mfb(AMMO_INCENDIARY));

AMMO(".270 Winchester",	 8, 600,AT_3006,	c_dkgray,	STEEL,
	 1,  7, 42,  4, 80,  6, 34,  20, "\
Based off the military .30-03 round, the .270 rifle round is compatible with\n\
most guns that fire .30-06 rounds. However, it is designed for hunting, and\n\
is less powerful than the military rounds, with nearly no armor penetration.",
0);

//  NAME		RAR PRC TYPE		COLOR		MAT
AMMO(".30-06 AP",	 4, 650,AT_3006,	c_dkgray,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 1, 12, 50, 30, 90,  7, 36,  10, "\
The .30-06 is a very powerful rifle round designed for long-range use. Its\n\
stupendous accuracy and armor piercing capabilities make it one of the most\n\
deadly rounds available, offset only by its drastic recoil and noise.",
0);

AMMO(".30-06 incendiary", 1, 780,AT_3006,	c_dkgray,	STEEL,
	  1, 12, 35, 50, 90,  8, 35,  5, "\
A variant of the powerful .30-06 sniper round, incendiary rounds are designed\n\
to burn hotly upon impact, piercing armor and igniting flammable substances.",
mfb(AMMO_INCENDIARY));

AMMO(".308 Winchester",	 7, 620,AT_308,		c_dkgray,	STEEL,
	 1,  9, 36,  2, 65,  7, 33,  20, "\
The .308 Winchester is a rifle round, the commercial equivalent of the\n\
military 7.62x51mm round. Its high accuracy and phenominal damage have made\n\
it the most poplar hunting round in the world.",
0);

AMMO("7.62x51mm",	 6, 680,AT_308,		c_dkgray,	STEEL,
	 1,  9, 44,  8, 75,  6, 34,  20, "\
The 7.62x51mm largely replaced the .30-06 round as the standard military\n\
rifle round. It is lighter, but offers similar velocities, resulting in\n\
greater accuracy and reduced recoil.",
0);

//  NAME		   RAR PRC TYPE		COLOR		MAT
AMMO("7.62x51mm incendiary",6, 740,AT_308,	c_dkgray,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	  1,  9, 30, 25, 75,  6, 32,  10, "\
A variant of the powerful 7.62x51mm round, incendiary rounds are designed\n\
to burn hotly upon impact, piercing armor and igniting flammable substances.",
mfb(AMMO_INCENDIARY));

AMMO("fusion pack",	 2, 800,AT_FUSION,	c_ltgreen,	PLASTIC,
	 1,  2, 12, 15, 30,  4,  0,  20, "\
In the middle of the 21st Century, military powers began to look towards\n\
energy based weapons. The result was the standard fusion pack, capable of\n\
delivering bolts of superheated gas at near light speed with no recoil.",
mfb(AMMO_INCENDIARY));

AMMO("40mm concussive",     10,400,AT_40MM,	c_ltred,	STEEL,
	  1,200,  5,  0, 40,  8, 15,  4, "\
A 40mm grenade with a concussive explosion.",
mfb(AMMO_EXPLOSIVE));

//  NAME		   RAR PRC TYPE		COLOR		MAT
AMMO("40mm frag",           8, 450,AT_40MM,	c_ltred,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	  1,220,  5,  0, 40,  8, 15,  4, "\
A 40mm grenade with a small explosion and a high number of damaging fragments.",
mfb(AMMO_FRAG));

AMMO("40mm incendiary",     6, 500,AT_40MM,	c_ltred,	STEEL,
	  1,200,  5,  0, 40,  8, 15,  4, "\
A 40mm grenade with a small napalm load, designed to create a burst of flame.",
mfb(AMMO_NAPALM));

AMMO("40mm teargas",        5, 450,AT_40MM,	c_ltred,	STEEL,
	  1,210,  5,  0, 40,  8, 15,  4, "\
A 40mm grenade with a teargas load. It will burst in a cloud of highly\n\
incapacitating gas.",
mfb(AMMO_TEARGAS));

AMMO("40mm smoke cover",    4, 350,AT_40MM,	c_ltred,	STEEL,
	  1,210,  5,  0, 40,  8, 15,  6, "\
A 40mm grenade with a smoke load. It will burst in a cloud of harmless gas,\n\
and will also leave a streak of smoke cover in its wake.",
mfb(AMMO_SMOKE)|mfb(AMMO_TRAIL));

//  NAME		   RAR PRC TYPE		COLOR		MAT
AMMO("40mm flashbang",      8, 400,AT_40MM,	c_ltred,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	  1,210,  5,  0, 40,  8, 15,  6, "\
A 40mm grenade with a flashbang load. It will detonate with a blast of light\n\
and sound, designed to blind, deafen, and disorient anyone nearby.",
mfb(AMMO_FLASHBANG));

AMMO("H&K 12mm",	 2, 500,AT_12MM,		c_red,	STEEL,
	 1,  10, 25, 12, 70,  9, 7,  20, "\
The Heckler & Koch 12mm projectiles are used in H&K railguns. It's made of a\n\
ferromagnetic metal, probably cobalt.",
0);

AMMO("hydrogen",	 8, 800,AT_PLASMA,	c_green,	STEEL,
	 10,  25, 35, 14,12,  4,  0,  25, "\
A canister of hydrogen. With proper equipment, it could be heated to plasma.",
mfb(AMMO_INCENDIARY));

// The following ammo type is charger rounds and subject to change wildly
//  NAME		   RAR PRC TYPE		COLOR		MAT
AMMO("charge",	     0,  0,AT_NULL,	c_red,		MNULL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	  0,  0,  5,  0, 30,  8,  0, 1, "\
A weak plasma charge.",
0);

AMMO("shotgun hull",		 10, 80,AT_NULL,	c_red,		PLASTIC,
	 0, 0, 0,  0,  0, 0,  0, 200, "\
An empty hull from a shotgun round.",
0);

AMMO("9mm casing",		 10, 30,AT_NULL,		c_ltblue,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 0,  0,  0,  0, 0,  0,  0,  100, "\
An empty casing from a 9mm round.",
0);

AMMO(".38 casing",	 10, 40,AT_NULL,		c_ltblue,	STEEL,
	 0, 0, 0,  0, 0, 0, 0, 200, "\
An empty casing from a .38 round.",
0);

AMMO(".40 casing",		 10, 45,AT_NULL,		c_blue,		STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 0, 0, 0,  0, 0, 0, 0, 200, "\
An empty casing from a .40 round.",
0);

AMMO(".44 casing",	 10, 58,AT_NULL,		c_blue,		STEEL,
	 0, 0, 0,  0, 0, 0, 0, 200, "\
An empty casing from a .44 round.",
0);

AMMO(".45 casing",		 10, 47,AT_NULL,		c_blue,		STEEL,
	 0, 0, 0,  0, 0, 0, 0, 200, "\
An empty casing from a .45 round.",
0);

AMMO("5.7x28mm casing",	 10, 50,AT_NULL,		c_dkgray,	STEEL,
	 0, 0, 0, 0, 0,  0, 0, 200, "\
An empty casing from a 5.7x28mm round.",
0);

AMMO("4.6x30mm casing",	 10, 52,AT_NULL,		c_dkgray,	STEEL,
	 0, 0, 0, 0, 0,  0, 0, 200, "\
An empty casing from a 4.6x30mm round.",
0);

AMMO("7.62x39mm casing",	 10, 50,AT_NULL,		c_dkgray,	STEEL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 0, 0, 0, 0, 0, 0, 0, 200, "\
An empty casing from a 7.62x39mm round.",
0);

AMMO(".223 casing",	 10, 72,AT_NULL,		c_dkgray,	STEEL,
	 0,  0, 0,  0, 0, 0, 0, 200, "\
An empty casing from a .223 round.",
0);

AMMO(".30-06 casing",	 10, 90,AT_NULL,	c_dkgray,	STEEL,
	 0,  0, 0, 0, 0, 0, 0, 200, "\
An empty casing from a .30-06 round.",
0);

AMMO(".308 casing",	 10, 92,AT_NULL,		c_dkgray,	STEEL,
	 0,  0, 0,  0,  0, 0, 0, 200, "\
An empty casing from a .308 round.",
0);

AMMO("40mm canister",	 10, 92,AT_NULL,		c_dkgray,	STEEL,
	 0,  0, 0,  0,  0, 0, 0, 20, "\
A large canister from a spent 40mm grenade.",
0);

AMMO("gunpowder",	 10, 30,AT_NULL,		c_dkgray,	POWDER,
	 0,  0, 0,  0,  0, 0, 0, 200, "\
Firearm quality gunpowder.",
0);

AMMO("shotgun primer",	 10, 30,AT_NULL,		c_dkgray,	STEEL,
	 0,  0, 0, 0,  0, 0, 0, 200, "\
Primer from a shotgun round.",
0);

AMMO("small pistol primer",	 15, 40,AT_NULL,		c_dkgray,	STEEL,
	 0,  0, 0, 0,  0, 0, 0, 200, "\
Primer from a small caliber pistol round.",
0);

AMMO("large pistol primer",	 15, 60,AT_NULL,		c_dkgray,	STEEL,
	 0,  0, 0,  0,  0, 0, 0, 200, "\
Primer from a large caliber pistol round.",
0);

AMMO("small rifle primer",	 10, 60,AT_NULL,		c_dkgray,	STEEL,
	 0,  0,  0, 0,  0, 0, 0, 200, "\
Primer from a small caliber rifle round.",
0);

AMMO("large rifle primer",	 10, 80,AT_NULL,		c_dkgray,	STEEL,
	 0,  0,  0, 0,  0, 0, 0, 200, "\
Primer from a large caliber rifle round.",
0);

AMMO("lead",	 10, 50,AT_NULL,		c_dkgray,	STEEL,
	 0,  0,  0, 0,  0, 0, 0, 200, "\
Assorted bullet materials, useful in constructing a variety of ammunition.",
0);

AMMO("incendiary",	 0, 100,AT_NULL,		c_dkgray,	STEEL,
	 0,  0,  0, 0,  0, 0, 0, 200, "\
Material from an incendiary round, useful in constructing incendiary\n\
ammunition.",
0);


// FUEL
// Fuel is just a special type of ammo; liquid
#define FUEL(name,rarity,price,ammo_type,color,dmg,AP,range,accuracy,recoil,\
             count,des,effects) \
	index++;itypes.push_back(new it_ammo(index,rarity,price,name,des,'~',\
color,LIQUID,1,1,0,0,0,effects,ammo_type,dmg,AP,accuracy,recoil,range,count))
FUEL("gasoline",	0,  50,   AT_GAS,	c_ltred,
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

#define GUN(name,rarity,price,color,mat1,mat2,skill,ammo,volume,wgt,melee_dam,\
to_hit,dmg,accuracy,recoil,durability,burst,clip,reload_time,des,flags) \
	index++;itypes.push_back(new it_gun(index,rarity,price,name,des,'(',\
color,mat1,mat2,volume,wgt,melee_dam,0,to_hit,flags,skill,ammo,dmg,accuracy,\
recoil,durability,burst,clip,reload_time))

//  NAME		RAR PRC COLOR		MAT1	MAT2
GUN("nail gun",		12, 100,c_ltblue,	IRON,	MNULL,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP RELOAD
	sk_pistol,	AT_NAIL, 4, 22, 12,  1,  0, 20,  0,  8,  5, 100, 450, "\
A tool used to drive nails into wood or other material. It could also be\n\
used as a ad-hoc weapon, or to practice your handgun skill up to level 1.",
mfb(IF_MODE_BURST));

GUN("BB gun",		10, 100,c_ltblue,	IRON,	WOOD,
	sk_rifle,	AT_BB,	 8, 16,  9,  2,  0,  6, -5,  7,  0, 20, 500, "\
Popular among children. It's fairly accurate, but BBs deal nearly no damage.\n\
It could be used to practice your rifle skill up to level 1.",
0);

GUN("crossbow",		 2,1000,c_green,	IRON,	WOOD,
	sk_archery,	AT_BOLT, 6,  9, 11,  1,  0, 18,  0,  6,  0,  1, 800, "\
A slow-loading hand weapon that launches bolts. Stronger people can reload\n\
it much faster. Bolts fired from this weapon have a good chance of remaining\n\
intact for re-use.",
mfb(IF_STR_RELOAD));

//  NAME		RAR PRC COLOR		MAT1	MAT2
GUN("compound bow",      2,1400,c_yellow,       STEEL,  PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP RELOAD
        sk_archery,     AT_ARROW,12, 8,  8,  1,  0, 20,  0,  6,  0,  1, 100, "\
A bow with wheels that fires high velocity arrows. Weaker people can use\n\
compound bows more easily. Arrows fired from this weapon have a good chance\n\
of remaining intact for re-use.",
mfb(IF_STR8_DRAW)|mfb(IF_RELOAD_AND_SHOOT));

GUN("longbow",           5, 800,c_yellow,       WOOD,   MNULL,
        sk_archery,     AT_ARROW,8, 4, 10,  0,  0, 12,  0,  6,  0,  1,  80, "\
A six-foot wooden bow that fires feathered arrows. This takes a fair amount\n\
of strength to draw. Arrows fired from this weapon have a good chance of\n\
remaining intact for re-use.",
mfb(IF_STR10_DRAW)|mfb(IF_RELOAD_AND_SHOOT));

GUN("pipe rifle: .22",	0,  800,c_ltblue,	IRON,	WOOD,
sk_rifle,	AT_22,	 9, 13, 10,  2, -2, 15,  2,  6,  0,  1, 250, "\
A home-made rifle. It is simply a pipe attached to a stock, with a hammer to\n\
strike the single round it holds.",
0);

//  NAME		RAR PRC COLOR		MAT1	MAT2
GUN("pipe rifle: 9mm",	0,  900,c_ltblue,	IRON,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP RELOAD
	sk_rifle,	AT_9MM,	10, 16, 10,  2, -2, 15,  2,  6,  0,  1, 250, "\
A home-made rifle. It is simply a pipe attached to a stock, with a hammer to\n\
strike the single round it holds.",
0);

GUN("pipe SMG: 9mm",	0, 1050,c_ltblue,	IRON,	WOOD,
	sk_smg,		AT_9MM,  5,  8,  6, -1,  0, 30,  6,  5,  4, 10, 400, "\
A home-made machine pistol. It features a rudimentary blowback system, which\n\
allows for small bursts.",
mfb(IF_MODE_BURST));

GUN("pipe SMG: .45",	0, 1150,c_ltblue,	IRON,	WOOD,
	sk_smg,		AT_45,	 6,  9,  7, -1,  0, 30,  6,  5,  3,  8, 400, "\
A home-made machine pistol. It features a rudimentary blowback system, which\n\
allows for small bursts.",
mfb(IF_MODE_BURST));

GUN("SIG Mosquito",	 5,1200,c_dkgray,	STEEL,	PLASTIC,
	sk_pistol,	AT_22,	 1,  6,  9,  1,  1, 28,  4,  8,  0, 10, 350, "\
A popular, very small .22 pistol. \"Ergonomically designed to give the best\n\
shooting experience.\" --SIG Sauer official website",
0);

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("S&W 22A",		 5,1250,c_dkgray,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_pistol,	AT_22,	 1, 10,  9,  1,  1, 25,  5,  7,  0, 10, 300, "\
A popular .22 pistol. \"Ideal for competitive target shooting or recreational\n\
shooting.\" --Smith & Wesson official website",
0);

GUN("Glock 19",		 7,1400,c_dkgray,	STEEL,	PLASTIC,
	sk_pistol,	AT_9MM,	 2,  5,  8,  1,  0, 24,  6,  6,  0, 15, 300, "\
Possibly the most popular pistol in existance. The Glock 19 is often derided\n\
for its plastic contruction, but it is easy to shoot.",
0);

GUN("USP 9mm",		 6,1450,c_dkgray,	STEEL,	PLASTIC,
	sk_pistol,	AT_9MM,	 2,  6,  8,  1, -1, 25,  5,  9,  0, 15, 350, "\
A popular 9mm pistol, widely used among law enforcement. Extensively tested\n\
for durability, it has been found to stay accurate even after subjected to\n\
extreme abuse.",
0);

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("S&W 619",		 4,1450,c_dkgray,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_pistol,	AT_38,	 2,  9,  9,  1,  2, 23,  4,  8,  0,  7, 75, "\
A seven-round .38 revolver sold by Smith & Wesson. It features a fixed rear\n\
sight and a reinforced frame.",
mfb(IF_RELOAD_ONE));

GUN("Taurus Pro .38",	 4,1500,c_dkgray,	STEEL,	PLASTIC,
	sk_pistol,	AT_38,	 2,  6,  8,  1,  1, 22,  6,  7,  0, 10, 350, "\
A popular .38 pistol. Designed with numerous safety features and built from\n\
high-quality, durable materials.",
0);

GUN("SIG Pro .40",	 4,1500,c_dkgray,	STEEL,	PLASTIC,
	sk_pistol,	AT_40,	 2,  6,  8,  1,  1, 22,  8,  7,  0, 12, 350, "\
Originally marketed as a lightweight and compact alternative to older SIG\n\
handguns, the Pro .40 is popular among European police forces.",
0);

GUN("S&W 610",		 2,1460,c_dkgray,	STEEL,	WOOD,
	sk_pistol,	AT_40,	 2, 10, 10,  1,  2, 23,  6,  8,  0,  6, 60, "\
The Smith and Wesson 610 is a classic six-shooter revolver chambered for 10mm\n\
rounds, or for S&W's own .40 round.",
mfb(IF_RELOAD_ONE));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("Ruger Redhawk",	 3,1560,c_dkgray,	STEEL,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_pistol,	AT_44,	 2, 12, 10,  1,  2, 21,  6,  8,  0,  6, 80, "\
One of the most powerful handguns in the world when it was released in 1979,\n\
the Redhawk offers very sturdy contruction, with an appearance that is\n\
reminiscent of \"Wild West\" revolvers.",
mfb(IF_RELOAD_ONE));

GUN("Desert Eagle .44",	 2,1750,c_dkgray,	STEEL,	PLASTIC,
	sk_pistol,	AT_44,	 4, 17, 14,  1,  4, 35,  3,  7,  0, 10, 400, "\
One of the most recognizable handguns due to its popularity in movies, the\n\
\"Deagle\" is better known for its menacing appearance than its performace.\n\
It's highly innaccurate, but its heavy weight reduces recoil.",
0);

GUN("USP .45",		 6,1600,c_dkgray,	STEEL,	PLASTIC,
	sk_pistol,	AT_45,	 2,  7,  9,  1,  1, 25,  8,  9,  0, 12, 350, "\
A popular .45 pistol, widely used among law enforcement. Extensively tested\n\
for durability, it has been found to stay accurate even after subjected to\n\
extreme abuse.",
0);

GUN("M1911",		 5,1680,c_ltgray,	STEEL,	PLASTIC,
	sk_pistol,	AT_45,	 3, 10, 12,  1,  6, 25,  9,  7,  0,  7, 300, "\
The M1911 was the standard-issue sidearm from the US Military for most of the\n\
20th Century. It remains one of the most popular .45 pistols today.",
0);

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("FN Five-Seven",	 2,1550,c_ltgray,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_pistol,	AT_57,	 2,  5,  6,  0,  2, 13,  6,  8,  0, 20, 300, "\
Designed to work with FN's proprietary 5.7x28mm round, the Five-Seven is a\n\
lightweight pistol with a very high capacity, best used against armored\n\
opponents.",
0);

GUN("H&K UCP",		 2,1500,c_ltgray,	STEEL,	PLASTIC,
	sk_pistol,	AT_46,	 2,  5,  6,  0,  2, 12,  6,  8,  0, 20, 300, "\
Designed to work with H&K's proprietary 4.6x30mm round, the UCP is a small\n\
pistol with a very high capacity, best used against armored opponents.",
0);

GUN("sawn-off shotgun",	 1, 700,c_red,	IRON,	WOOD,
	sk_shotgun,	AT_SHOT, 6, 10, 14,  2,  4, 40, 15,  4,  0,  2, 100, "\
The barrels of shotguns are often sawed in half to make it more maneuverable\n\
and concealable. This has the added effect of reducing accuracy greatly.",
mfb(IF_RELOAD_ONE));

GUN("sawn-off Saiga 12",	 1, 700,c_red,	IRON,	WOOD,
	sk_shotgun,	AT_SHOT, 6, 10, 14,  2,  4, 40, 15,  4,  0,  10, 100, "\
The Saiga-12 shotgun is designed on the same Kalishnikov pattern as the AK47\n\
rifle. It reloads with a magazine, rather than one shell at a time like most\n\
shotguns. This one has had the barrel cut short, vastly reducing accuracy\n\
but making it more portable",
mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("single barrel shotgun",1,600,c_red,IRON,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_shotgun,	AT_SHOT,12, 20, 14,  3,  0,  6,  5,  6,  0,  1, 100, "\
An old shotgun, possibly antique. It is little more than a barrel, a wood\n\
stock, and a hammer to strike the cartridge. Its simple design keeps it both\n\
light and accurate.",
0);

GUN("double barrel shotgun",2,1050,c_red,IRON,	WOOD,
	sk_shotgun,	AT_SHOT,12, 26, 15,  3,  0,  7,  4,  7,  2,  2, 100, "\
An old shotgun, possibly antique. It is little more than a pair of barrels,\n\
a wood stock, and a hammer to strike the cartridges.",
mfb(IF_RELOAD_ONE)|mfb(IF_MODE_BURST));

GUN("Remington 870",	 9,2200,c_red,	STEEL,	PLASTIC,
	sk_shotgun,	AT_SHOT,16, 30, 17,  3,  5, 10,  0,  8,  3,  6, 100, "\
One of the most popular shotguns on the market, the Remington 870 is used by\n\
hunters and law enforcement agencies alike thanks to its high accuracy and\n\
muzzle velocity.",
mfb(IF_RELOAD_ONE)|mfb(IF_MODE_BURST));

GUN("Mossberg 500",	 5,2250,c_red,	STEEL,	PLASTIC,
	sk_shotgun,	AT_SHOT,15, 30, 17,  3,  0, 13, -2,  9,  3,  8, 80, "\
The Mossberg 500 is a popular series of pump-action shotguns, often acquired\n\
for military use. It is noted for its high durability and low recoil.",
mfb(IF_RELOAD_ONE)|mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("Saiga-12",		 3,2300,c_red,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_shotgun,	AT_SHOT,15, 36, 17,  3,  0, 17,  2,  7,  4, 10, 500, "\
The Saiga-12 shotgun is designed on the same Kalishnikov pattern as the AK47\n\
rifle. It reloads with a magazine, rather than one shell at a time like most\n\
shotguns.",
mfb(IF_MODE_BURST));

GUN("American-180",	 2,1600,c_cyan, STEEL,	MNULL,
	sk_smg,		AT_22,  12, 23, 11,  0,  2, 20,  0,  6, 30,165, 500, "\
The American-180 is a submachine gun developed in the 1960's which fires .22\n\
LR, unusual for an SMG. Though the round is low-powered, the high rate of\n\
fire and large magazine makes the 180 a formidable weapon.",
mfb(IF_MODE_BURST));

GUN("Uzi 9mm",		 8,2080,c_cyan,	STEEL,	MNULL,
	sk_smg,		AT_9MM,	 6, 29, 10,  1,  0, 25, -2,  7, 12, 32, 450, "\
The Uzi 9mm has enjoyed immense popularity, selling more units than any other\n\
submachine gun. It is widely used as a personal defense weapon, or as a\n\
primary weapon by elite frontline forces.",
mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("TEC-9",		10,1750,c_cyan,	STEEL,	MNULL,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_smg,		AT_9MM,	 5, 12,  9,  1,  3, 24,  0,  6,  8, 32, 400, "\
The TEC-9 is a machine pistol made of cheap polymers and machine stamped\n\
parts. Its rise in popularity among criminals is largely due to its\n\
intimidating looks and low price.",
mfb(IF_MODE_BURST));

GUN("Calico M960",	 6,2400,c_cyan,	STEEL,	MNULL,
	sk_smg,		AT_9MM,	 7, 19,  9,  1, -3, 28, -4,  6, 20, 50, 500, "\
The Calico M960 is an automatic carbine with a unique circular magazine which\n\
allows for high capacities and reduced recoil.",
mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("H&K MP5",		12,2800,c_cyan,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_smg,		AT_9MM,	12, 26, 10,  2,  1, 18, -3,  8,  8, 30, 400, "\
The Heckler & Koch MP5 is one of the most widely-used submachine guns in the\n\
world, and has been adopted by special police forces and militaries alike.\n\
Its high degree of accuracy and low recoil are universally praised.",
mfb(IF_MODE_BURST));

GUN("MAC-10",		14,1800,c_cyan,	STEEL,	MNULL,
	sk_smg,		AT_45,	 4, 25,  8,  1, -4, 28,  0,  7, 30, 30, 450, "\
The MAC-10 is a popular machine pistol originally designed for military use.\n\
For many years they were the most inexpensive automatic weapon in the US, and\n\
enjoyed great popularity among criminals less concerned with quality firearms."
,
mfb(IF_MODE_BURST));

GUN("H&K UMP45",	12,3000,c_cyan,	STEEL,	PLASTIC,
	sk_smg,		AT_45,	13, 20, 11,  1,  0, 13, -3,  8,  4, 25, 450, "\
Developed as a successor to the MP5 submachine gun, the UMP45 retains the\n\
earlier model's supreme accuracy and low recoil, but in the higher .45 caliber."
,
mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("TDI Vector",	 4,4200,c_cyan,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_smg,		AT_45,	13, 20,  9,  0, -2, 15,-14,  7,  8, 30, 450, "\
The TDI Vector is a submachine gun with a unique in-line design which makes\n\
recoil very managable, even in the powerful .45 caliber.",
mfb(IF_MODE_BURST));

GUN("FN P90",		 7,4000,c_cyan,	STEEL,	PLASTIC,
	sk_smg,		AT_57,	14, 22, 10,  1,  0, 22, -8,  8, 20, 50, 500, "\
The first in a new genre of guns, termed \"personal defense weapons.\"  FN\n\
designed the P90 to use their proprietary 5.7x28mm ammunition.  It is made\n\
for firing bursts manageably.",
mfb(IF_MODE_BURST));

GUN("H&K MP7",		 5,3400,c_cyan,	STEEL,	PLASTIC,
	sk_smg,		AT_46,	 7, 17,	 7,  1,  0, 21,-10,  8, 20, 20, 450, "\
Designed by Heckler & Koch as a competitor to the FN P90, as well as a\n\
successor to the extremely popular H&K MP5. Using H&K's proprietary 4.6x30mm\n\
ammunition, it is designed for burst fire.",
mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("Marlin 39A",	14,1600,c_brown,IRON,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP RELOAD
	sk_rifle,	AT_22,	 11, 26, 12,  3,  3, 10, -5,  8,  0, 19,  90, "\
The oldest and longest-produced shoulder firearm in the world. Though it\n\
fires the weak .22 round, it is highly accurate and damaging, and has\n\
essentially no recoil.",
mfb(IF_RELOAD_ONE));

GUN("Ruger 10/22",	12,1650,c_brown,IRON,	WOOD,
	sk_rifle,	AT_22,	11, 23, 12,  3,  0,  8, -5,  8,  0, 10, 500, "\
A popular and highly accurate .22 rifle. At the time of its introduction in\n\
1964, it was one of the first modern .22 rifles designed for quality, and not\n\
as a gun for children.",
0);

GUN("Browning BLR",	 8,3500,c_brown,IRON,	WOOD,
	sk_rifle,	AT_3006,12, 28, 12,  3, -3,  6, -4,  7,  0,  4, 100, "\
A very popular rifle for hunting and sniping. Its low ammo capacity is\n\
offset by the very powerful .30-06 round it fires.",
mfb(IF_RELOAD_ONE));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("Remington 700",	14,3200,c_brown,IRON,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_rifle,	AT_3006,12, 34, 13,  3,  7,  9, -3,  8,  0,  4, 75, "\
A very popular and durable hunting or sniping rifle. Popular among SWAT\n\
and US Marine snipers. Highly damaging, but perhaps not as accurate as the\n\
competing Browning BLR.",
mfb(IF_RELOAD_ONE));

GUN("SKS",		12,3000,c_brown,IRON,	WOOD,
	sk_rifle,	AT_762,	12, 34, 13,  3,  0,  5, -4,  8,  0, 10, 450, "\
Developed by the Soviets in 1945, this rifle was quickly replaced by the\n\
full-auto AK47. However, due to its superb accuracy and low recoil, this gun\n\
maintains immense popularity.",
0);

GUN("Ruger Mini-14",	12,3200,c_brown,IRON,	WOOD,
	sk_rifle,	AT_223,	12, 26, 12,  3,  4,  5, -4,  8,  0, 10, 500, "\
A small, lightweight semi-auto carbine designed for military use. Its superb\n\
accuracy and low recoil makes it more suitable than full-auto rifles for some\n\
situations.",
0);

GUN("Savage 111F",	10,3280,c_brown,STEEL,	PLASTIC,
	sk_rifle,	AT_308, 12, 26, 13,  3,  6,  4,-11,  9,  0,  3, 100, "\
A very accurate rifle chambered for the powerful .308 round. Its very low\n\
ammo capacity is offset by its accuracy and near-complete lack of recoil.",
mfb(IF_RELOAD_ONE));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("H&K G3",		15,5050,c_blue,	IRON,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_rifle,	AT_308,	16, 40, 13,  2,  8, 10,  4,  8, 10, 20, 550, "\
An early battle rifle developed after the end of WWII. The G3 is designed to\n\
unload large amounts of deadly ammunition, but it is less suitable over long\n\
ranges.",
mfb(IF_MODE_BURST));

GUN("H&K G36",		17,5100,c_blue,	IRON,	PLASTIC,
	sk_rifle,	AT_223, 15, 32, 13,  2,  6,  8,  5,  8, 15, 30, 500, "\
Designed as a replacement for the early H&K G3 battle rifle, the G36 is more\n\
accurate, and uses the much-lighter .223 round, allowing for a higher ammo\n\
capacity.",
mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("AK-47",		16,4000,c_blue,	IRON,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_rifle,	AT_762,	16, 38, 14,  2,  0, 11,  4,  9,  8, 30, 475, "\
One of the most recognizable assault rifles ever made, the AK-47 is renowned\n\
for its durability even under the worst conditions.",
mfb(IF_MODE_BURST));

GUN("FN FAL",		16,4500,c_blue,	IRON,	WOOD,
	sk_rifle,	AT_308,	19, 36, 14,  2,  7, 13, -2,  8, 10, 20, 550, "\
A Belgian-designed battle rifle, the FN FAL is not very accurate for a rifle,\n\
but its high fire rate and powerful .308 ammunition have made it one of the\n\
most widely-used battle rifles in the world.",
mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("Bushmaster ACR",	 4,4200,c_blue,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_rifle,	AT_223,	15, 27,	18,  2,  2, 10, -2,  8,  3, 30, 475, "\
This carbine was developed for military use in the early 21st century. It is\n\
damaging and accurate, though its rate of fire is a bit slower than competing\n\
.223 carbines.",
mfb(IF_MODE_BURST));

GUN("AR-15",		 9,4000,c_blue,	STEEL,	PLASTIC,
	sk_rifle,	AT_223,	19, 28, 12,  2,  0,  6,  0,  7, 10, 30, 500, "\
A widely used assault rifle and the father of popular rifles such as the M16.\n\
It is light and accurate, but not very durable.",
mfb(IF_MODE_BURST));

GUN("M4A1",		 7,4400,c_blue,	STEEL,	PLASTIC,
	sk_rifle,	AT_223, 14, 24, 13,  2,  4,  7,  2,  6, 10, 30, 475, "\
A popular carbine, long used by the US military. Though accurate, small, and\n\
lightweight, it is infamous for its fragility, particularly in less-than-\n\
ideal terrain.",
mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	MAT1	MAT2
GUN("FN SCAR-L",	 6,4800,c_blue,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_rifle,	AT_223,	15, 29, 18,  2,  1,  6, -4,  8, 10, 30, 500, "\
A modular assault rifle designed for use by US Special Ops units. The 'L' in\n\
its name stands for light, as it uses the lightweight .223 round. It is very\n\
accurate and low on recoil.",
mfb(IF_MODE_BURST));

GUN("FN SCAR-H",	 5,4950,c_blue,	STEEL,	PLASTIC,
	sk_rifle,	AT_308,	16, 32, 20,  2,  1,  8, -4,  8,  8, 20, 550, "\
A modular assault rifle designed for use by US Special Ops units. The 'H' in\n\
its name stands for heavy, as it uses the powerful .308 round. It is fairly\n\
accurate and low on recoil.",
mfb(IF_MODE_BURST));

GUN("Steyr AUG",	 6,4900,c_blue, STEEL,	PLASTIC,
	sk_rifle,	AT_223, 14, 32, 17,  1, -3,  7, -8,  8,  6, 30, 550, "\
The Steyr AUG is an Austrian assault rifle that uses a bullpup design. It is\n\
used in the armed forces and police forces of many nations, and enjoys\n\
low recoil and high accuracy.",
mfb(IF_MODE_BURST));

GUN("M249",		 1,7500,c_ltred,STEEL,	PLASTIC,
//  SKILL       AMMO    VOL WGT MDG HIT DMG ACC REC DUR BST CLIP RELOAD
	sk_rifle,	AT_223,	32, 68, 27, -4, -6, 20,  6,  7, 30,200, 750, "\
The M249 is a mountable machine gun used by the US military and SWAT teams.\n\
Quite innaccurate and difficult to control, the M249 is designed to fire many\n\
rounds very quickly.",
mfb(IF_MODE_BURST));

//  NAME		RAR PRC COLOR	 MAT1	MAT2
GUN("V29 laser pistol",	 1,7200,c_magenta,STEEL,PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP
	sk_pistol,	AT_FUSION,4, 6,  5,  1, -2, 20,  0,  8,  0, 20, 200, "\
The V29 laser pistol was designed in the mid-21st century, and was one of the\n\
first firearms to use fusion as its ammunition. It is larger than most\n\
traditional handguns, but displays no recoil whatsoever.",
0);

GUN("FTK-93 fusion gun", 1,9800,c_magenta,STEEL, PLASTIC,
	sk_rifle,	AT_FUSION,18,20, 10, 1, 40, 10,  0,  9,  0,  2, 600, "\
A very powerful fusion rifle developed shortly before the influx of monsters.\n\
It can only hold two rounds at a time, but a special superheating unit causes\n\
its bolts to be extremely deadly.",
0);

GUN("NX-17 charge rifle",1,12000,c_magenta,STEEL, PLASTIC,
	sk_rifle,	AT_NULL, 13,16,  8, -1,  0,   6,  0,  8,  0, 10,   0, "\
A multi-purpose rifle, designed for use in conjunction with a unified power\n\
supply, or UPS. It does not reload normally; instead, press fire once to\n\
start charging it from your UPS, then again to unload the charge.",
mfb(IF_CHARGE));

//  NAME		RAR PRC COLOR	 MAT1	MAT2
GUN("simple flamethr.",1,1600,c_pink,	STEEL,	PLASTIC,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP RELOAD
	sk_shotgun,	AT_GAS,  16,  8,  8, -1, -5,  6,  0,  6,  0,800, 800, "\
A simple, home-made flamethrower. While its capacity is not superb, it is\n\
more than capable of igniting terrain and monsters alike.",
mfb(IF_FIRE_100));

GUN("flamethrower",	 1,3800,c_pink,	STEEL,	MNULL,
	sk_shotgun,	AT_GAS,  20, 14, 10, -2,  0,  4,  0,  8,  4,1600, 900, "\
A large flamethrower with substantial gas reserves. Very menacing and\n\
deadly.",
mfb(IF_FIRE_100));

GUN("tube 40mm launcher",0, 400,c_ltred,STEEL,	WOOD,
	sk_launcher,	AT_40MM,12, 20, 13, -1,  0, 16,  0,  6, 0,  1, 350, "\
A simple, home-made grenade launcher. Basically a tube with a pin firing\n\
mechanism to activate the grenade.",
0);

//  NAME		RAR PRC COLOR	 MAT1	MAT2
GUN("M79 launcher",	 5,4000,c_ltred,STEEL,	WOOD,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP RELOAD
	sk_launcher,	AT_40MM,14, 24, 16, -1,  3,  4, -5,  8, 0,  1, 250, "\
A widely-used grenade launcher which first saw use by American forces in the\n\
Vietnam war. Though mostly replaced by more modern launchers, the M79 still\n\
sees use with many units worldwide.",
0);

GUN("M320 launcher",	10,8500,c_ltred,STEEL,	MNULL,
	sk_launcher,	AT_40MM,  5, 13,  6,  0,  0, 12,  5,  9,  0,  1, 150, "\
Developed by Heckler & Koch, the M320 grenade launcher has the functionality\n\
of larger launchers in a very small package. However, its smaller size\n\
contributes to a lack of accuracy.",
0);

GUN("Milkor MGL",	 6,10400,c_ltred,STEEL,	MNULL,
	sk_launcher,	AT_40MM, 24, 45, 13, -1,  0,  5, -2,  8,  2,  6, 300, "\
The Milkor Multi-Grenade Launcher is designed to compensate for the drawback\n\
of single-shot grenade launchers by allowing sustained heavy firepower.\n\
However, it is still slow to reload and must be used with careful planning.",
mfb(IF_RELOAD_ONE)|mfb(IF_MODE_BURST));

//  NAME		    RAR PRC COLOR		MAT1	MAT2
GUN("coilgun",		1, 200,c_ltblue,	IRON,	MNULL,
//	SKILL		AMMO	VOL WGT MDG HIT DMG ACC REC DUR BST CLIP RELOAD
	sk_pistol,	AT_NAIL, 6, 30, 10, -1,  8, 10,  0,  5,  0, 100, 600, "\
A homemade gun, using electromagnets to accelerate a ferromagnetic\n\
projectile to high velocity. Powered by UPS.",
mfb(IF_USE_UPS));

GUN("H&K G80 Railgun",		2,9200,c_ltblue,STEEL,	MNULL,
	sk_rifle,	AT_12MM,12, 36, 12,  1,  5,  15, 0,  8,  0, 20, 550, "\
Developed by Heckler & Koch in 2033, the railgun magnetically propels\n\
a ferromagnetic projectile using an alternating current. This makes it\n\
silent while still deadly. Powered by UPS.",
mfb(IF_USE_UPS));

GUN("Boeing XM-P Plasma Rifle",		1,13000,c_ltblue,STEEL,	MNULL,
	sk_rifle,	AT_PLASMA,15, 40, 12, 1,  5,  5, 0,  8,  5, 25, 700, "\
Boeing developed the focused plasma weaponry together with DARPA. It heats\n\
hydrogen to create plasma and envelops it with polymers to reduce blooming.\n\
While powerful, it suffers from short range. Powered by UPS.",
mfb(IF_USE_UPS)|mfb(IF_MODE_BURST));

// GUN MODS
// Accuracy is inverted from guns; high values are a bonus, low values a penalty
// The clip modification is a percentage of the original clip size.
// The final variable is a bitfield of acceptible ammo types.  Using 0 means
//   that any ammo type is acceptable (so long as the mod works on the class of
//   gun)
#define GUNMOD(name, rare, value, color, mat1, mat2, volume, weight, meleedam,\
               meleecut, meleehit, acc, damage, loudness, clip, recoil, burst,\
               newtype, pistol, shotgun, smg, rifle, a_a_t, des, flags)\
  index++; itypes.push_back(new it_gunmod(index, rare, value, name, des, ':',\
                            color, mat1, mat2, volume, weight, meleedam,\
                            meleecut, meleehit, flags, acc, damage, loudness,\
                            clip, recoil, burst, newtype, a_a_t, pistol,\
                            shotgun, smg, rifle))


//	NAME      	 RAR  PRC  COLOR     MAT1   MAT2     VOL WGT DAM CUT HIT
GUNMOD("silencer",	 15,  480, c_dkgray, STEEL, PLASTIC,  2,  1,  3,  0,  2,
//	ACC DAM NOI CLP REC BST NEWTYPE,	PISTOL	SHOT	SMG	RIFLE
	-1, -4,-25,  0,  0,  0,	AT_NULL,	true,	false,	true,	true,
	0, "\
Using a silencer is almost an imperative in zombie-infested regions. Gunfire\n\
is very noisy, and will attract predators. Its only drawback is a reduced\n\
muzzle velocity, resulting in less accuracy and damage.",
0);

GUNMOD("enhanced grip",  12, 280, c_brown,  STEEL, PLASTIC,   1,  1,  0,  0, -1,
	 2,  0,  0,  0, -2,  0, AT_NULL,	false,	true,	true,	true,
	0, "\
A grip placed forward on the barrel allows for greater control and accuracy.\n\
Aside from increased weight, there are no drawbacks.",
0);

GUNMOD("barrel extension",10,400,  c_ltgray, STEEL, MNULL,    4,  1,  5,  0,  2,
	 6,  1,  0,  0,  5,  0,	AT_NULL,	false,	false,	true,	true,
	0, "\
A longer barrel increases the muzzle velocity of a firearm, contributing to\n\
both accuracy and damage.  However, the longer barrel tends to vibrate after\n\
firing, greatly increasing recoil.",
0);

//	NAME      	 RAR  PRC  COLOR     MAT1   MAT2     VOL WGT DAM CUT HIT
GUNMOD("shortened barrel", 6, 320, c_ltgray, STEEL, MNULL,    1,  1, -2,  0, -1,
//	ACC DAM NOI CLP REC BST NEWTYPE,	PISTOL	SHOT	SMG	RIFLE
	-5,  0,  2,  0, -6,  0, AT_NULL,	false,	false,	true,	true,
	0, "\
A shortened barrel results in markedly reduced accuracy, and a minor increase\n\
in noise, but also reduces recoil greatly as a result of the improved\n\
managability of the firearm.",
0);

GUNMOD("rifled barrel",    5, 220, c_ltgray, STEEL, MNULL,    2,  1,  3,  0,  1,
	10,-20,  0,  0,  0, -1, AT_NULL,	false,	true,	false,	false,
	0, "\
Rifling a shotgun barrel is mainly done in order to improve its accuracy when\n\
firing slugs. The rifling makes the gun less suitable for shot, however.",
0);

GUNMOD("extended magazine",	  8,  560, c_ltgray, STEEL, PLASTIC,  1,  1, -2,  0, -1,
	-1,  0,  0, 50,  0,  0, AT_NULL,	true,	true,	true,	true,
	0, "\
Increases the ammunition capacity of your firearm by 50%, but the added bulk\n\
reduces accuracy slightly.",
0);

//	NAME      	 RAR  PRC  COLOR     MAT1   MAT2     VOL WGT DAM CUT HIT
GUNMOD("double magazine",	   4, 720, c_ltgray, STEEL, PLASTIC,  2,  2,  0,  0,  0,
//	ACC DAM NOI CLP REC BST NEWTYPE,	PISTOL	SHOT	SMG	RIFLE
	-2,  0,  0,100,  2,  0, AT_NULL,	false,	true,	true,	true,
	0, "\
Completely doubles the ammunition capacity of your firearm, but the added\n\
bulk reduces accuracy and increases recoil.",
0);

GUNMOD("spare magazine",	   15, 200, c_ltgray, STEEL, PLASTIC,  1,  1,  0,  0,  0,
	0,  0,  0,  0,  0,  0, AT_NULL,	true,	true,	true,	true,
	0, "\
A spare magazine you can keep on hand to make reloads faster, but must itself\n\
 be reloaded before it can be used again.",
0);

GUNMOD("gyroscopic stablizer",4,680,c_blue,  STEEL, PLASTIC,  3,  2,  0,  0, -3,
	 2, -2,  0,-10, -8,  0, AT_NULL,	false,	false,	true,	true,
	0, "\
An advanced unit which straps onto the side of your firearm and reduces\n\
vibration, greatly reducing recoil and increasing accuracy.  However, it also\n\
takes up space in the magazine slot, reducing ammo capacity.",
0);

GUNMOD("rapid blowback",   3, 700, c_red,    STEEL, PLASTIC,  0,  1,  0,  0,  0,
	-3,  0,  4,  0,  0,  6, AT_NULL,	false,	false,	true,	true,
	0, "\
An improved blowback mechanism makes your firearm's automatic fire faster, at\n\
the cost of reduced accuracy and increased noise.",
0);

//	NAME      	 RAR  PRC  COLOR     MAT1   MAT2     VOL WGT DAM CUT HIT
GUNMOD("auto-fire mechanism",2,650,c_red,    STEEL, PLASTIC,  1,  2,  2,  0, -1,
//	ACC DAM NOI CLP REC BST NEWTYPE,	PISTOL	SHOT	SMG	RIFLE
	-2,  0,  2,  0,  2,  3, AT_NULL,	true,	false,	false,	false,
	(mfb(AT_22)|mfb(AT_9MM)|mfb(AT_38)|mfb(AT_40)|mfb(AT_57)|mfb(AT_46)), "\
A simple mechanism which converts a pistol to a fully-automatic weapon, with\n\
a burst size of three rounds. However, it reduces accuracy, while increasing\n\
noise and recoil.",
0);

//	NAME      	 RAR  PRC  COLOR     MAT1   MAT2     VOL WGT DAM CUT HIT
GUNMOD(".45 caliber retool",3,480, c_green,  STEEL, MNULL,    2,  2,  3,  0, -1,
//	ACC DAM NOI CLP REC BST NEWTYPE,	PISTOL	SHOT	SMG	RIFLE
	-2,  0,  0,  0,  2,  0, AT_45,		true,	false,	true,	false,
	(mfb(AT_9MM)|mfb(AT_38)|mfb(AT_40)|mfb(AT_44)), "\
Replacing several key parts of a 9mm, .38, .40 or .44 firearm converts it to\n\
a .45 firearm.  The conversion results in reduced accuracy and increased\n\
recoil.",
0);

GUNMOD("9mm caliber retool",3,420, c_green,  STEEL, MNULL,    1,  1,  0,  0, -1,
	-1,  0,  0,  0,  0,  0, AT_9MM,		true,	false,	true,	false,
	(mfb(AT_38)|mfb(AT_40)|mfb(AT_44)|mfb(AT_45)), "\
Replacing several key parts of a .38, .40, .44 or .45 firearm converts it to\n\
a 9mm firearm.  The conversion results in a slight reduction in accuracy.",
0);

GUNMOD(".22 caliber retool",2,320, c_green,  STEEL, MNULL,    1,  1, -2,  0, -1,
	-1,  0,  0,  0,  0,  0, AT_22,		true,	false,	true,	true,
	(mfb(AT_9MM)|mfb(AT_38)|mfb(AT_40)|mfb(AT_57)|mfb(AT_46)|mfb(AT_762)|
	 mfb(AT_223)), "\
Replacing several key parts of a 9mm, .38, .40, 5.7mm, 4.6mm, 7.62mm or .223\n\
firearm converts it to a .22 firearm. The conversion results in a slight\n\
reduction in accuracy.",
0);

//	NAME      	 RAR  PRC  COLOR     MAT1   MAT2     VOL WGT DAM CUT HIT
GUNMOD("5.7mm caliber retool",1,460,c_green, STEEL, MNULL,    1,  1, -3,  0, -1,
//	ACC DAM NOI CLP REC BST NEWTYPE,	PISTOL	SHOT	SMG	RIFLE
	-1,  0,  0,  0,  0,  0, AT_57,		true,	false,	true,	false,
	(mfb(AT_22)|mfb(AT_9MM)|mfb(AT_38)), "\
FN Hestal sells a conversion kit, used to convert .22, 9mm, or .38 firearms\n\
to their proprietary 5.7x28mm, a round designed for accuracy and armor\n\
penetration.",
0);

GUNMOD("4.6mm caliber retool",1,460,c_green, STEEL, MNULL,    1,  1, -3,  0, -1,
	-1,  0,  0,  0,  0,  0, AT_46,		true,	false,	true,	false,
	(mfb(AT_22)|mfb(AT_9MM)|mfb(AT_38)), "\
Heckler and Koch sells a conversion kit, used to convert .22, 9mm, or .38\n\
firearms to their proprietary 4.6x30mm, a round designed for accuracy and\n\
armor penetration.",
0);

//	NAME      	RAR  PRC  COLOR     MAT1   MAT2      VOL WGT DAM CUT HIT
GUNMOD(".308 caliber retool",2,520,c_green, STEEL, MNULL,     2,  1,  4,  0,  1,
//	ACC DAM NOI CLP REC BST NEWTYPE		PISTOL	SHOT	SMG	RIFLE
	-2,  0,  0,-20,  0,  0, AT_308,		false,	true,	false,	true,
	(mfb(AT_SHOT)|mfb(AT_762)|mfb(AT_223)|mfb(AT_3006)), "\
This kit is used to convert a shotgun or 7.62mm, .223 or .30-06 rifle to the\n\
popular and powerful .308 caliber. The conversion results in reduced ammo\n\
capacity and a slight reduction in accuracy.",
0);

GUNMOD(".223 caliber retool",2,500,c_green, STEEL, MNULL,     2,  1,  4,  0,  1,
	-2,  0,  0,-10,  0,  0, AT_223,		false,	true,	false,	true,
	(mfb(AT_SHOT)|mfb(AT_762)|mfb(AT_3006)|mfb(AT_308)), "\
This kit is used to convert a shotgun or 7.62mm, .30-06, or .308 rifle to the\n\
popular, accurate, and damaging .223 caliber. The conversion results in\n\
slight reductions in both accuracy and ammo capacity.",
0);

//	NAME      	RAR  PRC  COLOR     MAT1   MAT2      VOL WGT DAM CUT HIT
GUNMOD("battle rifle conversion",1,680,c_magenta,STEEL,MNULL, 4,  3,  6,  0, -1,
//	ACC DAM NOI CLP REC BST NEWTYPE		PISTOL	SHOT	SMG	RIFLE
	-6,  6,  4, 20,  4,  4, AT_NULL,	false,	false,	false,	true,
	0, "\
This is a complete conversion kit, designed to turn a rifle into a powerful\n\
battle rifle. It reduces accuracy, and increases noise and recoil, but also\n\
increases damage, ammo capacity, and fire rate.",
0);

GUNMOD("sniper conversion",1,660, c_green,  STEEL, MNULL,     1,  2,  0,  0, -1,
	10,  8,  3,-15,  0,-99, AT_NULL,	false,	false,	false,	true,
	0, "\
This is a complete conversion kit, designed to turn a rifle into a deadly\n\
sniper rifle. It decreases ammo capacity, and removes any automatic fire\n\
capabilities, but also increases accuracy and damage.",
0);

GUNMOD("M203",		2,650,	c_ltred, STEEL,	MNULL,        2,  1,  2,  0, -1,
	-2,  0,  0,  1,  0, 0, AT_40MM,		false,	false,	false,	true,
	0, "\
The M203 was originally designed for use with M16 variants but today can be\n\
attached to almost any rifle. It allows a single 40mm grenade to be loaded\n\
and fired.",
mfb(IF_MODE_AUX));

//	NAME      	RAR  PRC  COLOR     MAT1   MAT2      VOL WGT DAM CUT HIT
GUNMOD("bayonet",	 6, 400, c_ltcyan, STEEL, MNULL,       2,  2,  0, 16, -3,
//	ACC DAM NOI CLP REC BST NEWTYPE		PISTOL	SHOT	SMG	RIFLE
	  0,  0,  0,  0,  3,  0, AT_NULL,	false,	true,	true,	true,
	0, "\
A bayonet is a stabbing weapon which can be attached to the front of a\n\
shotgun, sub-machinegun or rifle, allowing a melee attack to deal\n\
piercing damage. The added length increases recoil slightly.",
mfb(IF_STAB));

GUNMOD("underslung shotgun", 2,650,  c_ltred, STEEL, MNULL,        2,  1,  2,  0, -1,
        -2,  0,  0,  2,  0, 0, AT_SHOT,         false,  false,  false,  true,
        0, "\
A miniaturized shotgun with 2 barrels, which can be mounted under the\n\
barrel of many rifles. It allows two shotgun shells to be loaded and fired.",
mfb(IF_MODE_AUX));

GUNMOD("rail-mounted crossbow", 2, 500,  c_ltred, STEEL, WOOD,      2,  1,  2,  0, -1,
        0,  0,  0,  1,  0, 0, AT_BOLT,         false,  true,  false,  true,
        0, "\
A kit to attach a pair of crossbow arms and a firing rail to\n\
the barrel of a long firearm. It allows crossbow bolts to be fired.",
mfb(IF_MODE_AUX)|mfb(IF_STR_RELOAD));

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
You can read it for the articles. Or not.");

BOOK("US Weekly",		40,  40,c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	sk_speech,	 1,  0,  1,  3,  8, "\
Weekly news about a bunch of famous people who're all (un)dead now.");

BOOK("TIME magazine",		35,  40,c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	sk_null,	 0,  0,  2,  7,  10, "\
Current events concerning a bunch of people who're all (un)dead now.");

BOOK("Top Gear magazine",	40,  45,c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	sk_driving,	 1,  0,  1,  2,  8, "\
Lots of articles about cars and driving techniques.");

BOOK("Bon Appetit",		30,  45,c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	sk_cooking,	 1,  0,  1,  5,  8, "\
Exciting recipes and restaurant reviews. Full of handy tips about cooking.");

BOOK("Birdhouse Monthly",       30,  45,c_pink,		PAPER,	MNULL,
    1,  1, -3,  1,	sk_carpentry,	 1,  0,  1,  5,  8, "\
A riveting periodical all about birdhouses and their construction.");

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
bound paper. Still, there are lots of useful tips for unarmed combat.");

BOOK("Spetsnaz Knife Techniques",12,200,c_green,	PAPER,	MNULL,
    1,  1, -5,  0,	sk_cutting,	 4,  1,  0,  4, 18, "\
A classic Soviet text on the art of attacking with a blade.");

//	NAME			RAR PRC	COLOR		MAT1	MAT2
BOOK("Under the Hood",		35, 190,c_green,	PAPER,	MNULL,
// VOL WGT DAM HIT	TYPE		LEV REQ FUN INT TIME
    3,  1, -3,  0,	sk_mechanics,	 3,  0,  0,  5, 18, "\
An advanced mechanics manual, covering all sorts of topics.");

BOOK("Pitching a Tent",20,200,c_green,  PAPER,  MNULL,
// VOL WGT DAM HIT      TYPE            LEV REQ FUN INT TIME
    3,  1,  -3, 0,      sk_survival,    3,   0,  0,  4,  18,"\
A guide detailing the basics of woodsmanship and outdoor survival.");

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

//	NAME			RAR PRC	COLOR		MAT1	MAT2
BOOK("Building for Beginners",  10, 220,c_green,	PAPER,	MNULL,
// VOL WGT DAM HIT	TYPE		LEV REQ FUN INT TIME
    2,  1, -3,  0,	sk_carpentry,	 3,  0,  0,  5, 16, "\
A large, paperback book detailing several beginner's projects in\n\
construction.");

BOOK("Computer Science 301",	 8, 500,c_blue,		PAPER,	MNULL,
    7,  4,  5,  1,	sk_computer,	 5,  2, -2, 11, 35, "\
A college textbook on computer science.");

BOOK("Advanced Electronics",	 6, 520,c_blue,		PAPER,	MNULL,
    7,  5,  5,  1,	sk_electronics,	 5,  2, -1, 11, 35, "\
A college textbook on circuit design.");

BOOK("Advanced Economics",	12, 480,c_blue,		PAPER,	MNULL,
    7,  4,  5,  1,	sk_barter,	 5,  3, -1,  9, 30, "\
A college textbook on economics.");

BOOK("Mechanical Mastery",12,495,c_blue,PAPER,MNULL,
    6,  3,  4,  1,      sk_mechanics,   6,   3, -1,  6,  30,"\
An advanced guide on mechanics and welding, covering topics like\n\
\"Grinding off rust\" and \"Making cursive E\'s\".");

//	NAME			RAR PRC	COLOR		MAT1	MAT2
BOOK("Chemistry Textbook",	11, 495,c_blue,		PAPER,	MNULL,
// VOL WGT DAM HIT	TYPE		LEV REQ FUN INT TIME
    8,  6,  5,  1,	sk_cooking,	 6,  3, -1, 12, 35, "\
A college textbook on chemistry.");

BOOK("Engineering 301",		 6, 550,c_blue,		PAPER,	MNULL,
    6,  3,  4,  1,	sk_carpentry,	 6,  3, -1,  8, 30, "\
A textbook on civil engineering and construction.");

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
A small, open plastic bag. Essentially trash.");

CONT("plastic bottle",	70,  8,	c_ltcyan,	PLASTIC,MNULL,
    2,  0, -8,  1,	 2,	mfb(con_rigid)|mfb(con_wtight)|mfb(con_seals),"\
A resealable plastic bottle, holds 500mls of liquid.");

CONT("glass bottle",	70, 12,	c_cyan,		GLASS,	MNULL,
    2,  1,  8,  1,	 3,	mfb(con_rigid)|mfb(con_wtight)|mfb(con_seals),"\
A resealable glass bottle, holds 750mls of liquid.");

CONT("aluminum can",	70,  1,	c_ltblue,	STEEL,	MNULL,
    1,  0,  0,  0,	 1,	mfb(con_rigid)|mfb(con_wtight), "\
An aluminum can, like what soda comes in.");

CONT("tin can",		65,  2,	c_blue,		IRON,	MNULL,
    1,  0, -1,  1,	 1,	mfb(con_rigid)|mfb(con_wtight), "\
A tin can, like what beans come in.");

CONT("sm. cardboard box",50, 0,	c_brown,	PAPER,	MNULL,
    4,  0, -5,  1,	 4,	mfb(con_rigid), "\
A small cardboard box. No bigger than a foot in any dimension.");

CONT("plastic canteen",	20,  1000,	c_green,	PLASTIC,MNULL,
    6,  2, -8,  1,	 6,	mfb(con_rigid)|mfb(con_wtight)|mfb(con_seals),"\
A large military-style water canteen, with a 1.5 litre capacity");

CONT("plastic jerrycan",	10,  2500,	c_green,	PLASTIC,MNULL,
    40,  4, -2,  -2,	 40,	mfb(con_rigid)|mfb(con_wtight)|mfb(con_seals),"\
A bulky plastic jerrycan, meant to carry fuel, but can carry other liquids\n\
in a pinch. It has a capacity of 10 litres.");

CONT("plastic carboy",	10,  2500,	c_ltcyan,	PLASTIC,MNULL,
    10,  2, -8,  1,	 10,	mfb(con_rigid)|mfb(con_wtight)|mfb(con_seals),"\
A 2.5 litre plastic carboy for household cleaning chemicals.");

CONT("glass flask",	10,  2500,	c_ltcyan,	GLASS,MNULL,
    1,  0, 8,  1,	 1,	mfb(con_rigid)|mfb(con_wtight)|mfb(con_seals),"\
A 250 ml laboratory conical flask, with a rubber bung.");

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
    2,  0, -3,  0, -1,  200, 50, 1,  0, AT_THREAD, itm_null, &iuse::sew, 0, "\
Use a sewing kit on an article of clothing to attempt to repair or reinforce\n\
that clothing. This uses your tailoring skill.");

TOOL("scissors",	50,  45,',', c_ltred,	IRON,	PLASTIC,
    1,  1,  0,  8, -1,   0,  0, 0,  0, AT_NULL, itm_null, &iuse::scissors,
mfb(IF_SPEAR), "\
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
    3,  2,  1,  0,  2, 100,100, 0, 15, AT_BATT,itm_flashlight,&iuse::light_on,
mfb(IF_LIGHT_8),
"This flashlight is turned on, and continually draining its batteries. It\n\
provides light during the night or while underground. Use it to turn it off.");

TOOL("hotplate",	10, 250,';', c_green,	IRON,	PLASTIC,
    5,  6,  8,  0, -1, 200, 100,  0,  0, AT_BATT, itm_null, &iuse::none,0,"\
A small heating element. Indispensable for cooking and chemistry.");

//	NAME		RAR PRC	SYM  COLOR	MAT1	MAT
TOOL("soldering iron",	70, 200,',', c_ltblue,	IRON,	MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    3,  1,  2,  6,  0, 50, 20,  0,  0, AT_BATT, itm_null, &iuse::none,
mfb(IF_SPEAR), "\
A piece of metal that can get very hot. Necessary for electronics crafting.");

TOOL("water purifier",   5,1200,';', c_ltblue,	PLASTIC, IRON,
   12, 20,  2,  0, -3, 100,100, 1,  0, AT_BATT,itm_null,&iuse::water_purifier,0,
"Using this item on a container full of water will purify the water. Water\n\
taken from uncertain sources like a river may be dirty.");

TOOL("two-way radio",	10, 800,';', c_yellow,	PLASTIC, IRON,
    2,  3, 10,  0,  0, 100,100, 1,  0, AT_BATT, itm_null,&iuse::two_way_radio,0,
"Using this allows you to send out a signal; either a general SOS, or if you\n\
are in contact with a faction, to send a direct call to them.");

TOOL("radio (off)",	20, 420,';', c_yellow,	PLASTIC, IRON,
    4,  2,  4,  0, -1, 100,100, 0,  0, AT_BATT,	itm_null, &iuse::radio_off, 0,"\
Using this radio turns it on. It will pick up any nearby signals being\n\
broadcast and play them audibly.");

//	NAME		RAR PRC	SYM  COLOR	MAT1	MAT
TOOL("radio (on)",	 0, 420,';', c_yellow,	PLASTIC, IRON,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    4,  2,  4,  0, -1, 100,100, 0,  8, AT_BATT, itm_radio,&iuse::radio_on, 0,"\
This radio is turned on, and continually draining its batteries. It is\n\
playing the broadcast being sent from any nearby radio towers.");

TOOL("crowbar",		18, 130,';', c_ltblue,	IRON,	MNULL,
    4,  9, 16,  3,  2,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::crowbar, 0,"\
A prying tool. Use it to open locked doors without destroying them, or to\n\
lift manhole covers.");

TOOL("hoe",		30,  90,'/', c_brown,	IRON,	WOOD,
   14, 14, 12, 10,  3,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::makemound,
	mfb(IF_STAB), "\
A farming implement. Use it to turn tillable land into a slow-to-cross pile\n\
of dirt.");

TOOL("shovel",		40, 100,'/', c_brown,	IRON,	WOOD,
   16, 18, 15,  5,  3,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::dig, 0, "\
A digging tool. Use it to dig pits adjacent to your location.");

TOOL("chainsaw (off)",	 7, 350,'/', c_red,	IRON,	PLASTIC,
   12, 40, 10,  0, -4, 400, 0,  0,  0, AT_GAS,	itm_null, &iuse::chainsaw_off,0,
"Using this item will, if loaded with gas, cause it to turn on, making a very\n\
powerful, but slow, unwieldy, and noisy, melee weapon.");
TECH( mfb(TEC_SWEEP) );

//	NAME		RAR VAL	SYM  COLOR	MAT1	MAT
TOOL("chainsaw (on)",	 0, 350,'/', c_red,	IRON,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
   12, 40,  4, 70, -5, 400, 0,  0,  1, AT_GAS,	itm_chainsaw_off,
	&iuse::chainsaw_on, mfb(IF_MESSY), "\
This chainsaw is on, and is continuously draining gasoline. Use it to turn\n\
it off.");
TECH( mfb(TEC_SWEEP) );

TOOL("jackhammer",	 2, 890,';', c_magenta,	IRON,	MNULL,
   13, 54, 20,  6, -4, 120,  0,10,  0, AT_GAS,	itm_null, &iuse::jackhammer,0,"\
This jackhammer runs on gasoline. Use it (if loaded) to blast a hole in\n\
adjacent solid terrain.");

TOOL("jacqueshammer",     2, 890,  ';', c_magenta, IRON,   MNULL,
   13, 54, 20,  6, -4, 120,  0,10,  0, AT_GAS,  itm_null, &iuse::jacqueshammer,0,"\
Ce marteau-piqueur fonctionne a l'essence.\n\
Utilisez-le (si charge) pour faire sauter un troudans\n\
adjacente terrain solide.");

TOOL("bubblewrap",	50,  40,';', c_ltcyan,	PLASTIC,MNULL,
    2,  0, -8,  0,  0,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::set_trap,0,"\
A sheet of plastic covered with air-filled bubbles. Use it to set it on the\n\
ground, creating a trap that will warn you with noise when something steps on\n\
it.");

TOOL("bear trap",	 5, 120,';', c_cyan,	IRON,	MNULL,
    4, 12,  9,  1, -2,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::set_trap,0,"\
A spring-loaded pair of steel jaws. Use it to set it on the ground, creating\n\
a trap that will ensnare and damage anything that steps on it. If you are\n\
carrying a shovel, you will have the option of burying it.");

//	NAME		RAR VAL	SYM  COLOR	MAT1	MAT
TOOL("nailboard trap",	 0, 30, ';', c_brown,	WOOD,	MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
   18, 18, 12,  6, -3,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::set_trap,0,"\
Several pieces of wood, nailed together, with nails sticking straight up. If\n\
an unsuspecting victim steps on it, they'll get nails through the foot.");

TOOL("tripwire trap",	 0, 35, ';', c_ltgray,	PAPER,	MNULL,
    1,  0,-10,  0, -1,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::set_trap,0,"\
A tripwire trap must be placed across a doorway or other thin passage. Its\n\
purpose is to trip up bypassers, causing them to stumble and possibly hurt\n\
themselves minorly.");

TOOL("crossbow trap",	 0,600, ';', c_green,	IRON,	WOOD,
    7, 10,  4,  0, -2,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::set_trap,0,"\
A simple tripwire is attached to the trigger of a loaded crossbow. When\n\
pulled, the crossbow fires. Only a single round can be used, after which the\n\
trap is disabled.");

TOOL("shotgun trap",	 0,450, ';', c_red,	IRON,	WOOD,
    7, 11, 12,  0, -2,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::set_trap,0,"\
A simple tripwire is attached to the trigger of a loaded sawn-off shotgun.\n\
When pulled, the shotgun fires. Two rounds are used; the first time the\n\
trigger is pulled, one or two may be used.");

//	NAME		RAR VAL	SYM  COLOR	MAT1	MAT
TOOL("blade trap",	 0,500, ';', c_ltgray,	IRON,	MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
   13, 21,  4, 16, -4,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::set_trap,0,"\
A machete is attached laterally to a motor, with a tripwire controlling its\n\
throttle. When the tripwire is pulled, the blade is swung around with great\n\
force. The trap forms a 3x3 area of effect.");

TOOL("land mine",	 3,2400,';', c_red,	IRON,	MNULL,
    3,  6, 10,  0, -1,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::set_trap,0,"\
An explosive that is triggered when stepped upon. It must be partially\n\
buried to be effective, and so you will need a shovel to use it.");

TOOL("geiger ctr (off)", 8, 300,';', c_green,	PLASTIC,STEEL,
    2,  2,  2,  0,  0,100,100,  1,  0, AT_BATT,	itm_null, &iuse::geiger,0,"\
A tool for measuring radiation. Using it will prompt you to choose whether\n\
to scan yourself or the terrain, or to turn it on, which will provide\n\
continuous feedback on ambient radiation.");

TOOL("geiger ctr (on)",	0, 300, ';', c_green,	PLASTIC,STEEL,
    2,  2,  2,  0,  0,100,100,  0, 10, AT_BATT, itm_geiger_off,&iuse::geiger,0,
"A tool for measuring radiation. It is in continuous scan mode, and will\n\
produce quiet clicking sounds in the presence of ambient radiation. Using it\n\
allows you to turn it off, or scan yourself or the ground.");

//	NAME		RAR VAL	SYM  COLOR	MAT1	MAT
TOOL("teleporter",      10,6000,';', c_magenta,	PLASTIC,STEEL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    3, 12,  4,  0, -1, 20, 20,  1,  0, AT_PLUT,	itm_null, &iuse::teleport,0,"\
An arcane device, powered by plutonium fuel cells. Using it will cause you\n\
to teleport a short distance away.");

TOOL("goo canister",     8,3500,';', c_dkgray,  STEEL,	MNULL,
    6, 22,  7,  0,  1,  1,  1,  1,  0, AT_NULL,	itm_null, &iuse::can_goo,0,"\
\"Warning: contains highly toxic and corrosive materials. Contents may be\n\
 sentient. Open at your own risk.\"");

TOOL("pipe bomb",	 4, 150,'*', c_white,	IRON,	MNULL,
    2,  3, 11,  0,  1,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::pipebomb,0,"\
A section of a pipe filled with explosive materials. Use this item to light\n\
the fuse, which gives you 3 turns before it detonates. You will need a\n\
lighter. It is somewhat unreliable, and may fail to detonate.");

TOOL("active pipe bomb", 0,   0,'*', c_white,	IRON,	MNULL,
    2,  3, 11,  0,  1,  3,  3,  0,  1, AT_NULL,	itm_null, &iuse::pipebomb_act,0,
"This pipe bomb's fuse is lit, and it will explode any second now. Throw it\n\
immediately!");

//	NAME		RAR VAL	SYM  COLOR	MAT1	MAT
TOOL("grenade",		 3, 400,'*', c_green,	IRON,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    1,  1, 10,  0, -1,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::grenade,0,"\
Use this item to pull the pin, turning it into an active grenade. You will\n\
then have five turns before it explodes; throwing it would be a good idea.");

TOOL("active grenade",	 0,   0,'*', c_green,	IRON,	PLASTIC,
    1,  1, 10,  0, -1,  5,  5,  0,  1, AT_NULL, itm_null, &iuse::grenade_act,0,
"This grenade is active, and will explode any second now. Better throw it!");

TOOL("flashbang",	 3, 380,'*', c_white,	STEEL,	PLASTIC,
    1,  1,  8,  0, -1,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::flashbang,0,"\
Use this item to pull the pin, turning it into an active flashbang. You will\n\
then have five turns before it detonates with intense light and sound,\n\
blinding, deafening and disorienting anyone nearby.");

TOOL("active flashbang", 0,   0,'*', c_white,	STEEL,	PLASTIC,
    1,  1,  8,  0, -1,  5,  5,  0,  1, AT_NULL,	itm_null, &iuse::flashbang_act,
0,"This flashbang is active, and will soon detonate with intense light and\n\
sound, blinding, deafening and disorienting anyone nearby.");

//	NAME		RAR VAL	SYM  COLOR	MAT1	MAT
TOOL("EMP grenade",	 2, 600,'*', c_cyan,	STEEL,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    1,  1,  8,  0, -1,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::EMPbomb,0,"\
Use this item to pull the pin, turning it into an active EMP grenade. You\n\
will then have three turns before it detonates, creating an EMP field which\n\
damages robots and drains bionic energy.");

TOOL("active EMP grenade",0,  0,'*', c_cyan,	STEEL,	PLASTIC,
    1,  1,  8,  0, -1,  3,  3,  0,  1, AT_NULL,	itm_null, &iuse::EMPbomb_act,0,
"This EMP grenade is active, and will shortly detonate, creating a large EMP\n\
field which damages robots and drains bionic energy.");

//	NAME		RAR VAL	SYM  COLOR	MAT1	MAT
TOOL("scrambler grenade",	 2, 600,'*', c_cyan,	STEEL,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    1,  1,  8,  0, -1,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::scrambler,0,"\
This is a highly modified EMP grenade, designed to scramble robots' control\n\
chips, rather than destroy them. This converts the robot to your side for a \n\
short time, before the backup systems kick in.");

TOOL("active scrambler grenade",0,  0,'*', c_cyan,	STEEL,	PLASTIC,
    1,  1,  8,  0, -1,  3,  3,  0,  1, AT_NULL,	itm_null, &iuse::scrambler_act,0,
"This scrambler grenade is active, and will soon detonate, releasing a control\n\
wave that temporarily converts robots to your side.");

TOOL("teargas canister",3,  600,'*', c_yellow,	STEEL, MNULL,
    1,  1,  6,  0, -1,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::gasbomb,0,"\
Use this item to pull the pin. Five turns after you do that, it will begin\n\
to expell a highly toxic gas for several turns. This gas damages and slows\n\
those who enter it, as well as obscuring vision and scent.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("active teargas",	0,    0,'*', c_yellow,	STEEL, MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    1,  1,  6,  0, -1,  5,  5,  0,  1, AT_NULL, itm_null, &iuse::gasbomb_act,0,
"This canister of teargas has had its pin removed, indicating that it is (or\n\
will shortly be) expelling highly toxic gas.");

TOOL("smoke bomb",	5,  180,'*', c_dkgray,	STEEL,	MNULL,
    1,  1,  5,  0, -1,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::smokebomb,0,"\
Use this item to pull the pin. Five turns after you do that, it will begin\n\
to expell a thick black smoke. This smoke will slow those who enter it, as\n\
well as obscuring vision and scent.");

TOOL("active smoke bomb",0,  0, '*', c_dkgray,	STEEL,	MNULL,
    1,  1,  5,  0, -1,  0,  0,  0,  1, AT_NULL, itm_null,&iuse::smokebomb_act,0,
"This smoke bomb has had its pin removed, indicating that it is (or will\n\
shortly be) expelling thick smoke.");

TOOL("molotov cocktail",0,  200,'*', c_ltred,	GLASS,	COTTON,
    2,  2,  8,  0,  1,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::molotov,0,"\
A bottle of flammable liquid with a rag inserted. Use this item to light the\n\
rag; you will, of course, need a lighter in your inventory to do this. After\n\
lighting it, throw it to cause fires.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("molotov cocktail (lit)",0,0,'*', c_ltred,	GLASS,	COTTON,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    2,  2,  8,  0,  1,  1,  1,  0,  0, AT_NULL,itm_molotov,&iuse::molotov_lit,0,
"A bottle of flammable liquid with a flaming rag inserted. Throwing it will\n\
cause the bottle to break, spreading fire. The flame may go out shortly if\n\
you do not throw it. Dropping it while lit is not safe.");

TOOL("acid bomb", 	  0,500,'*', c_yellow,	GLASS,	MNULL,
     1,  1,  4,  0, -1,  0,  0,  0,  0,AT_NULL,	itm_null, &iuse::acidbomb,0,"\
A glass vial, split into two chambers. The divider is removable, which will\n\
cause the chemicals to mix. If this mixture is exposed to air (as happens\n\
if you throw the vial) they will spill out as a pool of potent acid.");

TOOL("acid bomb (active)",0,  0,'*', c_yellow, GLASS, MNULL,
    1,  1,  4,  0,  -1,  0,  0,  0,  0,AT_NULL,itm_null,&iuse::acidbomb_act,0,"\
A glass vial, with two chemicals mixing inside. If this mixture is exposed\n\
to air (as happens if you throw the vial), they will spill out as a pool of\n\
potent acid.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("dynamite",	5,  700,'*', c_red,	PLASTIC,MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    6, 10,  4,  0, -3,  0,  0,  0,  0, AT_NULL,	itm_null, &iuse::dynamite,0,"\
Several sticks of explosives with a fuse attached. Use this item to light\n\
the fuse; you will, of course, need a lighter in your inventory to do this.\n\
Shortly after lighting the fuse, this item will explode, so get away!");

TOOL("dynamite (lit)",	5,    0,'*', c_red,	PLASTIC,MNULL,
    6, 10,  4,  0, -3,  0,  0,  0,  1, AT_NULL,	itm_null, &iuse::dynamite_act,0,
"The fuse on this dynamite is lit and hissing.  It'll explode any moment now.");

TOOL("mininuke",	1, 1800,'*', c_ltgreen,	STEEL,	PLASTIC,
    3,  4,  8,  0, -2,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::mininuke,0,"\
An extremely powerful weapon--essentially a hand-held nuclear bomb. Use it\n\
to activate the timer. Ten turns later it will explode, leaving behind a\n\
radioactive crater. The explosion is large enough to take out a house.");

TOOL("mininuke (active)",0,   0,'*', c_ltgreen,	STEEL,	PLASTIC,
    3,  4,  8,  0, -2,  0,  0,  0,  1, AT_NULL, itm_null, &iuse::mininuke_act,0,
"This miniature nuclear bomb has a light blinking on the side, showing that\n\
it will soon explode. You should probably get far away from it.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("zombie pheromone",1,  400,'*', c_yellow,	FLESH,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    1,  1, -5,  0, -1,  3,  3,  1,  0, AT_NULL,	itm_null, &iuse::pheromone,0,"\
This is some kind of disgusting ball of rotting meat. Squeezing it causes a\n\
small cloud of pheromones to spray into the air, causing nearby zombies to\n\
become friendly for a short period of time.");

TOOL("portal generator",2, 6600, ';', c_magenta, STEEL,	PLASTIC,
    2, 10,  6,  0, -1, 10, 10,  5,  0, AT_NULL,	itm_null, &iuse::portal,0,"\
A rare and arcane device, covered in alien markings.");

TOOL("inactive manhack",1,  600, ',', c_ltgreen, STEEL, PLASTIC,
    1,  3,  6,  6, -3,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::manhack,0,"\
An inactive manhack. Manhacks are fist-sized robots which fly through the\n\
air. They are covered with whirring blades and attack by throwing themselves\n\
against their target. Use this item to activate the manhack.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("inactive turret",  1,4000,';',c_ltgreen,	STEEL,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    12, 12, 8,  0, -3,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::turret,0,"\
An inactive turret. Using this item involves turning it on and placing it\n\
on the ground, where it will attach itself. The turret will then identify\n\
you as a friendly, and attack all enemies with an SMG.");

TOOL("UPS (off)",	 5,2800,';',c_ltgreen,	STEEL,	PLASTIC,
   12,  6, 10,  0, -1,1000, 0,  0,  0, AT_BATT, itm_null, &iuse::UPS_off,0,"\
A unified power supply, or UPS, is a device developed jointly by military and\n\
scientific interests for use in combat and the field. The UPS is designed to\n\
power armor and some guns, but drains batteries quickly.");

TOOL("UPS (on)",	 0,2800,';',c_ltgreen,	STEEL,	PLASTIC,
   12,  6, 10,  0, -1,1000, 0,  0,  2, AT_BATT,	itm_UPS_off, &iuse::UPS_on,0,"\
A unified power supply, or UPS, is a device developed jointly by military and\n\
scientific interests for use in combat and the field. The UPS is designed to\n\
power armor and some guns, but drains batteries quickly.");

TOOL("tazer",		 3,1400,';',c_ltred,	IRON,	PLASTIC,
    1,  3,  6,  0, -1, 500, 0,100, 0, AT_BATT, itm_null, &iuse::tazer,0,"\
A high-powered stun gun. Use this item to attempt to electrocute an adjacent\n\
enemy, damaging and temporarily paralyzing them. Because the shock can\n\
actually jump through the air, it is difficult to miss.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("mp3 player (off)",18, 800,';',c_ltblue,	IRON,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
    1,  1,  0,  0,  0, 100,100, 0,  0, AT_BATT, itm_null, &iuse::mp3,0,"\
This battery-devouring device is loaded up with someone's music collection.\n\
Fortunately, there's lots of songs you like, and listening to it will raise\n\
your morale slightly. Use it to turn it on.");

TOOL("mp3 player (on)",	 0, 800,';',c_ltblue,	IRON,	PLASTIC,
    1,  1,  0,  0,  0, 100,100, 0, 10, AT_BATT, itm_mp3, &iuse::mp3_on,0,"\
This mp3 player is turned on and playing some great tunes, raising your\n\
morale steadily while on your person. It runs through batteries quickly; you\n\
can turn it off by using it. It also obscures your hearing.");

TOOL("vortex stone",     2,3000,';',c_pink,	STONE,	MNULL,
    2,  0,  6,  0,  0,   1,  1, 1,  0, AT_NULL, itm_null, &iuse::vortex,0,"\
A stone with spirals all over it, and holes around its perimeter. Though it\n\
is fairly large, it weighs next to nothing. Air seems to gather around it.");

TOOL("dog food",         5,  60,';',c_red,     FLESH,     MNULL,
    1,  2,  0,  0,  -5,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::dogfood, 0, "\
Food for dogs. It smells strange, but dogs love it.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("booby trap",        0,500,';',c_ltcyan,   STEEL,	PLASTIC,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
     3,  2,  0,  0, -4,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::set_trap, 0, "\
A crude explosive device triggered by a piece of string.");

TOOL("C4-Explosive",      5, 6000,';',c_ltcyan, PLASTIC,     STEEL,
     6,  2,  0,  0, -4,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::c4, 0, "\
Highly explosive, use with caution! Armed with a small timer.");

TOOL("C4-Explosive(armed)",0,6000,';',c_ltcyan, PLASTIC,     STEEL,
     6,  2,  0,  0, -4,  9,  9,  0,  1, AT_NULL, itm_null, &iuse::c4armed, 0, "\
Highly explosive, use with caution. Comes with a small timer.\n\
It's armed and ticking!");

TOOL("dog whistle",	  0,  300,';',c_white,	STEEL,	MNULL,
     0,  0,  0,  0,  0,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::dog_whistle,
0, "\
A small whistle. When used, it produces a high tone which causes nearby\n\
friendly dogs to either follow you closely and stop attacking, or to start\n\
attacking enemies if they are currently docile.");

//	NAME		RAR PRC SYM  COLOR	MAT1	MAT
TOOL("vacutainer",	 10,300,';', c_ltcyan,	PLASTIC,MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL	REVERT	  FUNCTION
     1,  0,  0,  6, -3,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::vacutainer,
mfb(IF_SPEAR), "\
A tool for drawing blood, including a vacuum-sealed test tube for holding the\n\
sample. Use this tool to draw blood, either from yourself or from a corpse\n\
you are standing on.");

//  NAME        RAR PRC SYM  COLOR  MAT1    MAT
TOOL("welder",   25,900,';', c_ltred,  IRON,MNULL,
// VOL WGT DAM CUT HIT   MAX DEF  USE SEC   FUEL    REVERT    FUNCTION
     6, 24,  7,  0, -1, 500,  0,  50,  0, AT_BATT, itm_null, &iuse::none,
0, "\
A tool for welding metal pieces together. Useful for construction.");

TOOL("cot",      40,1000,';', c_green, IRON, COTTON,
     8, 10, 6, 0, -1, 0, 0, 0, 0, AT_NULL, itm_null, &iuse::set_trap,
0, "\
A military style fold up cot, not quite as comfortable as a bed\n\
but much better than slumming it on the ground.");

TOOL("rollmat",  40,400,';', c_blue, MNULL, MNULL,
     4, 3,  0, 0, -1, 0, 0, 0, 0, AT_NULL, itm_null, &iuse::set_trap,
0, "\
A thin rollmat, better than sleeping on the ground.");

TOOL("steak knife",	85,  25,';', c_ltcyan,	STEEL,	MNULL,
     1,  2,  2, 10, -3, 0, 0, 0, 0, AT_NULL, itm_null, &iuse::knife,
 mfb(IF_STAB), "\
A sharp knife. Makes a poor melee weapon, but is decent at butchering\n\
corpses.");

TOOL("butcher knife",	10,  80,';', c_cyan,	STEEL,	MNULL,
	 3,  6,  4, 18, -3 ,0, 0, 0, 0, AT_NULL, itm_null, &iuse::none, 0, "\
A sharp, heavy knife. Makes a good melee weapon, and is the best item for\n\
butchering corpses.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
TOOL("combat knife",	14, 100,';', c_blue,	STEEL,  PLASTIC,
//	VOL WGT DAM CUT HIT FLAGS
	 2,  2,  2, 22, -2, 0, 0, 0, 0, AT_NULL, itm_null, &iuse::knife,
mfb(IF_STAB), "\
Designed for combat, and deadly in the right hands. Can be used to butcher\n\
corpses.");

TOOL("wood saw",	15, 40, ';', c_cyan,	IRON,	WOOD,
	 7,  3, -6,  1, -2, 0, 0, 0, 0, AT_NULL, itm_null, &iuse::lumber,
0, "\
A flimsy saw, useful for cutting through wood objects.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
TOOL("wood ax",	 8, 105,'/', c_ltgray,	WOOD,	IRON,
//	VOL WGT DAM CUT HIT FLAGS
	17, 15, 24, 18,  1, 0, 0, 0, 0, AT_NULL, itm_null, &iuse::lumber,
0, "\
A large two-handed axe. Makes a good melee weapon, but is a bit slow.");

TOOL("hack saw",	17, 65, ';', c_ltcyan,	IRON,	MNULL,
	 4,  2,  1,  1, -1, 0, 0, 0, 0, AT_NULL, itm_null, &iuse::hacksaw,
0, "\
A sturdy saw, useful for cutting through metal objects.");

//  NAME        RAR PRC SYM  COLOR  MAT1    MAT
TOOL("tent",	17, 65, ';', c_green,	IRON,	MNULL,
	 10,  20,  4,  0, -3, 0, 0, 0, 0, AT_NULL, itm_null, &iuse::tent,
0, "\
A small tent, just big enough to fit a person comfortably.");

TOOL("torch",    95,  0, '/', c_brown,   WOOD,   MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL     REVERT    FUNCTION
    6, 10, 12,  0,  3, 25, 25,  0, 0, AT_NULL, itm_null, &iuse::torch,
0,"\
A large stick, wrapped in gasoline soaked rags. When lit, produces\n\
a fair amount of light");

TOOL("torch (lit)",    95,  0, '/', c_brown,   WOOD,   MNULL,
// VOL WGT DAM CUT HIT MAX DEF USE SEC FUEL     REVERT    FUNCTION
    6, 10, 12,  0,  3, 25, 25,  1, 15, AT_NULL, itm_torch_done, &iuse::torch_lit,
mfb(IF_FIRE),"\
A large stick, wrapped in gasoline soaked rags. This is burning,\n\
producing plenty of light");

//    NAME              RAR PRC SYM COLOR       MAT1    MAT2
TOOL("candle",           40,  0, ',', c_white,  VEGGY,  MNULL,
    1,  1,  0,  0, -2, 100, 100, 0, 0, AT_NULL, itm_can_food, &iuse::candle,
0, "\
A thick candle, doesn't provide very much light, but it can burn for\n\
quite a long time.");

//    NAME              RAR PRC SYM COLOR       MAT1    MAT2
TOOL("candle (lit)",           40,  0, ',', c_white,  VEGGY,  MNULL,
    1,  1,  0,  0, -2, 100, 100, 1, 50, AT_NULL, itm_null, &iuse::candle_lit,
0, "\
A thick candle, doesn't provide very much light, but it can burn for\n\
quite a long time. This candle is lit.");

//  NAME        RAR PRC SYM  COLOR  MAT1    MAT
TOOL("brazier",  50,900,';', c_ltred,  IRON,MNULL,
// VOL WGT DAM CUT HIT   MAX DEF  USE SEC   FUEL    REVERT    FUNCTION
     6, 5,  11,  0, 1,    0,  0,  0,  0, AT_NULL, itm_null, &iuse::set_trap,
0, "\
A large stand, with a shallow bowl on top. Used for old school\n\
fire sconces.");

TOOL("kinetic bullet puller",		5, 100, ';', c_blue,	STEEL,	PLASTIC,
    2,  4, 10,  0,  0,   0,  0, 0,  0, AT_NULL, itm_null, &iuse::bullet_puller, 0, "\
A tool used for disassembling firearm ammunition.");

TOOL("hand press & die set",		5, 100, ';', c_blue,	STEEL,	PLASTIC,
    2,  4, 8,  0,  -2,   100, 100, 0,  0, AT_BATT, itm_null, &iuse::none, 0, "\
A small hand press for hand loading firearm ammunition. Comes with everything \n\
you need to start hand loading.");

TOOL("screwdriver",	40, 65, ';', c_ltcyan,	IRON,	PLASTIC,
	 1,  1,  2,  8,  1, 0, 0, 0, 0, AT_NULL, itm_null, &iuse::none, mfb(IF_SPEAR), "\
A Philips-head screwdriver, important for almost all electronics crafting and\n\
most mechanics crafting.");

//    NAME		RAR PRC SYM COLOR	MAT1	MAT2
TOOL("wrench",		30, 86, ';', c_ltgray,	IRON,	MNULL,
//	VOL WGT DAM CUT HIT FLAGS
	 2,  5, 15,  0,  2, 0, 0, 0, 0, AT_NULL, itm_null, &iuse::none, 0, "\
An adjustable wrench. Makes a decent melee weapon, and is used in many\n\
mechanics crafting recipes.");

TOOL("bolt cutters",		5, 100, ';', c_blue,	STEEL,	PLASTIC,
    5,  4, 10,  4,  1,   0,  0, 0,  0, AT_NULL, itm_null, &iuse::boltcutters, 0, "\
A large pair of bolt cutters, you could use them to cut padlocks or heavy\n\
gauge wire.");

TOOL("mop",		30, 24, '/', c_blue, PLASTIC, MNULL,
   10, 8,   6,  0,  1,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::mop, 0, "\
An unwieldy mop. Good for cleaning up spills.");
TECH( mfb(TEC_WBLOCK_1) );

TOOL("picklock kit",    20, 0,  ';', c_blue, STEEL,   MNULL,
   0,  0,   0,  0,  0,  0,  0,  0,  0, AT_NULL, itm_null, &iuse::picklock, 0, "\
A set of sturdy steel picklocks, essential for silently opening locks.");

// BIONICS
// These are the modules used to install new bionics in the player.  They're
// very simple and straightforward; a difficulty, followed by a NULL-terminated
// list of options.
#define BIO(name, rarity, price, color, difficulty, des, ...) \
	index++;itypes.push_back(new it_bionic(index,rarity,price,name,des,':',\
color, STEEL, PLASTIC, 10, 18, 8, 0, 0, 0, difficulty, __VA_ARGS__))
//  Name			RAR PRICE	COLOR		DIFFICULTY

BIO("CBM: Internal Battery",	24, 3800,	c_green,	 1, "\
Compact Bionics Module which upgrades your power capacity by 4 units. Having\n\
at least one of these is a prerequisite to using powered bionics. You will\n\
also need a power supply, found in another CBM.",
    NULL); // This is a special case, which increases power capacity by 4

BIO("CBM: Power Sources",	18, 5000,	c_yellow,	 4, "\
Compact Bionics Module containing the materials necessary to install any one\n\
of several power sources. Having a power source is necessary to use powered\n\
bionics.",
    bio_batteries, bio_metabolics, bio_solar, bio_furnace, bio_ethanol, NULL);

BIO("CBM: Utilities",		20, 2000,	c_ltgray,	 2, "\
Compact Bionics Module containing a variety of utilities. Popular among\n\
civilians, especially specialist laborers like mechanics.",
    bio_tools, bio_storage, bio_flashlight, bio_lighter, bio_magnet, NULL);

BIO("CBM: Neurological",	 8, 6000,	c_pink,		 8, "\
Compact Bionics Module containing a few upgrades to one's central nervous\n\
system or brain. Due to the difficulty associated with what is essentially\n\
brain surgery, these are best installed by a highly skill professional.",
    bio_memory, bio_painkiller, bio_alarm, NULL);

BIO("CBM: Sensory",		10, 4500,	c_ltblue,	 5, "\
Compact Bionics Module containing a few upgrades to one's sensory systems,\n\
particularly sight. Fairly difficult to install.",
    bio_ears, bio_eye_enhancer, bio_night_vision, bio_infrared, bio_scent_vision, NULL);

BIO("CBM: Aquatic",		 5, 3000,	c_blue,		 3, "\
Compact Bionics Module with a couple of upgrades designed with those who are\n\
often underwater; popular among diving enthusiasts and Navy SEAL teams.",
    bio_membrane, bio_gills, NULL);

BIO("CBM: Combat Augs",		10, 4500,	c_red,		 3, "\
Compact Bionics Module containing several augmentations designed to aid in\n\
combat. While none of these are weapons, all are very useful for improving\n\
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
dry, hot areas like a desert. Geared towards providing a source of water.",
    bio_climate, bio_recycler, bio_evap, bio_water_extractor, NULL);

BIO("CBM: Melee Combat",	 6, 5800,	c_red,		 3, "\
Compact Bionics Module containing a few upgrades designed for melee combat.\n\
Useful for those who like up-close combat.",
    bio_shock, bio_heat_absorb, bio_claws, NULL);

BIO("CBM: Armor",		12, 6800,	c_cyan,		 5, "\
Compact Bionics Module containing the supplies necessary to install one of\n\
several high-strength armors. Very popular.",
    bio_carbon, bio_armor_head, bio_armor_torso, bio_armor_arms, bio_armor_legs,
    NULL);

BIO("CBM: Espionage",		 5, 7500,	c_magenta,	 4, "\
Compact Bionics Module often used by high-tech spies. Its contents are\n\
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
of super-soldier. Due to the highly advanced technology used, this module is\n\
very difficult to install.",
    bio_time_freeze, bio_teleport, NULL);

BIO("CBM: Ranged Combat",	 7, 9200,	c_red,		 6, "\
Compact Bionics Module containing a few devices designed for ranged combat.\n\
Good for those who want a gun on occasion, but do not wish to carry lots of\n\
heavy ammunition and weapons.",
    bio_blaster, bio_laser, bio_emp, NULL);

// SOFTWARE

#define SOFTWARE(name, price, swtype, power, description) \
index++; itypes.push_back(new it_software(index, 0, price, name, description,\
	' ', c_white, MNULL, MNULL, 0, 0, 0, 0, 0, 0, swtype, power))
SOFTWARE("misc software", 300, SW_USELESS, 0, "\
A miscellaneous piece of hobby software. Probably useless.");

SOFTWARE("hackPRO", 800, SW_HACKING, 2, "\
A piece of hacking software.");

SOFTWARE("MediSoft", 600, SW_MEDICAL, 2, "\
A piece of medical software.");

SOFTWARE("MatheMAX", 500, SW_SCIENCE, 3, "\
A piece of mathematical software.");

SOFTWARE("infection data", 200, SW_DATA, 5, "\
Medical data on zombie blood.");


#define MACGUFFIN(name, price, sym, color, mat1, mat2, volume, wgt, dam, cut,\
                  to_hit, readable, function, description) \
index++; itypes.push_back(new it_macguffin(index, 0, price, name, description,\
	sym, color, mat1, mat2, volume, wgt, dam, cut, to_hit, 0, readable,\
	function))
MACGUFFIN("paper note", 0, '?', c_white, PAPER, MNULL, 1, 0, 0, 0, 0,
	true, &iuse::mcg_note, "\
A hand-written paper note.");

MELEE("poppy flower",   1, 400,',', c_magenta,	VEGGY,	MNULL,
	 1,  0, -8,  0, -3, 0, "\
A poppy stalk with some petals.");

MELEE("a poppy bud",   1, 400,',', c_magenta,	VEGGY,	MNULL,
	 1,  0, -8,  0, -3, 0, "\
Contains some substances commonly produced by mutated poppy flower");

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
	 2,  0,  8, 16,  4,
 mfb(IF_STAB)|mfb(IF_UNARMED_WEAPON)|mfb(IF_NO_UNWIELD), "\
Short and sharp claws made from a high-tech metal.");

//  NAME		RARE  TYPE	COLOR		MAT
AMMO("Fusion blast",	 0,0, AT_FUSION,c_dkgray,	MNULL,
//	VOL WGT DMG  AP RNG ACC REC COUNT
	 0,  0, 40,  0, 10,  1,  0,  5, "", mfb(AMMO_INCENDIARY));

//  NAME		RARE	COLOR		MAT1	MAT2
GUN("fusion blaster",	 0,0,c_magenta,	STEEL,	PLASTIC,
//	SKILL		AMMO	   VOL WGT MDG HIT DMG ACC REC DUR BST CLIP REL
	sk_rifle,	AT_FUSION, 12,  0,  0,  0,  0,  4,  0, 10,  0,  1, 500,
"",0);


// Unarmed Styles

#define STYLE(name, dam, description, ...) \
index++; \
itypes.push_back(new it_style(index, 0, 0, name, description, '$', \
                              c_white, MNULL, MNULL, 0, 0, dam, 0, 0, 0)); \
setvector((static_cast<it_style*>(itypes[index]))->moves, __VA_ARGS__, NULL); \
itypes[index]->item_flags |= mfb(IF_UNARMED_WEAPON)

/*
#define STYLE(name, description)\
index++; \
itypes.push_back(new it_style(index, 0, 0, name, description, '$', \
                              c_white, MNULL, MNULL, 0, 0, 0, 0, 0, 0))

STYLE("karate", "karate is cool");
*/

STYLE("karate", 2, "\
Karate is a popular martial art, originating from Japan. It focuses on\n\
rapid, precise attacks, blocks, and fluid movement. A successful hit allows\n\
you an extra dodge and two extra blocks on the following round.",

"quickly punch", TEC_RAPID, 0,
"block", TEC_BLOCK, 2,
"karate chop", TEC_PRECISE, 4
);

STYLE("aikido", 0, "\
Aikido is a Japanese martial art focused on self-defense, while minimizing\n\
injury to the attacker. It uses defense throws and disarms. Damage done\n\
while using this technique is halved, but pain inflicted is doubled.",

"feint at", TEC_FEINT, 2,
"throw", TEC_DEF_THROW, 2,
"disarm", TEC_DISARM, 3,
"disarm", TEC_DEF_DISARM, 4
);

STYLE("judo", 0, "\
Judo is a martial art which focuses on grabs and throws, both defensive and\n\
offensive. It also focuses on recovering from throws; while using judo, you\n\
will not lose any turns to being thrown or knocked down.",

"grab", TEC_GRAB, 2,
"throw", TEC_THROW, 3,
"throw", TEC_DEF_THROW, 4
);

STYLE("tai chi", 0, "\
Though tai chi is often seen as a form of mental and physical exercise, it is\n\
a legitimate martial art, focused on self-defense. Its ability to absorb the\n\
force of an attack makes your Perception decrease damage further on a block.",

"block", TEC_BLOCK, 1,
"disarm", TEC_DEF_DISARM, 3,
"strike", TEC_PRECISE, 4
);

STYLE("capoeira", 1, "\
A dance-like style with its roots in Brazilian slavery, capoeira is focused\n\
on fluid movement and sweeping kicks. Moving a tile will boost attack and\n\
dodge; attacking boosts dodge, and dodging boosts attack.",

"bluff", TEC_FEINT, 1,
"low kick", TEC_SWEEP, 3,
"spin and hit", TEC_COUNTER, 4,
"spin-kick", TEC_WIDE, 5
);

STYLE("krav maga", 4, "\
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

STYLE("muay thai", 4, "\
Also referred to as the \"Art of 8 Limbs,\" Muay Thai is a popular fighting\n\
technique from Thailand which uses powerful strikes. It does extra damage\n\
against large or strong opponents.",

"slap", TEC_RAPID, 2,
"block", TEC_BLOCK, 3,
"block", TEC_BLOCK_LEGS, 4,
"power-kick", TEC_BRUTAL, 4,
"counter-attack", TEC_COUNTER, 5
);

STYLE("ninjutsu", 1, "\
Ninjutsu is a martial art and set of tactics used by ninja in feudal Japan.\n\
It focuses on rapid, precise, silent strikes. Ninjutsu is entirely silent.\n\
It also provides small combat bonuses the turn after moving a tile.",

"quickly punch", TEC_RAPID, 3,
"jab", TEC_PRECISE, 4,
"block", TEC_BLOCK, 4
);

STYLE("taekwondo", 2, "\
Taekwondo is the national sport of Korea, and was used by the South Korean\n\
army in the 20th century. Focused on kicks and punches, it also includes\n\
strength training; your blocks absorb extra damage the stronger you are.",

"block", TEC_BLOCK, 2,
"block", TEC_BLOCK_LEGS, 3,
"jab", TEC_PRECISE, 4,
"brutally kick", TEC_BRUTAL, 4,
"spin-kick", TEC_SWEEP, 5
);

STYLE("tiger style", 4, "\
One of the five Shaolin animal styles. Tiger style focuses on relentless\n\
attacks above all else. Strength, not Dexterity, is used to determine hits;\n\
you also receive an accumulating bonus for several turns of sustained attack.",

"grab", TEC_GRAB, 4
);

STYLE("crane style", 0, "\
One of the five Shaolin animal styles. Crane style uses intricate hand\n\
techniques and jumping dodges. Dexterity, not Strength, is used to determine\n\
damage; you also receive a dodge bonus the turn after moving a tile.",

"feint at", TEC_FEINT, 2,
"block", TEC_BLOCK, 3,
"", TEC_BREAK, 3,
"hand-peck", TEC_PRECISE, 4
);

STYLE("leopard style", 3, "\
One of the five Shaolin animal styles. Leopard style focuses on rapid,\n\
strategic strikes. Your Perception and Intelligence boost your accuracy, and\n\
moving a single tile provides an increased boost for one turn.",

"swiftly jab", TEC_RAPID, 2,
"counter-attack", TEC_COUNTER, 4,
"leopard fist", TEC_PRECISE, 5
);

STYLE("snake style", 1, "\
One of the five Shaolin animal styles. Snake style uses sinuous movement and\n\
precision strikes. Perception increases your chance to hit as well as the\n\
damage you deal.",

"swiftly jab", TEC_RAPID, 2,
"feint at", TEC_FEINT, 3,
"snakebite", TEC_PRECISE, 4,
"writhe free from", TEC_BREAK, 4
);

STYLE("dragon style", 2, "\
One of the five Shaolin animal styles. Dragon style uses fluid movements and\n\
hard strikes. Intelligence increases your chance to hit as well as the\n\
damage you deal. Moving a tile will boost damage further for one turn.",

"", TEC_BLOCK, 2,
"grab", TEC_GRAB, 4,
"counter-attack", TEC_COUNTER, 4,
"spin-kick", TEC_SWEEP, 5,
"dragon strike", TEC_BRUTAL, 6
);

STYLE("centipede style", 0, "\
One of the Five Deadly Venoms. Centipede style uses an onslaught of rapid\n\
strikes. Every strike you make reduces the movement cost of attacking by 4;\n\
this is cumulative, but is reset entirely if you are hit even once.",

"swiftly hit", TEC_RAPID, 2,
"block", TEC_BLOCK, 3
);

STYLE("viper style", 2, "\
One of the Five Deadly Venoms. Viper Style has a unique three-hit combo; if\n\
you score a critical hit, it is initiated. The second hit uses a coned hand\n\
to deal piercing damage, and the 3rd uses both hands in a devastating strike.",

"", TEC_RAPID, 3,
"feint at", TEC_FEINT, 3,
"writhe free from", TEC_BREAK, 4
);

STYLE("scorpion style", 3, "\
One of the Five Deadly Venoms. Scorpion Style is a mysterious art which uses\n\
pincer-like fists and a stinger-like kick, which is used when you score a\n\
critical hit, and does massive damage, knocking your target far back.",

"block", TEC_BLOCK, 3,
"pincer fist", TEC_PRECISE, 4
);

STYLE("lizard style", 1, "\
One of the Five Deadly Venoms. Lizard Style focuses on using walls to one's\n\
advantage. Moving alongside a wall will make you run up along it, giving you\n\
a large to-hit bonus. Standing by a wall allows you to use it to boost dodge.",

"block", TEC_BLOCK, 2,
"counter-attack", TEC_COUNTER, 4
);

STYLE("toad style", 0, "\
One of the Five Deadly Venoms. Immensely powerful, and immune to nearly any\n\
weapon. You may meditate by pausing for a turn; this will give you temporary\n\
armor, proportional to your Intelligence and Perception.",

"block", TEC_BLOCK, 3,
"grab", TEC_GRAB, 4
);

STYLE("zui quan", 1, "\
Also known as \"drunken boxing,\" Zui Quan imitates the movement of a drunk\n\
to confuse the enemy. The turn after you attack, you may dodge any number of\n\
attacks with no penalty.",

"stumble and leer at", TEC_FEINT, 3,
"counter-attack", TEC_COUNTER, 4
);


 if (itypes.size() != num_all_items)
  debugmsg("%d items, %d itypes (+bio)", itypes.size(), num_all_items - 1);

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

   art->id = itypes.size();
   itypes.push_back(art);

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

   art->id = itypes.size();
   itypes.push_back(art);

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
  case AT_GAS:	  return "gasoline";
  case AT_THREAD: return "thread";
  case AT_BATT:   return "batteries";
  case AT_PLUT:   return "plutonium";
  case AT_MUSCLE: return "Muscle";
  case AT_FUSION: return "fusion cell";
  case AT_12MM:   return "12mm slugs";
  case AT_PLASMA: return "hydrogen";
  default:	  return "XXX";
 }
}

itype_id default_ammo(ammotype guntype)
{
 switch (guntype) {
 case AT_NAIL:	return itm_nail;
 case AT_BB:	return itm_bb;
 case AT_BOLT:	return itm_bolt_wood;
 case AT_ARROW: return itm_arrow_wood;
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
 case AT_40MM:  return itm_40mm_concussive;
 case AT_BATT:	return itm_battery;
 case AT_FUSION:return itm_laser_pack;
 case AT_12MM:  return itm_12mm;
 case AT_PLASMA:return itm_plasma;
 case AT_PLUT:	return itm_plut_cell;
 case AT_GAS:	return itm_gasoline;
 case AT_THREAD:return itm_thread;
 }
 return itm_null;
}

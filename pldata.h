#ifndef _PLDATA_H_
#define _PLDATA_H_

#include "enums.h"
#include "translations.h"
#include <sstream>
#include <vector>

enum character_type {
 PLTYPE_CUSTOM,
 PLTYPE_RANDOM,
 PLTYPE_TEMPLATE,
 PLTYPE_MAX
};

typedef std::string dis_type;

enum add_type {
 ADD_NULL,
 ADD_CAFFEINE, ADD_ALCOHOL, ADD_SLEEP, ADD_PKILLER, ADD_SPEED, ADD_CIG,
 ADD_COKE, ADD_CRACK,
};

struct disease
{
 dis_type type;
 int intensity;
 int duration;
 disease() { type = "null"; duration = 0; intensity = 0; }
 disease(dis_type t, int d, int i = 0) { type = t; duration = d; intensity = i;}
};

struct addiction
{
 add_type type;
 int intensity;
 int sated;
 addiction() { type = ADD_NULL; intensity = 0; sated = 600; }
 addiction(add_type t) { type = t; intensity = 1; sated = 600; }
 addiction(add_type t, int i) { type = t; intensity = i; sated = 600; }
};

enum activity_type {
 ACT_NULL = 0,
 ACT_RELOAD, ACT_READ, ACT_WAIT, ACT_CRAFT, ACT_LONGCRAFT,
 ACT_DISASSEMBLE, ACT_BUTCHER, ACT_FORAGE, ACT_BUILD, ACT_VEHICLE, ACT_REFILL_VEHICLE,
 ACT_TRAIN,
 NUM_ACTIVITIES
};

struct player_activity
{
 activity_type type;
 int moves_left;
 int index;
 char invlet;
 std::string name;
 bool continuous;
 bool ignore_trivial;
 std::vector<int> values;
 point placement;

 player_activity() { type = ACT_NULL; moves_left = 0; index = -1; invlet = 0;
                     name = ""; placement = point(-1, -1); continuous = false; }

 player_activity(activity_type t, int turns, int Index, char ch, std::string name_in)
 {
  type = t;
  moves_left = turns;
  index = Index;
  invlet = ch;
  name = name_in;
  placement = point(-1, -1);
  continuous = false;
  ignore_trivial = false;
 }

 player_activity(const player_activity &copy)
 {
  type = copy.type;
  moves_left = copy.moves_left;
  index = copy.index;
  invlet = copy.invlet;
  name = copy.name;
  placement = copy.placement;
  continuous = copy.continuous;
  ignore_trivial = copy.ignore_trivial;
  values.clear();
  for (int i = 0; i < copy.values.size(); i++)
   values.push_back(copy.values[i]);
 }

 std::string save_info()
 {
  std::stringstream ret;
  // name can be empty, so make sure we prepend something to it
  ret << type << " " << moves_left << " " << index << " " << invlet << " str:" << name << " "
         << placement.x << " " << placement.y << " " << values.size();
  for (int i = 0; i < values.size(); i++)
   ret << " " << values[i];

  return ret.str();
 }

 void load_info(std::stringstream &dump)
 {
  int tmp, tmptype;
  std::string tmpname;
  dump >> tmptype >> moves_left >> index >> invlet >> tmpname >> placement.x >> placement.y >> tmp;
  name = tmpname.substr(4);
  type = activity_type(tmptype);
  for (int i = 0; i < tmp; i++) {
   int tmp2;
   dump >> tmp2;
   values.push_back(tmp2);
  }
 }
};

enum pl_flag {
 PF_NULL = 0,
 PF_FLEET,	// -15% terrain movement cost
 PF_PARKOUR,	// Terrain movement cost of 3 or 4 are both 2
 PF_QUICK,	// +10% movement points
 PF_OPTIMISTIC,	// Morale boost
 PF_FASTHEALER,	// Heals faster
 PF_LIGHTEATER,	// Less hungry
 PF_PAINRESIST,	// Effects of pain are reduced
 PF_NIGHTVISION,// Increased sight during the night
 PF_POISRESIST,	// Resists poison, etc
 PF_FASTREADER,	// Reads books faster
 PF_TOUGH,	// Bonus to HP
 PF_THICKSKIN,	// Built-in armor of 1
 PF_PACKMULE,	// Bonus to carried volume
 PF_FASTLEARNER,// Better chance of skills leveling up
 PF_DEFT,	// Less movement penalty for melee miss
 PF_DRUNKEN,	// Having a drunk status improves melee combat
 PF_GOURMAND,	// Faster eating, higher level of max satiated
 PF_ANIMALEMPATH,// Animals attack less
 PF_TERRIFYING,	// All creatures run away more
 PF_DISRESISTANT,// Less likely to succumb to low health
 PF_ADRENALINE,	// Big bonuses when low on HP
 PF_SELFAWARE, // Let's you see exact HP totals
 PF_INCONSPICUOUS,// Less spawns due to timeouts
 PF_MASOCHIST,	// Morale boost from pain
 PF_CROSSDRESSER, // Morale bonus for covering body parts in clothing typical of the opposite gender.
 PF_LIGHTSTEP,	// Less noise from movement
 PF_ANDROID,	// Start with two bionics (occasionally one)
 PF_ROBUST,	// Mutations tend to be good (usually they tend to be bad)
 PF_CANNIBAL, // No penalty for eating human meat
 PF_MARTIAL_ARTS, // Start with a martial art
 PF_MARTIAL_ARTS2, // Start with a martial art
 PF_MARTIAL_ARTS3, // Start with a martial art
 PF_MARTIAL_ARTS4, // Start with a martial art
 PF_LIAR, // Better at telling lies
 PF_PRETTY, // -1 grotesqueness

 PF_SPLIT,	// Null trait, splits between bad & good

 PF_MYOPIC,	// Smaller sight radius UNLESS wearing glasses
 PF_HYPEROPIC, // With no reading glasses, can't read and takes melee penalty
 PF_HEAVYSLEEPER, // Sleeps in, won't wake up to sounds as easily
 PF_ASTHMA,	// Occasionally needs medicine or suffers effects
 PF_BADBACK,	// Carries less
 PF_ILLITERATE,	// Can not read books
 PF_BADHEARING,	// Max distance for heard sounds is halved
 PF_INSOMNIA,	// Sleep may not happen
 PF_VEGETARIAN,	// Morale penalty for eating meat
 PF_GLASSJAW,	// Head HP is 15% lower
 PF_FORGETFUL,	// Skills decrement faster
 PF_LIGHTWEIGHT,// Longer DI_DRUNK and DI_HIGH
 PF_ADDICTIVE,	// Better chance of addiction / longer addictive effects
 PF_TRIGGERHAPPY,// Possible chance of unintentional burst fire
 PF_SMELLY,	// Default scent is higher
 PF_CHEMIMBALANCE,// Random tweaks to some values
 PF_SCHIZOPHRENIC,// Random bad effects, variety
 PF_JITTERY,	// Get DI_SHAKES under some circumstances
 PF_HOARDER,	// Morale penalty when volume is less than max
 PF_SAVANT,	// All skills except our best are trained more slowly
 PF_MOODSWINGS,	// Big random shifts in morale
 PF_WEAKSTOMACH,// More likely to throw up in all circumstances
 PF_WOOLALLERGY,// Can't wear wool
 PF_TRUTHTELLER, // Worse at telling lies
 PF_UGLY, // +1 grotesqueness
 PF_HARDCORE,	// Bodyhp is 75% lower

 PF_MAX,
// Below this point is mutations and other mid-game perks.
// They are NOT available during character creation.
 PF_SKIN_ROUGH,//
 PF_NIGHTVISION2,//
 PF_NIGHTVISION3,//
 PF_INFRARED,//
 PF_FASTHEALER2,//
 PF_REGEN,//
 PF_FANGS,//
 PF_MEMBRANE,//
 PF_GILLS,//
 PF_SCALES,//
 PF_THICK_SCALES,//
 PF_SLEEK_SCALES,//
 PF_LIGHT_BONES,//
 PF_FEATHERS,//
 PF_LIGHTFUR,// TODO: Warmth effects
 PF_FUR,// TODO: Warmth effects
 PF_CHITIN,//
 PF_CHITIN2,//
 PF_CHITIN3,//
 PF_SPINES,//
 PF_QUILLS,//
 PF_PLANTSKIN,//
 PF_BARK,//
 PF_THORNS,
 PF_LEAVES,//
 PF_NAILS,
 PF_CLAWS,
 PF_TALONS,//
 PF_RADIOGENIC,//
 PF_MARLOSS,//
 PF_PHEROMONE_INSECT,//
 PF_PHEROMONE_MAMMAL,//
 PF_DISIMMUNE,
 PF_POISONOUS,//
 PF_SLIME_HANDS,
 PF_COMPOUND_EYES,//
 PF_PADDED_FEET,//
 PF_HOOVES,//
 PF_SAPROVORE,//
 PF_RUMINANT,//
 PF_HORNS,//
 PF_HORNS_CURLED,//
 PF_HORNS_POINTED,//
 PF_ANTENNAE,//
 PF_FLEET2,//
 PF_TAIL_STUB,//
 PF_TAIL_FIN,//
 PF_TAIL_LONG,//
 PF_TAIL_FLUFFY,//
 PF_TAIL_STING,//
 PF_TAIL_CLUB,//
 PF_PAINREC1,//
 PF_PAINREC2,//
 PF_PAINREC3,//
 PF_WINGS_BIRD,//
 PF_WINGS_INSECT,//
 PF_MOUTH_TENTACLES,//
 PF_MANDIBLES,//
 PF_CANINE_EARS,
 PF_WEB_WALKER,
 PF_WEB_WEAVER,
 PF_WHISKERS,
 PF_STR_UP,
 PF_STR_UP_2,
 PF_STR_UP_3,
 PF_STR_UP_4,
 PF_DEX_UP,
 PF_DEX_UP_2,
 PF_DEX_UP_3,
 PF_DEX_UP_4,
 PF_INT_UP,
 PF_INT_UP_2,
 PF_INT_UP_3,
 PF_INT_UP_4,
 PF_PER_UP,
 PF_PER_UP_2,
 PF_PER_UP_3,
 PF_PER_UP_4,

 PF_HEADBUMPS,//
 PF_ANTLERS,//
 PF_SLIT_NOSTRILS,//
 PF_FORKED_TONGUE,//
 PF_EYEBULGE,//
 PF_MOUTH_FLAPS,//
 PF_WINGS_STUB,//
 PF_WINGS_BAT,//
 PF_PALE,//
 PF_SPOTS,//
 PF_SMELLY2,//TODO: NPC reaction
 PF_DEFORMED,
 PF_DEFORMED2,
 PF_DEFORMED3,
 PF_BEAUTIFUL,
 PF_BEAUTIFUL2,
 PF_BEAUTIFUL3,
 PF_HOLLOW_BONES,//
 PF_NAUSEA,//
 PF_VOMITOUS,//
 PF_HUNGER,//
 PF_THIRST,//
 PF_ROT1,//
 PF_ROT2,//
 PF_ROT3,//
 PF_ALBINO,//
 PF_SORES,//
 PF_TROGLO,//
 PF_TROGLO2,//
 PF_TROGLO3,//
 PF_WEBBED,//
 PF_BEAK,//
 PF_UNSTABLE,//
 PF_RADIOACTIVE1,//
 PF_RADIOACTIVE2,//
 PF_RADIOACTIVE3,//
 PF_SLIMY,//
 PF_HERBIVORE,//
 PF_CARNIVORE,//
 PF_PONDEROUS1,	// 10% movement penalty
 PF_PONDEROUS2, // 20%
 PF_PONDEROUS3, // 30%
 PF_SUNLIGHT_DEPENDENT,//
 PF_COLDBLOOD,//
 PF_COLDBLOOD2,//
 PF_COLDBLOOD3,//
 PF_GROWL,//
 PF_SNARL,//
 PF_SHOUT1,//
 PF_SHOUT2,//
 PF_SHOUT3,//
 PF_ARM_TENTACLES,
 PF_ARM_TENTACLES_4,
 PF_ARM_TENTACLES_8,
 PF_SHELL,
 PF_LEG_TENTACLES,

 PF_MAX2
};

struct trait {
 std::string name;
 int points;		// How many points it costs in character creation
 int visiblity;		// How visible it is--see below, at PF_MAX
 int ugliness;		// How ugly it is--see below, at PF_MAX
 std::string description;
};

extern trait traits[PF_MAX2];

enum hp_part {
 hp_head = 0,
 hp_torso,
 hp_arm_l,
 hp_arm_r,
 hp_leg_l,
 hp_leg_r,
 num_hp_parts
};
#endif

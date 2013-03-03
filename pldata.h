#ifndef _PLDATA_H_
#define _PLDATA_H_

#include <sstream>

enum character_type {
 PLTYPE_CUSTOM,
 PLTYPE_RANDOM,
 PLTYPE_TEMPLATE,
 PLTYPE_MAX
};

const std::string pltype_name[PLTYPE_MAX] = {
"Custom Character", "Random Character", "Template Character"};

const std::string pltype_desc[PLTYPE_MAX] = { "\
A custom character you design yourself.  A pool of points is used for\n\
statistics, traits, and skills.",
"\
A character with stats, traits, and skills chosen randomly.",
"\
A character loaded from a template file.",
};

enum dis_type {
 DI_NULL,
// Weather
 DI_GLARE, DI_WET,
// Temperature, the order is important
 DI_COLD,
 DI_COLD_HEAD, DI_COLD_EYES, DI_COLD_MOUTH, DI_COLD_TORSO,
 DI_COLD_ARMS, DI_COLD_HANDS, DI_COLD_LEGS, DI_COLD_FEET,
 DI_FROSTBITE,
 DI_FROSTBITE_HEAD, DI_FROSTBITE_EYES, DI_FROSTBITE_MOUTH, DI_FROSTBITE_TORSO,
 DI_FROSTBITE_ARMS, DI_FROSTBITE_HANDS, DI_FROSTBITE_LEGS, DI_FROSTBITE_FEET,
 DI_HOT,
 DI_HOT_HEAD, DI_HOT_EYES, DI_HOT_MOUTH, DI_HOT_TORSO,
 DI_HOT_ARMS, DI_HOT_HANDS, DI_HOT_LEGS, DI_HOT_FEET,
 DI_BLISTERS,
 DI_BLISTERS_HEAD, DI_BLISTERS_EYES, DI_BLISTERS_MOUTH, DI_BLISTERS_TORSO,
 DI_BLISTERS_ARMS, DI_BLISTERS_HANDS, DI_BLISTERS_LEGS, DI_BLISTERS_FEET,
// Diseases
 DI_INFECTION,
 DI_COMMON_COLD, DI_FLU,
// Fields
 DI_SMOKE, DI_ONFIRE, DI_TEARGAS,
// Monsters
 DI_BOOMERED, DI_SAP, DI_SPORES, DI_FUNGUS, DI_SLIMED,
 DI_DEAF, DI_BLIND,
 DI_LYING_DOWN, DI_SLEEP,
 DI_POISON, DI_BLEED, DI_BADPOISON, DI_FOODPOISON, DI_SHAKES,
 DI_DERMATIK, DI_FORMICATION,
 DI_WEBBED,
 DI_RAT,
// Food & Drugs
 DI_PKILL1, DI_PKILL2, DI_PKILL3, DI_PKILL_L, DI_DRUNK, DI_CIG, DI_HIGH,
  DI_HALLU, DI_VISUALS, DI_IODINE, DI_TOOK_XANAX, DI_TOOK_PROZAC,
  DI_TOOK_FLUMED, DI_ADRENALINE, DI_ASTHMA, DI_GRACK, DI_METH,
// Traps
 DI_BEARTRAP, DI_IN_PIT, DI_STUNNED, DI_DOWNED,
// Martial Arts
 DI_ATTACK_BOOST, DI_DAMAGE_BOOST, DI_DODGE_BOOST, DI_ARMOR_BOOST,
  DI_SPEED_BOOST, DI_VIPER_COMBO,
// Other
 DI_AMIGARA, DI_STEMCELL_TREATMENT, DI_TELEGLOW, DI_ATTENTION, DI_EVIL,
// Inflicted by an NPC
 DI_ASKED_TO_FOLLOW, DI_ASKED_TO_LEAD, DI_ASKED_FOR_ITEM,
// NPC-only
 DI_CATCH_UP
};

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
 disease() { type = DI_NULL; duration = 0; intensity = 0; }
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
 ACT_RELOAD, ACT_READ, ACT_WAIT, ACT_CRAFT, ACT_DISASSEMBLE, ACT_BUTCHER, ACT_FORAGE, ACT_BUILD,
 ACT_VEHICLE, ACT_REFILL_VEHICLE,
 ACT_TRAIN,
 NUM_ACTIVITIES
};

struct player_activity
{
 activity_type type;
 int moves_left;
 int index;
 std::vector<int> values;
 point placement;

 player_activity() { type = ACT_NULL; moves_left = 0; index = -1;
                     placement = point(-1, -1); }

 player_activity(activity_type t, int turns, int Index)
 {
  type = t;
  moves_left = turns;
  index = Index;
  placement = point(-1, -1);
 }

 player_activity(const player_activity &copy)
 {
  type = copy.type;
  moves_left = copy.moves_left;
  index = copy.index;
  placement = copy.placement;
  values.clear();
  for (int i = 0; i < copy.values.size(); i++)
   values.push_back(copy.values[i]);
 }

 std::string save_info()
 {
  std::stringstream ret;
  ret << type << " " << moves_left << " " << index << " " << placement.x <<
         " " << placement.y << " " << values.size();
  for (int i = 0; i < values.size(); i++)
   ret << " " << values[i];

  return ret.str();
 }

 void load_info(std::stringstream &dump)
 {
  int tmp, tmptype;
  dump >> tmptype >> moves_left >> index >> placement.x >> placement.y >> tmp;
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
 PF_DISRESISTANT,// Less likely to succumb to low health; TODO: Implement this
 PF_ADRENALINE,	// Big bonuses when low on HP
 PF_INCONSPICUOUS,// Less spawns due to timeouts
 PF_MASOCHIST,	// Morale boost from pain
 PF_LIGHTSTEP,	// Less noise from movement
 PF_HEARTLESS,	// No morale penalty for murder &c
 PF_ANDROID,	// Start with two bionics (occasionally one)
 PF_ROBUST,	// Mutations tend to be good (usually they tend to be bad)
 PF_MARTIAL_ARTS, // Start with a martial art

 PF_SPLIT,	// Null trait, splits between bad & good

 PF_MYOPIC,	// Smaller sight radius UNLESS wearing glasses
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
 PF_HPIGNORANT,	// Don't get to see exact HP numbers, just colors & symbols
 PF_TRUTHTELLER, // Worse at telling lies
 PF_UGLY, // +1 grotesqueness

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

const trait traits[] = {
{"NULL trait!", 0, 0, 0, "\
This is a bug.  Weird."},
{"Fleet-Footed", 3, 0, 0, "\
You can run more quickly than most, resulting in a 15%% speed bonus on sure\n\
footing."},
{"Parkour Expert", 2, 0, 0, "\
You're skilled at clearing obstacles; terrain like railings or counters are\n\
as easy for you to move on as solid ground."},
{"Quick", 3, 0, 0, "\
You're just generally quick!  You get a 10%% bonus to action points."},
{"Optimist", 2, 0, 0, "\
Nothing gets you down!  Your morale will generally be higher than average."},
{"Fast Healer", 2, 0, 0, "\
You heal a little faster than most; sleeping will heal more lost HP."},
{"Light Eater", 3, 0, 0, "\
Your metabolism is a little slower, and you require less food than most."},
{"Pain Resistant", 2, 0, 0, "\
You have a high tolerance for pain."},
{"Night Vision", 1, 0, 0, "\
You possess natural night vision, and can see two squares instead of one in\n\
pitch blackness."},
{"Poison Resistant", 1, 0, 0, "\
Your system is rather tolerant of poisons and toxins, and most will affect\n\
you less."},
{"Fast Reader", 1, 0, 0, "\
You're a quick reader, and can get through books a lot faster than most."},
{"Tough", 5, 0, 0, "\
It takes a lot to bring you down!  You get a 20%% bonus to all hit points."},
{"Thick-Skinned", 2, 0, 0, "\
Your skin is tough.  Cutting damage is slightly reduced for you."},
{"Packmule", 3, 0, 0, "\
You can manage to find space for anything!  You can carry 40%% more volume."},
{"Fast Learner", 3, 0, 0, "\
Your skill comprehension is 50%% higher, allowing you to learn skills much\n\
faster than others.  Note that this only applies to real-world experience,\n\
not to skill gain from other sources like books."},
{"Deft", 2, 0, 0, "\
While you're not any better at melee combat, you are better at recovering\n\
from a miss, and will be able to attempt another strike faster."},
{"Drunken Master", 2, 0, 0, "\
The martial art technique of Zui Quan, or Drunken Fist, comes naturally to\n\
you.  While under the influence of alcohol, your melee skill will rise\n\
considerably, especially unarmed combat."},
{"Gourmand", 2, 0, 0, "\
You eat faster, and can eat and drink more, than anyone else!  You also enjoy\n\
food more; delicious food is better for your morale, and you don't mind some\n\
unsavory meals."},
{"Animal Empathy", 1, 0, 0, "\
Peaceful animals will not run away from you, and even aggressive animals are\n\
less likely to attack.  This only applies to natural animals such as woodland\n\
creatures."},
{"Terrifying", 2, 0, 0, "\
There's something about you that creatures find frightening, and they are\n\
more likely to try to flee."},
{"Disease Resistant", 1, 0, 0, "\
It's very unlikely that you will catch ambient diseases like a cold or the\n\
flu."},
{"High Adrenaline", 3, 0, 0, "\
If you are in a very dangerous situation, you may experience a temporary rush\n\
which increases your speed and strength significantly."},
{"Inconspicuous", 2, 0, 0, "\
While sleeping or staying still, it is less likely that monsters will wander\n\
close to you."},
{"Masochist", 2, 0, 0, "\
Although you still suffer the negative effects of pain, it also brings a\n\
unique pleasure to you."},
{"Light Step", 1, 0, 0, "\
You make less noise while walking.  You're also less likely to set off traps."},
{"Heartless", 2, 0, 0, "\
You have few qualms, and no capacity for pity. Killing the helpless, the\n\
young, and your friends will not affect your morale at all."},
{"Android", 4, 0, 0, "\
At some point in the past you had a bionic upgrade installed in your body.\n\
You start the game with a power system, and one random bionic enhancement."},
{"Robust Genetics", 2, 0, 0, "\
You have a very strong genetic base.  If you mutate, the odds that the\n\
mutation will be beneficial are greatly increased."},
{"Martial Arts Training", 3, 0, 0, "\
You have receives some martial arts training at a local dojo.  You will start\n\
with your choice of karate, judo, aikido, tai chi, or taekwando."},

{"NULL", 0, 0, 0, " -------------------------------------------------- "},

{"Near-Sighted", -2, 0, 0, "\
Without your glasses, your seeing radius is severely reduced!  However, while\n\
wearing glasses this trait has no effect, and you are guaranteed to start\n\
with a pair."},
{"Heavy Sleeper", -1, 0, 0, "\
You're quite the heavy sleeper.  Noises are unlikely to wake you up."},
{"Asthmatic", -4, 0, 0, "\
You will occasionally need to use an inhaler, or else suffer severe physical\n\
limitations.  However, you are guaranteed to start with an inhaler."},
{"Bad Back", -3, 0, 0, "\
You simply can not carry as much as people with a similar strength could.\n\
Your maximum weight carried is reduced by 35%%."},
{"Illiterate", -5, 0, 0, "\
You never learned to read!  Books and computers are off-limits to you."},
{"Poor Hearing", -2, 0, 0, "\
Your hearing is poor, and you may not hear quiet or far-off noises."},
{"Insomniac", -2, 0, 0, "\
You have a hard time falling asleep, even under the best circumstances!"},
{"Meat Allergy", -3, 0, 0, "\
You have problems with eating meat, it's possible for you to eat it but\n\
you will suffer morale penalties due to nausea."},
{"Glass Jaw", -3, 0, 0, "\
Your head can't take much abuse.  Its maximum HP is 15%% lower than usual."},
{"Forgetful", -3, 0, 0, "\
You have a hard time remembering things.  Your skills will erode slightly\n\
faster than usual."},
{"Lightweight", -1, 0, 0, "\
Alcohol and drugs go straight to your head.  You suffer the negative effects\n\
of these for longer."},
{"Addictive Personality", -3, 0, 0, "\
It's easier for you to become addicted to substances, and harder to rid\n\
yourself of these addictions."},
{"Trigger Happy", -2, 0, 0, "\
On rare occasion, you will go full-auto when you intended to fire a single\n\
shot.  This has no effect when firing handguns or other semi-automatic\n\
firearms."},
{"Smelly", -1, 0, 0, "\
Your scent is particularly strong.  It's not offensive to humans, but animals\n\
that track your scent will do so more easily."},
{"Chemical Imbalance", -2, 0, 0, "\
You suffer from a minor chemical imbalance, whether mental or physical. Minor\n\
changes to your internal chemistry will manifest themselves on occasion,\n\
such as hunger, sleepiness, narcotic effects, etc."},
{"Schizophrenic", -5, 0, 0, "\
You will periodically suffer from delusions, ranging from minor effects to\n\
full visual hallucinations.  Some of these effects may be controlled through\n\
the use of Thorazine."},
{"Jittery", -3, 0, 0, "\
During moments of great stress or under the effects of stimulants, you may\n\
find your hands shaking uncontrollably, severely reducing your dexterity."},
{"Hoarder", -4, 0, 0, "\
You don't feel right unless you're carrying as much as you can.  You suffer\n\
morale penalties for carrying less than maximum volume (weight is ignored).\n\
Xanax can help control this anxiety."},
{"Savant", -4, 0, 0, "\
You tend to specialize in one skill and be poor at all others.  You advance\n\
at half speed in all skills except your best one. Note that combining this\n\
with Fast Learner will come out to a slower rate of learning for all skills."},
{"Mood Swings", -1, 0, 0, "\
Your morale will shift up and down at random, often dramatically."},
{"Weak Stomach", -1, 0, 0, "\
You are more likely to throw up from food poisoning, alcohol, etc."},
{"Wool Allergy", -1, 0, 0, "\
You are badly allergic to wool, and can not wear any clothing made of the\n\
substance."},
{"HP Ignorant", -2, 0, 0, "\
You do not get to see your exact amount of HP remaining, but do have a vague\n\
idea of whether you're in good condition or not."},
{"Truth Teller", -2, 0, 0, "\
When you try to tell a lie, you blush, stammer, and get all shifty-eyed.\n\
Telling lies and otherwise bluffing will be much more difficult for you."},
{"Ugly", -1, 0, 2, "\
You're not much to look at.  NPCs who care about such things will react\n\
poorly to you."},

{"Bug - PF_MAX", 0, 0, 0, "\
This shouldn't be here!  You have the trait PF_MAX toggled.  Weird."},

/* From here down are mutations.
 * In addition to a points value, mutations have a visibility value and an
 *  ugliness value.
 * Positive visibility means that the mutation is prominent.  This will visibly
 *  identify the player as a mutant, resulting in discrimination from mutant-
 *  haters and trust with mutants/mutant-lovers.
 * Poistive ugliness means that the mutation is grotesque.  This will result in
 *  a negative reaction from NPCs, even those who are themselves mutated, unless
 *  the NPC is a mutant-lover.
 */

{"Rough Skin", 0, 2, 1, "\
Your skin is slightly rough.  This has no gameplay effect."},
{"High Night Vision", 3, 0, 0, "\
You can see incredibly well in the dark!"},
{"Full Night Vision", 5, 0, 0, "\
You can see in pitch blackness as if you were wearing night-vision goggles."},
{"Infrared Vision", 5, 0, 0, "\
Your eyes have mutated to pick up radiation in the infrared spectrum."},
{"Very Fast Healer", 5, 0, 0, "\
Your flesh regenerates slowly, and you will regain HP even when not sleeping."},
{"Regeneration", 10, 0, 0, "\
Your flesh regenerates from wounds incredibly quickly."},
{"Fangs", 2, 2, 2, "\
Your teeth have grown into two-inch-long fangs, allowing you to make an extra\n\
attack when conditions favor it."},
{"Nictitating Membrane", 1, 1, 2, "\
You have a second set of clear eyelids which lower while underwater, allowing\n\
you to see as though you were wearing goggles."},
{"Gills", 3, 5, 3, "\
You've grown a set of gills in your neck, allowing you to breathe underwater."},
{"Scales", 6, 10, 3, "\
A set of flexible green scales have grown to cover your body, acting as a\n\
natural armor."},
{"Thick Scales", 6, 10, 4, "\
A set of heavy green scales have grown to cover your body, acting as a\n\
natural armor.  It is very difficult to penetrate, but also limits your\n\
flexibility, resulting in a -2 penalty to Dexterity."},
{"Sleek Scales", 6, 10, 4, "\
A set of very flexible and slick scales have grown to cover your body.  These\n\
act as a weak set of armor, improve your ability to swim, and make you\n\
difficult to grab."},
{"Light Bones", 2, 0, 0, "\
Your bones are very light.  This enables you to run and attack 10%% faster,\n\
but also reduces your carrying weight by 20%% and makes bashing attacks hurt\n\
a little more."},
{"Feathers", 2, 10, 3, "\
Iridescent feathers have grown to cover your entire body, providing a\n\
marginal protection against attacks and minor protection from cold. They\n\
also provide a natural waterproofing."},
{"Lightly Furred", 1, 6, 2, "\
Light fur has grown to coveryour entire body, providing slight protection\n\
from cold."},
{"Furry", 2, 10, 3, "\
Thick black fur has grown to cover your entire body, providing a marginal\n\
protection against attacks, and considerable protection from cold."},
{"Chitinous Skin", 2, 3, 2, "\
Your epidermis has turned into a thin, flexible layer of chitin.  It provides\n\
minor protection from cutting wounds."},
{"Chitinous Armor", 2, 6, 3, "\
You've grown a chitin exoskeleton, much like that of an insect.  It provides\n\
considerable physical protection, but reduces your dexterity by 1."},
{"Chitinous Plate", 2, 8, 5, "\
You've grown a chitin exoskeleton made of thick, stiff plates, like that of\n\
a beetle.  It provides excellent physical protection, but reduces your\n\
dexterity by 1 and encumbers all body parts but your eyes and mouth."},
{"Spines", 1, 0, 0, "\
Your skin is covered with fine spines.  Whenever an unarmed opponent strikes\n\
a part of your body that is not covered by clothing, they will receive\n\
moderate damage."},
{"Quills", 3, 0, 0, "\
Your body is covered with large quills.  Whenever an unarmed opponent strikes\n\
a part of your body that is not covered by clothing, they will receive\n\
significant damage."},
{"Phelloderm", 3, 3, 2, "\
Your skin is light green and has a slightly woody quality to it.  This\n\
provides a weak armor, and helps you retain moisture, resulting in less\n\
thirst."},
{"Bark", 5, 10, 3, "\
Your skin is coated in a light bark, like that of a tree.  This provides\n\
resistance to bashing and cutting damage and minor protection from fire."},
{"Thorns", 6, 8, 4, "\
Your skin is covered in small, woody thorns.  Whenever an unarmed opponent\n\
strikes a part of your body that is not covered by clothing, they will\n\
receive minor damage.  Your punches may also deal extra damage."},
{"Leaves", 6, 8, 3, "\
All the hair on your body has turned to long, grass-like leaves.  Apart from\n\
being physically striking, these provide you with a minor amount of nutrition\n\
while in sunlight."},
{"Long Fingernails", 1, 1, 0, "\
Your fingernails are long and sharp.  If you aren't wearing gloves, your\n\
unarmed attacks deal a minor amount of cutting damage."},
{"Claws", 2, 3, 2, "\
You have claws on the ends of your fingers.  If you aren't wearing gloves,\n\
your unarmed attacks deal a minor amount of cutting damage."},
{"Large Talons", 2, 4, 3, "\
Your index fingers have grown into huge talons.  After a bit of practice, you\n\
find that this does not affect your dexterity, but allows for a deadly\n\
unarmed attack.  They also prevent you from wearing gloves."},
{"Radiogenic", 3, 0, 0, "\
Your system has adapted to radiation.  While irradiated, you will actually\n\
heal slowly, converting the radiation into hit points."},
{"Marloss Carrier", 4, 0, 0, "\
Ever since you ate that Marloss berry, you can't get its scent out of your\n\
nose, and you have a strong desire to eat more."},
{"Insect Pheromones", 8, 0, 0, "\
Your body produces low-level pheromones, identifying you as a friend to many\n\
species of insects.  Insects will attack you much less."},
{"Mammal Pheromones", 8, 0, 0, "\
Your body produces low-level pheromones which puts mammals at ease.  They\n\
will be less likely to attack or flee from you."},
{"Disease Immune", 6, 0, 0, "\
Your body is simply immune to diseases.  You will never catch an ambient\n\
disease."},
{"Poisonous", 8, 0, 0, "\
Your body produces a potent venom.  Any special attacks from mutatations\n\
have a chance to poison your target."},
{"Slime Hands", 4, 5, 4, "\
The skin on your hands is a mucous membrane and produces a thick, acrid\n\
slime.  Attacks using your hand will cause minor acid damage."},
{"Compound Eyes", 2, 9, 5, "\
Your eyes are compound, like those of an insect.  This increases your\n\
perception by 2 so long as you aren't wearing eyewear."},
{"Padded Feet", 1, 1, 0, "\
The bottoms of your feet are strongly padded.  You receive no movement\n\
penalty for not wearing shoes, and even receive a 10%% bonus when running\n\
barefoot."},
{"Hooves", -4, 2, 2, "\
Your feet have fused into hooves.  This allows kicking attacks to do much\n\
more damage, provides natural armor, and removes the need to wear shoes;\n\
however, you can not wear shoes of any kind."},
{"Saprovore", 4, 0, 0, "\
Your digestive system is specialized to allow you to consume decaying\n\
material.  You can eat rotten food, albeit for less nutrition than\n\
usual."},
{"Ruminant", 5, 0, 0, "\
Your digestive system is capable of digesting cellulose and other rough\n\
plant material.  You can eat underbrush by standing over it and pressing\n\
E."},
{"Horns", 2, 3, 1, "\
You have a pair of small horns on your head.  They allow you to make a weak\n\
piercing headbutt attack."},
{"Curled Horns", 1, 8, 2, "\
You have a pair of large curled horns, like those of a ram.  They allow you\n\
to make a strong bashing headbutt attack, but prevent you from wearing any\n\
headwear."},
{"Pointed Horns", 2, 8, 2, "\
You have a pair of long, pointed horns, like those of an antelope.  They\n\
allow you to make a strong piercing headbutt attack, but prevent you from\n\
wearing any headwear the is not made of fabric."},
{"Antennae", 1, 9, 4, "\
You have a pair of antennae.  They allow you to detect the presence of\n\
monsters up to a few tiles away, even if you can't see or hear them, but\n\
prevent you from wearing headwear that is not made of fabric."},
{"Road-Runner", 4, 0, 0, "\
Your legs are extremely limber and fast-moving.  You run 30%% faster on\n\
flat surfaces."},
{"Stubby Tail", 0, 1, 2, "\
You have a short, stubby tail, like a rabbit's.  It serves no purpose."},
{"Tail Fin", 1, 4, 2, "\
You have a fin-like tail.  It allows you to swim more quickly."},
{"Long Tail", 2, 6, 2, "\
You have a long, graceful tail, like that of a big cat.  It improves your\n\
balance, making your ability to dodge higher."},
{"Fluffy Tail", 2, 7, 0, "\
You have a long, fluffy-furred tail.  It greatly improves your balance,\n\
making your ability to dodge much higher."},
{"Spiked Tail", 2, 6, 3, "\
You have a long tail that ends in a vicious stinger, like that of a\n\
scorpion.  It does not improve your balance at all, but allows for a\n\
powerful piercing attack."},
{"Club Tail", 2, 7, 2, "\
You have a long tail that ends in a heavy, bony club.  It does not improve\n\
your balance at all, but alows for a powerful bashing attack."},
{"Pain Recovery", 3, 0, 0, "\
You recover from pain slightly faster than normal."},
{"Quick Pain Recovery", 5, 0, 0, "\
You recover from pain faster than normal."},
{"Very Quick Pain Reovery", 8, 0, 0, "\
You recover from pain much faster than normal."},
{"Bird Wings", 2, 4, 2, "\
You have a pair of large, feathered wings.  Your body is too heavy to be able\n\
to fly, but you can use them to slow your descent during a fall, and will not\n\
take falling damage under any circumstances."},
{"Insect Wings", 3, 4, 4, "\
You have a pair of large, translucent wings.  You buzz them as you run,\n\
enabling you to run faster."},
{"Mouth Tentacles", 1, 8, 5, "\
A set of tentacles surrounds your mouth.  They allow you to eat twice as\n\
fast."},
{"Mandibles", 2, 8, 6, "\
A set of insect-like mandibles have grown around your mouth.  They allow you\n\
to eat faster and provide a slicing unarmed attack, but prevent you from\n\
wearing mouthwear."},
{"Canine Ears", 2, 4, 1, "\
Your ears have extended into long, pointed ones, like those of a canine.\n\
They enhance your hearing, allowing you to hear at greater distances."},
{"Web Walker", 3, 0, 0, "\
Your body excretes very fine amounts of a chemcial which prevents you from\n\
sticking to webs.  Walking through webs does not affect you at all."},
{"Web Weaver", 3, 0, 0, "\
Your body produces webs.  As you move, there is a chance that you will\n\
leave webs in your wake."},
{"Whiskers", 1, 3, 1, "\
You have a set of prominent rodent-like whiskers around your mouth.  These\n\
make you more aware of vibrations in the air, and improve your ability to\n\
dodge very slightly."},
{"Strong", 1, 0, 0, "\
Your muscles are a little stronger.  Strength + 1"},
{"Very Strong", 2, 0, 0, "\
Your muscles are stronger.  Strength + 2"},
{"Extremely Strong", 4, 1, 0, "\
Your muscles are much stronger.  Strength + 4"},
{"Insanely Strong", 7, 2, 2, "\
Your muscles are noticably bulging.  Strength + 7"},
{"Dextrous", 1, 0, 0, "\
You are a little nimbler.  Dexterity + 1"},
{"Very Dextrous", 2, 0, 0, "\
You are nimbler.  Dexterity + 2"},
{"Extremely Dextrous", 3, 0, 0, "\
You are nimble and quick.  Dexterity + 4"},
{"Insanely Dextrous", 4, 0, 0, "\
You are much nimbler than before.  Dexterity + 7"},
{"Smart", 1, 0, 0, "\
You are a little smarter.  Intelligence + 1"},
{"Very Smart", 2, 0, 0, "\
You are smarter.  Intelligence + 2"},
{"Extremely Smart", 3, 1, 1, "\
You are much smarter, and your skull bulges slightly.  Intelligence + 4"},
{"Insanely Smart", 4, 3, 3, "\
Your skull bulges noticably with your impressive brain.  Intelligence + 7"},
{"Perceptive", 1, 0, 0, "\
Your senses are a little keener.  Perception + 1"},
{"Very Perceptive", 2, 0, 0, "\
Your senses are keener.  Perception + 2"},
{"Extremely Perceptive", 3, 0, 0, "\
Your senses are much keener.  Perception + 4"},
{"Insanely Perceptive", 4, 0, 0, "\
You can sense things you never imagined.  Perception + 7"},

{"Head Bumps", 0, 3, 3, "\
You have a pair of bumps on your skull."},
{"Antlers", -2, 10, 3, "\
You have a huge rack of antlers, like those of a moose.  They prevent you\n\
from hearing headwear that is not made of fabric, but provide a weak\n\
headbutt attack."},
{"Slit Nostrils", -2, 7, 4, "\
You have a flattened nose and thin slits for nostrils, giving you a lizard-\n\
like appearance.  This makes breathing slightly difficult and increases\n\
mouth encumbrance by 1."},
{"Forked Tongue", 0, 1, 3, "\
Your tongue is forked, like that of a reptile.  This has no effect."},
{"Bulging Eyes", 0, 8, 4, "\
Your eyes bulge out several inches from your skull.  This does not affect\n\
your vision in any way."},
{"Mouth Flaps", -1, 7, 6, "\
Skin tabs and odd flaps of skin surround your mouth.  They don't affect your\n\
eating, but are unpleasant to look at."},
{"Wing Stubs", 0, 2, 2, "\
You have a pair of stubby little wings projecting from your shoulderblades.\n\
They can be wiggled at will, but are useless."},
{"Bat Wings", -1, 9, 4, "\
You have a pair of large, leathery wings.  You can move them a little, but\n\
they are useless, and in fact put you off balance, reducing your ability to\n\
dodge slightly."},
{"Pale Skin", 0, 3, 1, "\
Your skin is rather pale."},
{"Spots", 0, 6, 2, "\
Your skin is covered in a pattern of red spots."},
{"Very Smelly", -4, 4, 5, "\
You smell awful.  Monsters that track scent will find you very easily, and\n\
humans will react poorly."},
{"Deformed", -2, 4, 4, "\
You're minorly deformed.  Some people will react badly to your appearance."},
{"Badly Deformed", -4, 7, 7, "\
You're hideously deformed.  Some people will have a strong negative reaction\n\
to your appearance."},
{"Grotesque", -7, 10, 10, "\
Your visage is disgusting and liable to induce vomiting.  People will not\n\
want to interact with you unless they have a very good reason to."},
{"Hollow Bones", -6, 0, 0, "\
You have Avian Bone Syndrome--your bones are nearly hollow.  Your body is\n\
very light as a result, enabling you to run and attack 20%% faster, but\n\
also frail; you can carry 40%% less, and bashing attacks injure you more."},
{"Nausea", -3, 0, 0, "\
You feel nauseous almost constantly, and are more liable to throw up from\n\
food poisoning, alcohol, etc."},
{"Vomitous", -8, 0, 0, "\
You have a major digestive disorder, which causes you to vomit frequently."},
{"Fast Metabolism", -2, 0, 0, "\
You require more food than most people."},
{"High Thirst", -3, 0, 0, "\
Your body dries out easily; you need to drink a lot more water."},
{"Weakening", -6, 0, 0, "\
You feel as though you are slowly weakening, but it's so slight a feeling\n\
that it does not affect you at all."},
{"Deterioration", -8, 0, 0, "\
Your body is very slowly wasting away."},
{"Disintegration", -10, 0, 0, "\
Your body is slowly wasting away!"},
{"Albino", -2, 0, 0, "\
Your skin lacks pigment, and is nearly transparent.  You suffer serious burns\n\
in direct sunlight."},
{"Sores", -2, 5, 6, "\
Your body is covered in painful sores.  The pain is worse when they are\n\
covered in clothing."},
{"Light Sensitive", -2, 0, 0, "\
Sunlight makes you uncomfortable.  If you are outdoors and the weather is\n\
Sunny, you suffer -1 to all stats."},
{"Very Light Sensitive", -3, 0, 0, "\
Sunlight makes you very uncomfortable.  If you are outdoors during the day,\n\
you suffer -1 to all stats; -2 if the weather is Sunny."},
{"Troglobite", -5, 0, 0, "\
Sunlight makes you extremely uncomfortable, resulting in large penalties to\n\
all stats."},
{"Webbed Hands", -1, 3, 2, "\
Your hands and feet are heavily webbed, reducing your dexterity by 1 and\n\
preventing you from wearing gloves.  However, you can swim much faster."},
{"Beak", -1, 8, 4, "\
You have a beak for a mouth.  You can occasionally use it to peck at your\n\
enemies, but it is impossible for you to wear mouthgear."},
{"Genetically Unstable", -4, 0, 0, "\
Your DNA has been damaged in a way that causes you to continually develop\n\
more mutations."},
{"Minor Radioactivity", -4, 0, 0, "\
Your body has become radioactive!  You continuously emit low levels of\n\
radiation, some of which will be absorbed by you, and some of which will\n\
contaminate the world around you."},
{"Radioactivity", -4, 0, 0, "\
Your body has become radioactive!  You continuously emit moderate levels of\n\
radiation, some of which will be absorbed by you, and some of which will\n\
contaminate the world around you."},
{"Severe Radioactivity", -4, 0, 0, "\
Your body has become radioactive!  You continuously emit heavy levels of\n\
radiation, some of which will be absorbed by you, and some of which will\n\
contaminate the world around you."},
{"Slimy", -1, 7, 6, "\
Your body is coated with a fine slime, which oozes off of you, leaving a\n\
trail."},
{"Herbivore", -3, 0, 0, "\
Your body's ability to digest meat is severely hampered.  Eating meat has a\n\
good chance of making you vomit it back up; even if you manage to keep it\n\
down, its nutritional value is greatly reduced."},
{"Carnivore", -3, 0, 0, "\
Your body's ability to digest fruits, vegetables and grains is severely\n\
hampered.  You cannot eat anything besides meat."},
{"Ponderous", -3, 0, 0, "\
Your muscles are generally slow to move.  You run 10%% slower."},
{"Very Ponderous", -5, 0, 0, "\
Your muscles are quite slow to move.  You run 20%% slower."},
{"Extremely Ponderous", -8, 0, 0, "\
Your muscles are very slow to move.  You run 30%% slower."},
{"Sunlight dependent", -5, 0, 0, "\
You feel very sluggish when not in direct sunlight.  You suffer a 5%% drop in\n\
speed when in shade, and a 10%% drop in speed when in the dark."},
{"Heat dependent", -2, 0, 0, "\
Your muscle response is dependent on ambient temperatures.  You lose 1%% of\n\
your speed for every 5 degrees below 65 F."},
{"Very Heat dependent", -3, 0, 0, "\
Your muscle response is highly dependent on ambient temperatures.  You lose\n\
1%% of your speed for every 3 degrees below 65 F."},
{"Cold Blooded", -5, 0, 0, "\
You are cold-blooded and rely on heat to keep moving.  Your lose 1%% of your\n\
speed for every 2 degrees below 65 F."},
{"Growling Voice", -1, 0, 0, "\
You have a growling, rough voice.  Persuading NPCs will be more difficult,\n\
but threatening them will be easier."},
{"Snarling Voice", -2, 0, 0, "\
You have a threatening snarl in your voice.  Persuading NPCs will be near\n\
impossible, but threatening them will be much easier."},
{"Shouter", -2, 0, 0, "\
You occasionally shout uncontrollably."},
{"Screamer", -3, 0, 0, "\
You sometimes scream uncontrollably."},
{"Howler", -5, 0, 0, "\
You frequently let out a piercing howl."},
{"Tentacle Arms", -5, 7, 4, "\
Your arms have transformed into tentacles.  Though they are flexible and\n\
increase your dexterity by 1, the lack of fingers results in a permanent\n\
hand encumbrance of 3, and prevents the wearing of gloves."},
{"4 Tentacles", -3, 8, 5, "\
Your arms have transformed into four tentacles, resulting in a bonus of 1 to\n\
dexterity, permanent hand encumbrance of 3, and preventing you from wearing\n\
gloves.  You can make up to 3 extra attacks with them."},
{"8 Tentacles", -2, 9, 6, "\
Your arms have transformed into eight tentacles, resulting in a bonus of 1 to\n\
dexterity, permanent hand encumbrance of 3, and preventing you from wearing\n\
gloves.  You can make up to 7 extra attacks with them."},
{"Shell", -6, 8, 3, "\
You have grown a thick shell over your torso, providing excellent armor.  You\n\
find you can use the empty space as 16 storage space, but cannot wear\n\
anything on your torso."},
{"Leg Tentacles", -3, 8, 4, "\
Your legs have transformed into six tentacles.  This decreases your speed on\n\
land by 20%, but makes your movement silent.  However, they also increase\n\
your swimming speed."}
};

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

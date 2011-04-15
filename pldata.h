#ifndef _PLDATA_H_
#define _PLDATA_H_

enum character_type {
 PLTYPE_CUSTOM,
 PLTYPE_RANDOM,
 PLTYPE_STUDENT,
 PLTYPE_FARMER,
 PLTYPE_MECHANIC,
 PLTYPE_CLERK,
 PLTYPE_COP,
 PLTYPE_SURVIVALIST,
 PLTYPE_PROGRAMMER,
 PLTYPE_DOCTOR,
 PLTYPE_MAX
};

const std::string pltype_name[PLTYPE_MAX] = {
"Custom Character", "Random Character", "Student", "Farmer", "Mechanic",
 "Clerk", "Police Officer", "Survivalist", "Programmer", "Doctor" };

const std::string pltype_desc[PLTYPE_MAX] = { "\
A custom character you design yourself.  A pool of points is used for\n\
statistics, traits, and skills.",
"\
A character with stats, traits, and skills chosen randomly.",
"\
Before the influx of monsters, you were a college student.  You start with\n\
high intelligence, average strength, perception, and dexterity, and a\n\
smattering of varied skills.",
"\
As a farmer, you have a naturally robust strength, while your other stats may\n\
vary.  You are slightly skilled in cutting weapons, as many farm tools are.",
"\
Reparing cars has given you a keen skill in mechanics and a decent dexterity.\n\
You are also slightly skilled in bashing weapons, like wrenches and hammers.",
"\
Spending your time behind a checkout counter has honed your skills of speech\n\
and barter, but you're not particularly good at much else.",
"\
Police training demands a good strength, dexterity and perception, with less\n\
emphasis on intelligence.  You're also trained in the use of pistols, and\n\
possibly other firearms.",
"\
You love living off the land and firing your guns--the apocalypse is a dream\n\
come true!  You start off skilled in traps, rifles, and first aid.",
"\
Hours at a computer have caused your strength and perception to decline, but\n\
your mind couldn't be better honed.  You're highly skilled with computers and\n\
possibly one or two other technical areas.",
"\
Possessed of a mind and stamina capable of carrying you through law school,\n\
you're highly skilled both with first aid and more serious medical procedure,\n\
but less so with fighting."
};

enum dis_type {
 DI_NULL,
// Temperature
 DI_COLD, DI_COLD_FACE, DI_COLD_HANDS, DI_COLD_LEGS, DI_COLD_FEET,
 DI_HOT,	// Can lead to heatstroke
 DI_HEATSTROKE, DI_FBFACE, DI_FBHANDS, DI_FBFEET,
// Fields
 DI_SMOKE, DI_ONFIRE, DI_TEARGAS,
// Monsters
 DI_BOOMERED, DI_SPORES, DI_FUNGUS, DI_SLIMED,
 DI_BLIND,
 DI_LYING_DOWN, DI_SLEEP,
 DI_POISON, DI_FOODPOISON, DI_SHAKES,
// Food & Drugs
 DI_PKILL1, DI_PKILL2, DI_PKILL3, DI_PKILL_L, DI_DRUNK, DI_CIG, DI_HIGH,
  DI_HALLU, DI_VISUALS, DI_IODINE, DI_TOOK_XANAX, DI_TOOK_PROZAC,
  DI_TOOK_FLUMED, DI_ADRENALINE, DI_ASTHMA, DI_METH,
// Traps
 DI_BEARTRAP, DI_IN_PIT,
// Other
 DI_TELEGLOW
};

enum add_type {
 ADD_NULL,
 ADD_CAFFEINE, ADD_ALCOHOL, ADD_SLEEP, ADD_PKILLER, ADD_SPEED, ADD_CIG,
 ADD_COKE
};

struct disease
{
 dis_type type;
 int duration;
 disease() { type = DI_NULL; duration = 0; }
 disease(dis_type t, int d) { type = t; duration = d;}
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
 ACT_RELOAD, ACT_READ, ACT_WAIT, ACT_CRAFT, ACT_BUTCHER,
 NUM_ACTIVITIES
};

struct player_activity
{
 activity_type type;
 int moves_left;
 int index;
 player_activity() { type = ACT_NULL; moves_left = 0; index = -1; }
 player_activity(activity_type t, int turns, int Index)
 {
  type = t;
  moves_left = turns;
  index = Index;
 }
};
 
enum pl_flag {
 PF_FLEET = 0,	// -10%% terrain movement cost
 PF_PARKOUR,	// Terrain movement cost of 3 or 4 are both 2
 PF_QUICK,	// +5%% movement points
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
 PF_SUPERTASTER,// ALWAYS warned for rotten food
 PF_ANIMALEMPATH,// Animals run away more
 PF_TERRIFYING,	// All creatures run away more
 PF_DISRESISTANT,// Less likely to succumb to low health; TODO: Implement this
 PF_ADRENALINE,	// Big bonuses when low on HP
 PF_INCONSPICUOUS,// Less spawns due to timeouts
 PF_MASOCHIST,	// Morale boost from pain
 PF_LIGHTSTEP,	// Less noise from movement
 PF_HEARTLESS,	// No morale penalty for murder &c
 PF_ANDROID,	// Start with two bionics (occasionally one)
 PF_ROBUST,	// Mutations tend to be good (usually they tend to be bad)

 PF_SPLIT,	// Null trait, splits between bad & good

 PF_MYOPIC,	// Smaller sight radius UNLESS wearing glasses
 PF_HEAVYSLEEPER, // Sleeps in, won't wake up to sounds as easily
 PF_ASTHMA,	// Occasionally needs medicine or suffers effects
 PF_BADBACK,	// Carries and moves less
 PF_ILLITERATE,	// Can not read books
 PF_BADHEARING,	// Max distance for heard sounds is halved
 PF_INSOMNIA,	// Sleep may not happen
 PF_VEGETARIAN,	// Morale penalty for eating meat
 PF_GLASSJAW,	// Head HP is 15%% lower
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

 PF_MAX,
// Below this point is mutations and other mid-game perks.
// They are NOT available during character creation.
 PF_NIGHTVISION2,
 PF_INFRARED,
 PF_REGEN,
 PF_FANGS,
 PF_MEMBRANE,
 PF_GILLS,
 PF_SCALES,
 PF_FUR,	// TODO: Warmth effects
 PF_CHITIN,
 PF_SPINES,
 PF_QUILLS,
 PF_TALONS,
 PF_RADIOGENIC,

 PF_DEFORMED,
 PF_DEFORMED2,
 PF_VOMITOUS,
 PF_HUNGER,
 PF_THIRST,
 PF_ROT,
 PF_ALBINO,
 PF_SORES,
 PF_TROGLO,
 PF_WEBBED,
 PF_BEAK,
 PF_UNSTABLE,
 PF_RADIOACTIVE,
 PF_SLIMY,
 PF_HERBIVORE,
 PF_CARNIVORE,

 PF_MAX2
};

struct trait {
 std::string name;
 int points;		// How many points it costs in character creation
 bool curable;		// Can it be cured by stem cell purifier?
 std::string description;
};

const trait traits[] = {
{"Fleet-Footed", 3, false, "\
You can run more quickly than most, resulting in a 15%% speed bonus on sure\n\
footing."},
{"Parkour Expert", 1, false, "\
You're skilled at clearing obstacles; terrain like railings or counters are\n\
as easy for you to move on as solid ground."},
{"Quick", 3, false, "\
You're just generally quick!  You get a 10%% bonus to action points."},
{"Optimist", 2, false, "\
Nothing gets you down!  Your morale will generally be higher than average."},
{"Fast Healer", 2, false, "\
You heal a little faster than most; sleeping will heal more lost HP."},
{"Light Eater", 3, false, "\
Your metabolism is a little slower, and you require less food than most."},
{"Pain Resistant", 4, false, "\
You have a high tolerance for pain."},
{"Night Vision", 1, false, "\
You possess natural night vision, and can see two squares instead of one in\n\
pitch blackness."},
{"Poison Resistant", 1, false, "\
Your system is rather tolerant of poisons and toxins, and most will affect\n\
you less."},
{"Fast Reader", 1, false, "\
You're a quick reader, and can get through books a lot faster than most."},
{"Tough", 5, false, "\
It takes a lot to bring you down!  You get a 20%% bonus to all hit points."},
{"Thick-Skinned", 2, true, "\
Your skin is tough.  Cutting damage is slightly reduced for you."},
{"Packmule", 3, false, "\
You can manage to find space for anything!  You can carry 40%% more volume."},
{"Fast Learner", 3, false, "\
Your skill comprehension is 50%% higher, allowing you to learn skills much\n\
faster than others.  Note that this only applies to real-world experience,\n\
not to skill gain from other sources like books."},
{"Deft", 2, false, "\
While you're not any better at melee combat, you are better at recovering\n\
from a miss, and will be able to attempt another strike faster."},
{"Drunken Master", 2, false, "\
The martial art technique of Zui Quan, or Drunken Fist, comes naturally to\n\
you.  While under the influence of alcohol, your melee skill will rise\n\
considerably, especially unarmed combat."},
{"Gourmand", 2, false, "\
You eat faster, and can eat and drink more, than anyone else!  You also enjoy\n\
food more; delicious food is better for your morale, and you don't mind some\n\
unsavory meals."},
{"Supertaster", 1, false, "\
You can tell if food is even a little spoiled before eating it."},
{"Animal Empathy", 1, false, "\
Peaceful animals will not run away from you, and even aggressive animals are\n\
less likely to attack.  This only applies to natural animals such as woodland\n\
creatures."},
{"Terrifying", 2, false, "\
There's something about you that creatures find frightening, and they are\n\
more likely to try to flee."},
{"Disease Resistant", 1, false, "\
It's very unlikely that you will catch ambient diseases like a cold or the\n\
flu."},
{"High Adrenaline", 3, false, "\
If you are in a very dangerous situation, you may experience a temporary rush\n\
which increases your speed and strength significantly."},
{"Inconspicuous", 2, false, "\
While sleeping or staying still, it is less likely that monsters will wander\n\
close to you."},
{"Masochist", 2, false, "\
Although you still suffer the negative effects of pain, it also brings a\n\
unique pleasure to you."},
{"Light Step", 1, false, "\
You make less noise while walking.  You're also less likely to set off traps."},
{"Heartless", 2, false, "\
You have few qualms, and no capacity for pity. Killing the helpless, the\n\
young, and your friends will not affect your morale at all."},
{"Android", 4, false, "\
At some point in the past you had a bionic upgrade installed in your body.\n\
You start the game with a power system, and one random bionic enhancement."},
{"Robust Genetics", 2, false, "\
You have a very strong genetic base.  If you mutate, the odds that the\n\
mutation will be beneficial are greatly increased."},

{"NULL", 0, " -------------------------------------------------- "},

{"Near-Sighted", -2, true, "\
Without your glasses, your seeing radius is severely reduced!  However, while\n\
wearing glasses this trait has no effect, and you are guaranteed to start\n\
with a pair."},
{"Heavy Sleeper", -1, false, "\
You're quite the heavy sleeper.  Noises are unlikely to wake you up."},
{"Asthmatic", -4, true, "\
You will occasionally need to use an inhaler, or else suffer severe physical\n\
limitations.  However, you are guaranteed to start with an inhaler."},
{"Bad Back", -3, true, "\
You simply can not carry as much as people with a similar strength could.\n\
Your maximum weight carried is reduced by 35%%."},
{"Illiterate", -5, false, "\
You never learned to read!  Books are totally off-limits to you."},
{"Poor Hearing", -2, true, "\
Your hearing is poor, and you may not hear quiet or far-off noises."},
{"Insomniac", -2, false, "\
You have a hard time falling asleep, even under the best circumstances!"},
{"Vegetarian", -3, false, "\
You have moral objections to eating meat.  You may consume it, but doing so\n\
will hurt your morale."},
{"Glass Jaw", -3, false, "\
Your head can't take much abuse.  Its maximum HP is 15%% lower than usual."},
{"Forgetful", -3, false, "\
You have a hard time remembering things.  Your skills will erode slightly\n\
faster than usual."},
{"Lightweight", -1, false, "\
Alcohol and drugs go straight to your head.  You suffer the negative effects\n\
of these for longer."},
{"Addictive Personality", -3, false, "\
It's easier for you to become addicted to substances, and harder to rid\n\
yourself of these addictions."},
{"Trigger Happy", -2, false, "\
On rare occasion, you will go full-auto when you intended to fire a single\n\
shot.  This has no effect when firing handguns or other semi-automatic\n\
firearms."},
{"Smelly", -1, false, "\
Your scent is particularly strong.  It's not offensive to humans, but animals\n\
that track your scent will do so more easily."},
{"Chemical Imbalance", -2, true, "\
You suffer from a minor chemical imbalance, whether mental or physical. Minor\n\
changes to your internal chemistry will manifest themselves on occasion,\n\
such as hunger, sleepiness, narcotic effects, etc."},
{"Schizophrenic", -5, false, "\
You will periodically suffer from delusions, ranging from minor effects to\n\
full visual hallucinations.  Some of these effects may be controlled through\n\
the use of Thorazine."},
{"Jittery", -3, false, "\
During moments of great stress or under the effects of stimulants, you may\n\
find your hands shaking uncontrollably, severely reducing your dexterity."},
{"Hoarder", -4, false, "\
You don't feel right unless you're carrying as much as you can.  You suffer\n\
morale penalties for carrying less than maximum volume (weight is ignored).\n\
Xanax can help control this anxiety."},
{"Savant", -4, false, "\
You tend to specialize in one skill and be poor at all others.  You advance\n\
at half speed in all skills except your best one. Note that combining this\n\
with Fast Learner will come out to a slower rate of learning for all skills."},
{"Mood Swings", -1, false, "\
Your morale will shift up and down at random, often dramatically."},
{"Weak Stomach", -1, false, "\
You are more likely to throw up from food poisoning, alcohol, etc."},
{"Wool Allergy", -1, false, "\
You are badly allergic to wool, and can not wear any clothing made of the\n\
substance."},
{"HP Ignorant", -2, false, "\
You do not get to see your exact amount of HP remaining, but do have a vague\n\
idea of whether you're in good condition or not."},

{"Bug - PF_MAX", 0, false, "\
This shouldn't be here!  You have the trait PF_MAX toggled.  Weird."},

{"High Night Vision", 3, true, "\
You can see incredibly well in the dark!"},
{"Infrared Vision", 3, true, "\
Your eyes have mutated to pick up radiation in the infrared spectrum."},
{"Regeneration", 8, true, "\
Your flesh regenerates from wounds incredibly quickly."},
{"Fangs", 2, true, "\
Your teeth have grown into two-inch-long fangs, allowing you to make an extra\n\
attack when conditions favor it."},
{"Nictating Membrane", 1, true, "\
You have a second set of clear eyelids which lower while underwater, allowing\n\
you to see as though you were wearing goggles."},
{"Gills", 1, true, "\
You've grown a set of gills in your neck, allowing you to breathe underwater."},
{"Scales", 4, true, "\
A set of flexible green scales have grown to cover your body, acting as a\n\
natural armor."},
{"Furry", 2, true, "\
Thick black fur has grown to cover your entire body, providing a marginal\n\
protection against attacks, and considerable protection from cold."},
{"Chitinous Armor", 2, true, "\
You've grow a chitin exoskeleton, much like that of an insect.  It provides\n\
considerable physical protection, but reduces your dexterity by 3."},
{"Spines", 1, true, "\
Your skin is covered with fine spines.  Whenever an unarmed opponent strikes\n\
a part of your body that is not covered by clothing, they will receive minor\n\
damage."},
{"Quills", 3, true, "\
Your body is covered with large quills.  Whenever an unarmed opponent strikes\n\
a part of your body that is not covered by clothing, they will receive\n\
significant damage."},
{"Large Talons", 2, true, "\
Your index fingers have grown into huge talons.  After a bit of practice, you\n\
find that this does not affect your dexterity, but allows for a deadly\n\
unarmed attack."},
{"Radiogenic", 3, true, "\
Your system has adapted to radiation.  While irradiated, you will actually\n\
heal slowly, converting the radiation into hit points."},

{"Deformed", -2, true, "\
You're minorly deformed.  Some people will react badly to your appearance."},
{"Badly Deformed", -3, true, "\
You're hideously deformed.  Some people will have a strong negative reaction\n\
to your appearance."},
{"Vomitous", -4, true, "\
You have a major digestive disorder, which causes you to vomit frequently."},
{"Fast Metabolism", -2, true, "\
You require more food than most people."},
{"High Thirst", -3, true, "\
Your body dries out easily; you need to drink a lot more water."},
{"Disintegration", -10, true, "\
Your body is slowly wasting away!"},
{"Albino", -2, true, "\
Your skin lacks pigment, and is nearly transparent.  You suffer serious burns\n\
in direct sunlight."},
{"Sores", -2, true, "\
Your body is covered in painful sores.  The pain is worse when they are\n\
covered in clothing."},
{"Light Sensitive", -3, true, "\
Sunlight makes you extremely uncomfortable, resulting in large penalties to\n\
all stats."},
{"Webbed Hands", -1, true, "\
Your hands and feet are heavily webbed, reducing your dexterity by 1 and\n\
preventing you from wearing gloves.  However, you can swim much faster."},
{"Beak", -1, true, "\
You have a beak for a mouth.  You can occasionally use it to peck at your\n\
enemies, but it is impossible for you to wear mouthgear."},
{"Genetically Unstable", -4, true, "\
Your DNA has been damaged in a way that causes you to continually develop\n\
more mutations."},
{"Radioactive", -4, true, "\
Your body has become radioactive!  You continuously emit low levels of\n\
radiation, some of which will be absorbed by you, and some of which will\n\
contaminate the world around you."},
{"Slimy", -1, true, "\
Your body is coated with a fine slime, which oozes off of you, leaving a\n\
trail."},
{"Herbivore", -3, true, "\
Your body's ability to digest meat is severely hampered.  Eating meat has a\n\
good chance of making you vomit it back up; even if you manage to keep it\n\
down, its nutritional value is greatly reduced."},
{"Carnivore", -3, true, "\
Your body's ability to digest fruits, vegetables and grains is severely\n\
hampered.  You cannot eat anything besides meat."}
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

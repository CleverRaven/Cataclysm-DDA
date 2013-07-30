#include "player.h"
#include "profession.h"
#include "bionics.h"
#include "mission.h"
#include "game.h"
#include "disease.h"
#include "addiction.h"
#include "keypress.h"
#include "moraledata.h"
#include "inventory.h"
#include "artifact.h"
#include "options.h"
#include <sstream>
#include <stdlib.h>
#include <algorithm>
#include "weather.h"
#include "item.h"
#include "material.h"
#include "translations.h"
#include "name.h"
#include "cursesdef.h"
#include "catacharset.h"

nc_color encumb_color(int level);
bool activity_is_suspendable(activity_type type);
trait traits[PF_MAX2];

std::string morale_data[NUM_MORALE_TYPES];

void game::init_morale()
{
    std::string tmp_morale_data[NUM_MORALE_TYPES] = {
    "This is a bug (moraledata.h:moraledata)",
    _("Enjoyed %i"),
    _("Enjoyed a hot meal"),
    _("Music"),
    _("Marloss Bliss"),
    _("Good Feeling"),

    _("Nicotine Craving"),
    _("Caffeine Craving"),
    _("Alcohol Craving"),
    _("Opiate Craving"),
    _("Speed Craving"),
    _("Cocaine Craving"),
    _("Crack Cocaine Craving"),

    _("Disliked %i"),
    _("Ate Human Flesh"),
    _("Ate Meat"),
    _("Wet"),
    _("Dried Off"),
    _("Cold"),
    _("Hot"),
    _("Bad Feeling"),
    _("Killed Innocent"),
    _("Killed Friend"),
    _("Guilty about Killing"),

    _("Moodswing"),
    _("Read %i"),
    _("Heard Disturbing Scream"),

    _("Masochism"),
    _("Hoarder"),
    _("Cross-Dresser"),
    _("Optimist")
    };
    for(int i=0; i<NUM_MORALE_TYPES; i++){morale_data[i]=tmp_morale_data[i];}
}

//TODO: json it, maybe. Hope this huge array doesn't cause stack issue
void game::init_traits()
{
    trait tmp_traits[] = {
{"NULL trait!", 0, 0, 0, "\
This is a bug.  Weird. (pldata.cpp:traits)"},
{_("Fleet-Footed"), 3, 0, 0, _("\
You can run more quickly than most, resulting in a 15%% speed bonus on sure \
footing.")},
{_("Parkour Expert"), 2, 0, 0, _("\
You're skilled at clearing obstacles; terrain like railings or counters are \
as easy for you to move on as solid ground.")},
{_("Quick"), 3, 0, 0, _("\
You're just generally quick!  You get a 10%% bonus to action points.")},
{_("Optimist"), 2, 0, 0, _("\
Nothing gets you down!  You savor the joys of life, ignore its hardships, and \
are generally happier than most people.")},
{_("Fast Healer"), 2, 0, 0, _("\
You heal a little faster than most; sleeping will heal more lost HP.")},
{_("Light Eater"), 3, 0, 0, _("\
Your metabolism is a little slower, and you require less food than most.")},
{_("Pain Resistant"), 2, 0, 0, _("\
You have a high tolerance for pain.")},
{_("Night Vision"), 1, 0, 0, _("\
You possess natural night vision, and can see two squares instead of one in \
pitch blackness.")},
{_("Poison Resistant"), 1, 0, 0, _("\
Your system is rather tolerant of poisons and toxins, and most will affect \
you less.")},
{_("Fast Reader"), 1, 0, 0, _("\
You're a quick reader, and can get through books a lot faster than most.")},
{_("Tough"), 3, 0, 0, _("\
It takes a lot to bring you down!  You get a 20%% bonus to all hit points.")},
{_("Thick-Skinned"), 2, 0, 0, _("\
Your skin is tough.  Cutting damage is slightly reduced for you.")},
{_("Packmule"), 3, 0, 0, _("\
You can manage to find space for anything!  You can carry 40%% more volume.")},
{_("Fast Learner"), 3, 0, 0, _("\
You have a flexible mind, allowing you to learn skills much faster than \
others.  Note that this only applies to real-world experience, not to skill \
gain from other sources like books.")},
{_("Deft"), 2, 0, 0, _("\
While you're not any better at melee combat, you are better at recovering \
from a miss, and will be able to attempt another strike faster.")},
{_("Drunken Master"), 2, 0, 0, _("\
The ancient arts of drunken brawling come naturally to you! While under the \
influence of alcohol, your melee skill will rise considerably, especially \
unarmed combat.")},
{_("Gourmand"), 2, 0, 0, _("\
You eat faster, and can eat and drink more, than anyone else!  You also enjoy \
food more; delicious food is better for your morale, and you don't mind some \
unsavory meals.")},
{_("Animal Empathy"), 1, 0, 0, _("\
Peaceful animals will not run away from you, and even aggressive animals are \
less likely to attack.  This only applies to natural animals such as woodland \
creatures.")},
{_("Terrifying"), 2, 0, 0, _("\
There's something about you that creatures find frightening, and they are \
more likely to try to flee.")},
{_("Disease Resistant"), 1, 0, 0, _("\
It's very unlikely that you will catch ambient diseases like a cold or the \
flu.")},
{_("High Adrenaline"), 3, 0, 0, _("\
If you are in a very dangerous situation, you may experience a temporary rush \
which increases your speed and strength significantly.")},
{_("Self-aware"), 1, 0, 0, _("\
You get to see your exact amount of HP remaining, instead of only having a \
vague idea of whether you're in good condition or not.")},
{_("Inconspicuous"), 2, 0, 0, _("\
While sleeping or staying still, it is less likely that monsters will wander \
close to you.")},
{_("Masochist"), 2, 0, 0, _("\
Although you still suffer the negative effects of pain, it also brings a \
unique pleasure to you.")},
{_("Cross-Dresser"), 2, 0, 0, _("\
Covering your body in clothing typical for the opposite gender makes you feel better. \
Negates any gender restrictions on professions.")},
{_("Light Step"), 1, 0, 0, _("\
You make less noise while walking.  You're also less likely to set off traps.")},
{_("Android"), 4, 0, 0, _("\
At some point in the past you had a bionic upgrade installed in your body. \
You start the game with a power system, and one random bionic enhancement.")},
{_("Robust Genetics"), 2, 0, 0, _("\
You have a very strong genetic base.  If you mutate, the odds that the \
mutation will be beneficial are greatly increased.")},
{_("Cannibal"), 3, 0, 0, _("\
For your whole life you've been forbidden from indulging in your peculiar \
tastes. Now the world's ended, and you'll be damned if anyone is going to \
tell you you can't eat people.")},
{_("Martial Arts Training"), 3, 0, 0, _("\
You have received some martial arts training at a local dojo. \
You start with your choice of karate, judo, aikido, tai chi, or taekwondo.")},
{_("Self-Defense Classes"), 3, 0, 0, _("\
You have taken some self-defense classes at a nearby gym. You start \
with your choice of Capoeira, Krav Maga, Muay Thai, Ninjutsu, or Zui Quan.")},
{_("Shaolin Adept"), 3, 0, 0, _("\
You have studied the arts of the Shaolin monks.  You start with one \
of the five animal fighting styles: Tiger, Crane, Leopard, Snake, or Dragon.")},
{_("Venom Mob Protoge"), 3, 0, 0, _("\
You are a pupil of the Venom Clan.  You start with one of the \
five deadly venoms: Centipede, Viper, Scorpion, Lizard, or Toad.")},
{_("Skilled Liar"), 2, 0, 0, _("\
You have no qualms about bending the truth, and have practically no tells. \
Telling lies and otherwise bluffing will be much easier for you.")},
{_("Pretty"), 1, 0, -2, _("\
You are a sight to behold. NPCs who care about such thing will react more \
kindly to you.")},

{"NULL", 0, 0, 0, " -------------------------------------------------- "},

{_("Near-Sighted"), -2, 0, 0, _("\
Without your glasses, your seeing radius is severely reduced!  However, while \
wearing glasses this trait has no effect, and you are guaranteed to start \
with a pair.")},
{_("Far-Sighted"), -2, 0, 0, _("\
Without reading glasses, you are unable to read anything, and take penalities \
on melee accuracy and electronics/tailoring crafting. However, you are \
guaranteed to start with a pair of reading glasses.")},
{_("Heavy Sleeper"), -1, 0, 0, _("\
You're quite the heavy sleeper.  Noises are unlikely to wake you up.")},
{_("Asthmatic"), -4, 0, 0, _("\
You will occasionally need to use an inhaler, or else suffer severe physical \
limitations.  However, you are guaranteed to start with an inhaler.")},
{_("Bad Back"), -3, 0, 0, _("\
You simply can not carry as much as people with a similar strength could. \
Your maximum weight carried is reduced by 35%%.")},
{_("Illiterate"), -5, 0, 0, _("\
You never learned to read!  Books and computers are off-limits to you.")},
{_("Poor Hearing"), -2, 0, 0, _("\
Your hearing is poor, and you may not hear quiet or far-off noises.")},
{_("Insomniac"), -2, 0, 0, _("\
You have a hard time falling asleep, even under the best circumstances!")},
{_("Meat Intolerance"), -3, 0, 0, _("\
You have problems with eating meat, it's possible for you to eat it but \
you will suffer morale penalties due to nausea.")},
{_("Glass Jaw"), -3, 0, 0, _("\
Your head can't take much abuse.  Its maximum HP is 20%% lower than usual.")},
{_("Forgetful"), -3, 0, 0, _("\
You have a hard time remembering things.  Your skills will erode slightly \
faster than usual.")},
{_("Lightweight"), -1, 0, 0, _("\
Alcohol and drugs go straight to your head.  You suffer the negative effects \
of these for longer.")},
{_("Addictive Personality"), -3, 0, 0, _("\
It's easier for you to become addicted to substances, and harder to rid \
yourself of these addictions.")},
{_("Trigger Happy"), -2, 0, 0, _("\
On rare occasion, you will go full-auto when you intended to fire a single \
shot.  This has no effect when firing handguns or other semi-automatic \
firearms.")},
{_("Smelly"), -1, 0, 0, _("\
Your scent is particularly strong.  It's not offensive to humans, but animals \
that track your scent will do so more easily.")},
{_("Chemical Imbalance"), -2, 0, 0, _("\
You suffer from a minor chemical imbalance, whether mental or physical. Minor \
changes to your internal chemistry will manifest themselves on occasion, \
such as hunger, sleepiness, narcotic effects, etc.")},
{_("Schizophrenic"), -5, 0, 0, _("\
You will periodically suffer from delusions, ranging from minor effects to \
full visual hallucinations.  Some of these effects may be controlled through \
the use of Thorazine.")},
{_("Jittery"), -3, 0, 0, _("\
During moments of great stress or under the effects of stimulants, you may \
find your hands shaking uncontrollably, severely reducing your dexterity.")},
{_("Hoarder"), -4, 0, 0, _("\
You don't feel right unless you're carrying as much as you can.  You suffer \
morale penalties for carrying less than maximum volume (weight is ignored). \
Xanax can help control this anxiety.")},
{_("Savant"), -4, 0, 0, _("\
You tend to specialize in one skill and be poor at all others.  You advance \
at half speed in all skills except your best one. Note that combining this \
with Fast Learner will come out to a slower rate of learning for all skills.")},
{_("Mood Swings"), -1, 0, 0, _("\
Your morale will shift up and down at random, often dramatically.")},
{_("Weak Stomach"), -1, 0, 0, _("\
You are more likely to throw up from food poisoning, alcohol, etc.")},
{_("Wool Allergy"), -1, 0, 0, _("\
You are badly allergic to wool, and can not wear any clothing made of the \
substance.")},
{_("Truth Teller"), -2, 0, 0, _("\
When you try to tell a lie, you blush, stammer, and get all shifty-eyed. \
Telling lies and otherwise bluffing will be much more difficult for you.")},
{_("Ugly"), -1, 0, 2, _("\
You're not much to look at.  NPCs who care about such things will react \
poorly to you.")},
{_("Hardcore"), -6, 0, 0, _("\
Your whole body can't take much abuse.  Its maximum HP is 75%% points lower \
than usual. Stacks with Glass Jaw. Not for casuals.")},

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

{_("Rough Skin"), 0, 2, 1, _("\
Your skin is slightly rough.  This has no gameplay effect.")},
{_("High Night Vision"), 3, 0, 0, _("\
You can see incredibly well in the dark!")},
{_("Full Night Vision"), 5, 0, 0, _("\
You can see in pitch blackness as if you were wearing night-vision goggles.")},
{_("Infrared Vision"), 5, 0, 0, _("\
Your eyes have mutated to pick up radiation in the infrared spectrum.")},
{_("Very Fast Healer"), 5, 0, 0, _("\
Your flesh regenerates slowly, and you will regain HP even when not sleeping.")},
{_("Regeneration"), 10, 0, 0, _("\
Your flesh regenerates from wounds incredibly quickly.")},
{_("Fangs"), 2, 2, 2, _("\
Your teeth have grown into two-inch-long fangs, allowing you to make an extra \
attack when conditions favor it.")},
{_("Nictitating Membrane"), 1, 1, 2, _("\
You have a second set of clear eyelids which lower while underwater, allowing \
you to see as though you were wearing goggles.")},
{_("Gills"), 3, 5, 3, _("\
You've grown a set of gills in your neck, allowing you to breathe underwater.")},
{_("Scales"), 6, 10, 3, _("\
A set of flexible green scales have grown to cover your body, acting as a \
natural armor.")},
{_("Thick Scales"), 6, 10, 4, _("\
A set of heavy green scales have grown to cover your body, acting as a \
natural armor.  It is very difficult to penetrate, but also limits your \
flexibility, resulting in a -2 penalty to Dexterity.")},
{_("Sleek Scales"), 6, 10, 4, _("\
A set of very flexible and slick scales have grown to cover your body.  These \
act as a weak set of armor, improve your ability to swim, and make you \
difficult to grab.")},
{_("Light Bones"), 2, 0, 0, _("\
Your bones are very light.  This enables you to run and attack 10%% faster, \
but also reduces your carrying weight by 20%% and makes bashing attacks hurt \
a little more.")},
{_("Feathers"), 2, 10, 3, _("\
Iridescent feathers have grown to cover your entire body, providing a \
marginal protection against attacks and minor protection from cold. They \
also provide a natural waterproofing.")},
{_("Lightly Furred"), 1, 6, 2, _("\
Light fur has grown to coveryour entire body, providing slight protection \
from cold.")},
{_("Furry"), 2, 10, 3, _("\
Thick black fur has grown to cover your entire body, providing a marginal \
protection against attacks, and considerable protection from cold.")},
{_("Chitinous Skin"), 2, 3, 2, _("\
Your epidermis has turned into a thin, flexible layer of chitin.  It provides \
minor protection from cutting wounds.")},
{_("Chitinous Armor"), 2, 6, 3, _("\
You've grown a chitin exoskeleton, much like that of an insect.  It provides \
considerable physical protection, but reduces your dexterity by 1.")},
{_("Chitinous Plate"), 2, 8, 5, _("\
You've grown a chitin exoskeleton made of thick, stiff plates, like that of \
a beetle.  It provides excellent physical protection, but reduces your \
dexterity by 1 and encumbers all body parts but your eyes and mouth.")},
{_("Spines"), 1, 0, 0, _("\
Your skin is covered with fine spines.  Whenever an unarmed opponent strikes \
a part of your body that is not covered by clothing, they will receive \
moderate damage.")},
{_("Quills"), 3, 0, 0, _("\
Your body is covered with large quills.  Whenever an unarmed opponent strikes \
a part of your body that is not covered by clothing, they will receive \
significant damage.")},
{_("Phelloderm"), 3, 3, 2, _("\
Your skin is light green and has a slightly woody quality to it.  This \
provides a weak armor, and helps you retain moisture, resulting in less \
thirst.")},
{_("Bark"), 5, 10, 3, _("\
Your skin is coated in a light bark, like that of a tree.  This provides \
resistance to bashing and cutting damage and minor protection from fire.")},
{_("Thorns"), 6, 8, 4, _("\
Your skin is covered in small, woody thorns.  Whenever an unarmed opponent \
strikes a part of your body that is not covered by clothing, they will \
receive minor damage.  Your punches may also deal extra damage.")},
{_("Leaves"), 6, 8, 3, _("\
All the hair on your body has turned to long, grass-like leaves.  Apart from \
being physically striking, these provide you with a minor amount of nutrition \
while in sunlight.")},
{_("Long Fingernails"), 1, 1, 0, _("\
Your fingernails are long and sharp.  If you aren't wearing gloves, your \
unarmed attacks deal a minor amount of cutting damage.")},
{_("Claws"), 2, 3, 2, _("\
You have claws on the ends of your fingers.  If you aren't wearing gloves, \
your unarmed attacks deal a minor amount of cutting damage.")},
{_("Large Talons"), 2, 4, 3, _("\
Your index fingers have grown into huge talons.  After a bit of practice, you \
find that this does not affect your dexterity, but allows for a deadly \
unarmed attack.  They also prevent you from wearing gloves.")},
{_("Radiogenic"), 3, 0, 0, _("\
Your system has adapted to radiation.  While irradiated, you will actually \
heal slowly, converting the radiation into hit points.")},
{_("Marloss Carrier"), 4, 0, 0, _("\
Ever since you ate that Marloss berry, you can't get its scent out of your \
nose, and you have a strong desire to eat more.")},
{_("Insect Pheromones"), 8, 0, 0, _("\
Your body produces low-level pheromones, identifying you as a friend to many \
species of insects.  Insects will attack you much less.")},
{_("Mammal Pheromones"), 8, 0, 0, _("\
Your body produces low-level pheromones which puts mammals at ease.  They \
will be less likely to attack or flee from you.")},
{_("Disease Immune"), 6, 0, 0, _("\
Your body is simply immune to diseases.  You will never catch an ambient \
disease.")},
{_("Poisonous"), 8, 0, 0, _("\
Your body produces a potent venom.  Any special attacks from mutatations \
have a chance to poison your target.")},
{_("Slime Hands"), 4, 5, 4, _("\
The skin on your hands is a mucous membrane and produces a thick, acrid \
slime.  Attacks using your hand will cause minor acid damage.")},
{_("Compound Eyes"), 2, 9, 5, _("\
Your eyes are compound, like those of an insect.  This increases your \
perception by 2 so long as you aren't wearing eyewear.")},
{_("Padded Feet"), 1, 1, 0, _("\
The bottoms of your feet are strongly padded.  You receive no movement \
penalty for not wearing shoes, and even receive a 10%% bonus when running \
barefoot.")},
{_("Hooves"), -4, 2, 2, _("\
Your feet have fused into hooves.  This allows kicking attacks to do much \
more damage, provides natural armor, and removes the need to wear shoes; \
however, you can not wear shoes of any kind.")},
{_("Saprovore"), 4, 0, 0, _("\
Your digestive system is specialized to allow you to consume decaying \
material.  You can eat rotten food, albeit for less nutrition than \
usual.")},
{_("Ruminant"), 5, 0, 0, _("\
Your digestive system is capable of digesting cellulose and other rough \
plant material.  You can eat underbrush by standing over it and pressing \
E.")},
{_("Horns"), 2, 3, 1, _("\
You have a pair of small horns on your head.  They allow you to make a weak \
piercing headbutt attack.")},
{_("Curled Horns"), 1, 8, 2, _("\
You have a pair of large curled horns, like those of a ram.  They allow you \
to make a strong bashing headbutt attack, but prevent you from wearing any \
headwear.")},
{_("Pointed Horns"), 2, 8, 2, _("\
You have a pair of long, pointed horns, like those of an antelope.  They \
allow you to make a strong piercing headbutt attack, but prevent you from \
wearing any headwear the is not made of fabric.")},
{_("Antennae"), 1, 9, 4, _("\
You have a pair of antennae.  They allow you to detect the presence of \
monsters up to a few tiles away, even if you can't see or hear them, but \
prevent you from wearing headwear that is not made of fabric.")},
{_("Road-Runner"), 4, 0, 0, _("\
Your legs are extremely limber and fast-moving.  You run 30%% faster on \
flat surfaces.")},
{_("Stubby Tail"), 0, 1, 2, _("\
You have a short, stubby tail, like a rabbit's.  It serves no purpose.")},
{_("Tail Fin"), 1, 4, 2, _("\
You have a fin-like tail.  It allows you to swim more quickly.")},
{_("Long Tail"), 2, 6, 2, _("\
You have a long, graceful tail, like that of a big cat.  It improves your \
balance, making your ability to dodge higher.")},
{_("Fluffy Tail"), 2, 7, 0, _("\
You have a long, fluffy-furred tail.  It greatly improves your balance, \
making your ability to dodge much higher.")},
{_("Spiked Tail"), 2, 6, 3, _("\
You have a long tail that ends in a vicious stinger, like that of a \
scorpion.  It does not improve your balance at all, but allows for a \
powerful piercing attack.")},
{_("Club Tail"), 2, 7, 2, _("\
You have a long tail that ends in a heavy, bony club.  It does not improve \
your balance at all, but alows for a powerful bashing attack.")},
{_("Pain Recovery"), 3, 0, 0, _("\
You recover from pain slightly faster than normal.")},
{_("Quick Pain Recovery"), 5, 0, 0, _("\
You recover from pain faster than normal.")},
{_("Very Quick Pain Reovery"), 8, 0, 0, _("\
You recover from pain much faster than normal.")},
{_("Bird Wings"), 2, 4, 2, _("\
You have a pair of large, feathered wings.  Your body is too heavy to be able \
to fly, but you can use them to slow your descent during a fall, and will not \
take falling damage under any circumstances.")},
{_("Insect Wings"), 3, 4, 4, _("\
You have a pair of large, translucent wings.  You buzz them as you run, \
enabling you to run faster.")},
{_("Mouth Tentacles"), 1, 8, 5, _("\
A set of tentacles surrounds your mouth.  They allow you to eat twice as \
fast.")},
{_("Mandibles"), 2, 8, 6, _("\
A set of insect-like mandibles have grown around your mouth.  They allow you \
to eat faster and provide a slicing unarmed attack, but prevent you from \
wearing mouthwear.")},
{_("Canine Ears"), 2, 4, 1, _("\
Your ears have extended into long, pointed ones, like those of a canine. \
They enhance your hearing, allowing you to hear at greater distances.")},
{_("Web Walker"), 3, 0, 0, _("\
Your body excretes very fine amounts of a chemcial which prevents you from \
sticking to webs.  Walking through webs does not affect you at all.")},
{_("Web Weaver"), 3, 0, 0, _("\
Your body produces webs.  As you move, there is a chance that you will \
leave webs in your wake.")},
{_("Whiskers"), 1, 3, 1, _("\
You have a set of prominent rodent-like whiskers around your mouth.  These \
make you more aware of vibrations in the air, and improve your ability to \
dodge very slightly.")},
{_("Strong"), 1, 0, 0, _("\
Your muscles are a little stronger.  Strength + 1")},
{_("Very Strong"), 2, 0, 0, _("\
Your muscles are stronger.  Strength + 2")},
{_("Extremely Strong"), 4, 1, 0, _("\
Your muscles are much stronger.  Strength + 4")},
{_("Insanely Strong"), 7, 2, 2, _("\
Your muscles are noticably bulging.  Strength + 7")},
{_("Dextrous"), 1, 0, 0, _("\
You are a little nimbler.  Dexterity + 1")},
{_("Very Dextrous"), 2, 0, 0, _("\
You are nimbler.  Dexterity + 2")},
{_("Extremely Dextrous"), 3, 0, 0, _("\
You are nimble and quick.  Dexterity + 4")},
{_("Insanely Dextrous"), 4, 0, 0, _("\
You are much nimbler than before.  Dexterity + 7")},
{_("Smart"), 1, 0, 0, _("\
You are a little smarter.  Intelligence + 1")},
{_("Very Smart"), 2, 0, 0, _("\
You are smarter.  Intelligence + 2")},
{_("Extremely Smart"), 3, 1, 1, _("\
You are much smarter, and your skull bulges slightly.  Intelligence + 4")},
{_("Insanely Smart"), 4, 3, 3, _("\
Your skull bulges noticably with your impressive brain.  Intelligence + 7")},
{_("Perceptive"), 1, 0, 0, _("\
Your senses are a little keener.  Perception + 1")},
{_("Very Perceptive"), 2, 0, 0, _("\
Your senses are keener.  Perception + 2")},
{_("Extremely Perceptive"), 3, 0, 0, _("\
Your senses are much keener.  Perception + 4")},
{_("Insanely Perceptive"), 4, 0, 0, _("\
You can sense things you never imagined.  Perception + 7")},

{_("Head Bumps"), 0, 3, 3, _("\
You have a pair of bumps on your skull.")},
{_("Antlers"), -2, 10, 3, _("\
You have a huge rack of antlers, like those of a moose.  They prevent you \
from hearing headwear that is not made of fabric, but provide a weak \
headbutt attack.")},
{_("Slit Nostrils"), -2, 7, 4, _("\
You have a flattened nose and thin slits for nostrils, giving you a lizard- \
like appearance.  This makes breathing slightly difficult and increases \
mouth encumbrance by 1.")},
{_("Forked Tongue"), 0, 1, 3, _("\
Your tongue is forked, like that of a reptile.  This has no effect.")},
{_("Bulging Eyes"), 0, 8, 4, _("\
Your eyes bulge out several inches from your skull.  This does not affect \
your vision in any way.")},
{_("Mouth Flaps"), -1, 7, 6, _("\
Skin tabs and odd flaps of skin surround your mouth.  They don't affect your \
eating, but are unpleasant to look at.")},
{_("Wing Stubs"), 0, 2, 2, _("\
You have a pair of stubby little wings projecting from your shoulderblades. \
They can be wiggled at will, but are useless.")},
{_("Bat Wings"), -1, 9, 4, _("\
You have a pair of large, leathery wings.  You can move them a little, but \
they are useless, and in fact put you off balance, reducing your ability to \
dodge slightly.")},
{_("Pale Skin"), 0, 3, 1, _("\
Your skin is rather pale.")},
{_("Spots"), 0, 6, 2, _("\
Your skin is covered in a pattern of red spots.")},
{_("Very Smelly"), -4, 4, 5, _("\
You smell awful.  Monsters that track scent will find you very easily, and \
humans will react poorly.")},
{_("Deformed"), -2, 4, 4, _("\
You're minorly deformed.  Some people will react badly to your appearance.")},
{_("Badly Deformed"), -4, 7, 7, _("\
You're hideously deformed.  Some people will have a strong negative reaction \
to your appearance.")},
{_("Grotesque"), -7, 10, 10, _("\
Your visage is disgusting and liable to induce vomiting.  People will not \
want to interact with you unless they have a very good reason to.")},
{_("Beautiful"), 2, -4, -4, _("\
You're a real head-turner. Some people will react well to your appearance, \
and most people have an easier time trusting you.")},
{_("Very Beautiful"), 4, -7, -7, _("\
You are a vision of beauty. Some people will react very well to your looks, \
and most people will trust you immediately.")},
{_("Glorious"), 7, -10, -10, _("\
You are inredibly beautiful. People cannot help themselves for your charms, \
and will do whatever they can to please you.")},
{_("Hollow Bones"), -6, 0, 0, _("\
You have Avian Bone Syndrome--your bones are nearly hollow.  Your body is \
very light as a result, enabling you to run and attack 20%% faster, but \
also frail; you can carry 40%% less, and bashing attacks injure you more.")},
{_("Nausea"), -3, 0, 0, _("\
You feel nauseous almost constantly, and are more liable to throw up from \
food poisoning, alcohol, etc.")},
{_("Vomitous"), -8, 0, 0, _("\
You have a major digestive disorder, which causes you to vomit frequently.")},
{_("Fast Metabolism"), -2, 0, 0, _("\
You require more food than most people.")},
{_("High Thirst"), -3, 0, 0, _("\
Your body dries out easily; you need to drink a lot more water.")},
{_("Weakening"), -6, 0, 0, _("\
You feel as though you are slowly weakening, but it's so slight a feeling \
that it does not affect you at all.")},
{_("Deterioration"), -8, 0, 0, _("\
Your body is very slowly wasting away.")},
{_("Disintegration"), -10, 0, 0, _("\
Your body is slowly wasting away!")},
{_("Albino"), -2, 0, 0, _("\
Your skin lacks pigment, and is nearly transparent.  You suffer serious burns \
in direct sunlight.")},
{_("Sores"), -2, 5, 6, _("\
Your body is covered in painful sores.  The pain is worse when they are \
covered in clothing.")},
{_("Light Sensitive"), -2, 0, 0, _("\
Sunlight makes you uncomfortable.  If you are outdoors and the weather is \
Sunny, you suffer -1 to all stats.")},
{_("Very Light Sensitive"), -3, 0, 0, _("\
Sunlight makes you very uncomfortable.  If you are outdoors during the day, \
you suffer -1 to all stats; -2 if the weather is Sunny.")},
{_("Troglobite"), -5, 0, 0, _("\
Sunlight makes you extremely uncomfortable, resulting in large penalties to \
all stats.")},
{_("Webbed Hands"), -1, 3, 2, _("\
Your hands and feet are heavily webbed, reducing your dexterity by 1 and \
preventing you from wearing gloves.  However, you can swim much faster.")},
{_("Beak"), -1, 8, 4, _("\
You have a beak for a mouth.  You can occasionally use it to peck at your \
enemies, but it is impossible for you to wear mouthgear.")},
{_("Genetically Unstable"), -4, 0, 0, _("\
Your DNA has been damaged in a way that causes you to continually develop \
more mutations.")},
{_("Minor Radioactivity"), -4, 0, 0, _("\
Your body has become radioactive!  You continuously emit low levels of \
radiation, some of which will be absorbed by you, and some of which will \
contaminate the world around you.")},
{_("Radioactivity"), -4, 0, 0, _("\
Your body has become radioactive!  You continuously emit moderate levels of \
radiation, some of which will be absorbed by you, and some of which will \
contaminate the world around you.")},
{_("Severe Radioactivity"), -4, 0, 0, _("\
Your body has become radioactive!  You continuously emit heavy levels of \
radiation, some of which will be absorbed by you, and some of which will \
contaminate the world around you.")},
{_("Slimy"), -1, 7, 6, _("\
Your body is coated with a fine slime, which oozes off of you, leaving a \
trail.")},
{_("Herbivore"), -3, 0, 0, _("\
Your body's ability to digest meat is severely hampered.  Eating meat has a \
good chance of making you vomit it back up; even if you manage to keep it \
down, its nutritional value is greatly reduced.")},
{_("Carnivore"), -3, 0, 0, _("\
Your body's ability to digest fruits, vegetables and grains is severely \
hampered.  You cannot eat anything besides meat.")},
{_("Ponderous"), -3, 0, 0, _("\
Your muscles are generally slow to move.  You run 10%% slower.")},
{_("Very Ponderous"), -5, 0, 0, _("\
Your muscles are quite slow to move.  You run 20%% slower.")},
{_("Extremely Ponderous"), -8, 0, 0, _("\
Your muscles are very slow to move.  You run 30%% slower.")},
{_("Sunlight dependent"), -5, 0, 0, _("\
You feel very sluggish when not in direct sunlight.  You suffer a 5%% drop in \
speed when in shade, and a 10%% drop in speed when in the dark.")},
{_("Heat dependent"), -2, 0, 0, _("\
Your muscle response is dependent on ambient temperatures.  You lose 1%% of \
your speed for every 5 degrees below 65 F.")},
{_("Very Heat dependent"), -3, 0, 0, _("\
Your muscle response is highly dependent on ambient temperatures.  You lose \
1%% of your speed for every 3 degrees below 65 F.")},
{_("Cold Blooded"), -5, 0, 0, _("\
You are cold-blooded and rely on heat to keep moving.  Your lose 1%% of your \
speed for every 2 degrees below 65 F.")},
{_("Growling Voice"), -1, 0, 0, _("\
You have a growling, rough voice.  Persuading NPCs will be more difficult, \
but threatening them will be easier.")},
{_("Snarling Voice"), -2, 0, 0, _("\
You have a threatening snarl in your voice.  Persuading NPCs will be near \
impossible, but threatening them will be much easier.")},
{_("Shouter"), -2, 0, 0, _("\
You occasionally shout uncontrollably.")},
{_("Screamer"), -3, 0, 0, _("\
You sometimes scream uncontrollably.")},
{_("Howler"), -5, 0, 0, _("\
You frequently let out a piercing howl.")},
{_("Tentacle Arms"), -5, 7, 4, _("\
Your arms have transformed into tentacles.  Though they are flexible and \
increase your dexterity by 1, the lack of fingers results in a permanent \
hand encumbrance of 3, and prevents the wearing of gloves.")},
{_("4 Tentacles"), -3, 8, 5, _("\
Your arms have transformed into four tentacles, resulting in a bonus of 1 to \
dexterity, permanent hand encumbrance of 3, and preventing you from wearing \
gloves.  You can make up to 3 extra attacks with them.")},
{_("8 Tentacles"), -2, 9, 6, _("\
Your arms have transformed into eight tentacles, resulting in a bonus of 1 to \
dexterity, permanent hand encumbrance of 3, and preventing you from wearing \
gloves.  You can make up to 7 extra attacks with them.")},
{_("Shell"), -6, 8, 3, _("\
You have grown a thick shell over your torso, providing excellent armor.  You \
find you can use the empty space as 16 storage space, but cannot wear \
anything on your torso.")},
{_("Leg Tentacles"), -3, 8, 4, _("\
Your legs have transformed into six tentacles.  This decreases your speed on \
land by 20%, but makes your movement silent.  However, they also increase \
your swimming speed.")}
    };

    for(int i=0; i<PF_MAX2; i++) {
        traits[i] = tmp_traits[i];
    }
}

player::player()
{
 id = 0; // Player is 0. NPCs are different.
 view_offset_x = 0;
 view_offset_y = 0;
 str_cur = 8;
 str_max = 8;
 dex_cur = 8;
 dex_max = 8;
 int_cur = 8;
 int_max = 8;
 per_cur = 8;
 per_max = 8;
 underwater = false;
 dodges_left = 1;
 blocks_left = 1;
 power_level = 0;
 max_power_level = 0;
 hunger = 0;
 thirst = 0;
 fatigue = 0;
 stim = 0;
 pain = 0;
 pkill = 0;
 radiation = 0;
 cash = 0;
 recoil = 0;
 driving_recoil = 0;
 scent = 500;
 health = 0;
 name = "";
 male = true;
 prof = profession::has_initialized() ? profession::generic() : NULL; //workaround for a potential structural limitation, see player::create
 moves = 100;
 movecounter = 0;
 oxygen = 0;
 next_climate_control_check=0;
 last_climate_control_ret=false;
 active_mission = -1;
 in_vehicle = false;
 controlling_vehicle = false;
 style_selected = "null";
 focus_pool = 100;
 last_item = itype_id("null");
 for (int i = 0; i < PF_MAX2; i++)
  my_traits[i] = false;
 for (int i = 0; i < PF_MAX2; i++)
  my_mutations[i] = false;

 mutation_category_level[0] = 5; // Weigh us towards no category for a bit
 for (int i = 1; i < NUM_MUTATION_CATEGORIES; i++)
  mutation_category_level[i] = 0;

 for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin();
      aSkill != Skill::skills.end(); ++aSkill) {
   skillLevel(*aSkill).level(0);
 }

 for (int i = 0; i < num_bp; i++) {
  temp_cur[i] = BODYTEMP_NORM;
  frostbite_timer[i] = 0;
  temp_conv[i] = BODYTEMP_NORM;
 }
 nv_cached = false;
 volume = 0;
}

player::player(const player &rhs)
{
 *this = rhs;
}

player::~player()
{
}

player& player::operator= (const player & rhs)
{
 id = rhs.id;
 posx = rhs.posx;
 posy = rhs.posy;
 view_offset_x = rhs.view_offset_x;
 view_offset_y = rhs.view_offset_y;

 in_vehicle = rhs.in_vehicle;
 controlling_vehicle = rhs.controlling_vehicle;
 activity = rhs.activity;
 backlog = rhs.backlog;

 active_missions = rhs.active_missions;
 completed_missions = rhs.completed_missions;
 failed_missions = rhs.failed_missions;
 active_mission = rhs.active_mission;

 name = rhs.name;
 male = rhs.male;
 prof = rhs.prof;

 for (int i = 0; i < PF_MAX2; i++)
  my_traits[i] = rhs.my_traits[i];

 for (int i = 0; i < PF_MAX2; i++)
  my_mutations[i] = rhs.my_mutations[i];

 for (int i = 0; i < NUM_MUTATION_CATEGORIES; i++)
  mutation_category_level[i] = rhs.mutation_category_level[i];

 my_bionics = rhs.my_bionics;

 str_cur = rhs.str_cur;
 dex_cur = rhs.dex_cur;
 int_cur = rhs.int_cur;
 per_cur = rhs.per_cur;

 str_max = rhs.str_max;
 dex_max = rhs.dex_max;
 int_max = rhs.int_max;
 per_max = rhs.per_max;

 power_level = rhs.power_level;
 max_power_level = rhs.max_power_level;

 hunger = rhs.hunger;
 thirst = rhs.thirst;
 fatigue = rhs.fatigue;
 health = rhs.health;

 underwater = rhs.underwater;
 oxygen = rhs.oxygen;
 next_climate_control_check=rhs.next_climate_control_check;
 last_climate_control_ret=rhs.last_climate_control_ret;

 recoil = rhs.recoil;
 driving_recoil = rhs.driving_recoil;
 scent = rhs.scent;
 dodges_left = rhs.dodges_left;
 blocks_left = rhs.blocks_left;

 stim = rhs.stim;
 pain = rhs.pain;
 pkill = rhs.pkill;
 radiation = rhs.radiation;

 cash = rhs.cash;
 moves = rhs.moves;
 movecounter = rhs.movecounter;

 for (int i = 0; i < num_hp_parts; i++)
  hp_cur[i] = rhs.hp_cur[i];

 for (int i = 0; i < num_hp_parts; i++)
  hp_max[i] = rhs.hp_max[i];

 for (int i = 0; i < num_bp; i++)
  temp_cur[i] = rhs.temp_cur[i];

 for (int i = 0 ; i < num_bp; i++)
  temp_conv[i] = rhs.temp_conv[i];

 for (int i = 0; i < num_bp; i++)
  frostbite_timer[i] = rhs.frostbite_timer[i];

 morale = rhs.morale;
 focus_pool = rhs.focus_pool;

 _skills = rhs._skills;

 learned_recipes = rhs.learned_recipes;

 inv.clear();
 for (int i = 0; i < rhs.inv.size(); i++)
  inv.add_stack(rhs.inv.const_stack(i));

 last_item = rhs.last_item;
 worn = rhs.worn;
 styles = rhs.styles;
 style_selected = rhs.style_selected;
 weapon = rhs.weapon;

 ret_null = rhs.ret_null;

 illness = rhs.illness;
 addictions = rhs.addictions;

 nv_cached = false;

 return (*this);
}

void player::normalize(game *g)
{
 ret_null = item(g->itypes["null"], 0);
 weapon   = item(g->itypes["null"], 0);
 style_selected = "null";
 for (int i = 0; i < num_hp_parts; i++) {
  hp_max[i] = 60 + str_max * 3;
  if (has_trait(PF_TOUGH))
   hp_max[i] = int(hp_max[i] * 1.2);
  hp_cur[i] = hp_max[i];
 }
 for (int i = 0 ; i < num_bp; i++)
  temp_conv[i] = BODYTEMP_NORM;
}

void player::pick_name() {
  name = Name::generate(male);
}

void player::reset(game *g)
{
// Reset our stats to normal levels
// Any persistent buffs/debuffs will take place in disease.h,
// player::suffer(), etc.
 str_cur = str_max;
 dex_cur = dex_max;
 int_cur = int_max;
 per_cur = per_max;
// We can dodge again!
 dodges_left = 1;
 blocks_left = 1;
// Didn't just pick something up
 last_item = itype_id("null");
// Bionic buffs
 if (has_active_bionic("bio_hydraulics"))
  str_cur += 20;
 if (has_bionic("bio_eye_enhancer"))
  per_cur += 2;
if (has_active_bionic("bio_metabolics") && power_level < max_power_level &&
     hunger < 100 && (int(g->turn) % 5 == 0)) {
  hunger += 2;
  power_level++;
}

// Trait / mutation buffs
 if (has_trait(PF_THICK_SCALES))
  dex_cur -= 2;
 if (has_trait(PF_CHITIN2) || has_trait(PF_CHITIN3))
  dex_cur--;
 if (has_trait(PF_COMPOUND_EYES) && !wearing_something_on(bp_eyes))
  per_cur++;
 if (has_trait(PF_ARM_TENTACLES) || has_trait(PF_ARM_TENTACLES_4) ||
     has_trait(PF_ARM_TENTACLES_8))
  dex_cur++;
// Pain
 if (pain > pkill) {
  str_cur  -=     int((pain - pkill) / 15);
  dex_cur  -=     int((pain - pkill) / 15);
  per_cur  -=     int((pain - pkill) / 20);
  int_cur  -= 1 + int((pain - pkill) / 25);
 }
// Morale
 if (abs(morale_level()) >= 100) {
  str_cur  += int(morale_level() / 180);
  dex_cur  += int(morale_level() / 200);
  per_cur  += int(morale_level() / 125);
  int_cur  += int(morale_level() / 100);
 }
// Radiation
 if (radiation > 0) {
  str_cur  -= int(radiation / 80);
  dex_cur  -= int(radiation / 110);
  per_cur  -= int(radiation / 100);
  int_cur  -= int(radiation / 120);
 }
// Stimulants
 dex_cur += int(stim / 10);
 per_cur += int(stim /  7);
 int_cur += int(stim /  6);
 if (stim >= 30) {
  dex_cur -= int(abs(stim - 15) /  8);
  per_cur -= int(abs(stim - 15) / 12);
  int_cur -= int(abs(stim - 15) / 14);
 }

// Set our scent towards the norm
 int norm_scent = 500;
 if (has_trait(PF_SMELLY))
  norm_scent = 800;
 if (has_trait(PF_SMELLY2))
  norm_scent = 1200;

 // Scent increases fast at first, and slows down as it approaches normal levels.
 // Estimate it will take about norm_scent * 2 turns to go from 0 - norm_scent / 2
 // Without smelly trait this is about 1.5 hrs. Slows down significantly after that.
 if (scent < rng(0, norm_scent))
   scent++;

 // Unusually high scent decreases steadily until it reaches normal levels.
 if (scent > norm_scent)
  scent--;

// Give us our movement points for the turn.
 moves += current_speed(g);

// Floor for our stats.  No stat changes should occur after this!
 if (dex_cur < 0)
  dex_cur = 0;
 if (str_cur < 0)
  str_cur = 0;
 if (per_cur < 0)
  per_cur = 0;
 if (int_cur < 0)
  int_cur = 0;

 if (int(g->turn) % 10 == 0) {
  update_mental_focus();
 }

 nv_cached = false;
}

void player::action_taken()
{
    nv_cached = false;
}

void player::update_morale()
{
    // Decay existing morale entries.
    for (int i = 0; i < morale.size(); i++)
    {
        // Age the morale entry by one turn.
        morale[i].age += 1;

        // If it's past its expiration date, remove it.
        if (morale[i].age >= morale[i].duration)
        {
            morale.erase(morale.begin() + i);
            i--;

            // Future-proofing.
            continue;
        }

        // We don't actually store the effective strength; it gets calculated when we
        // need it.
    }

    // We reapply persistent morale effects after every decay step, to keep them fresh.
    apply_persistent_morale();
}

void player::apply_persistent_morale()
{
    // Hoarders get a morale penalty if they're not carrying a full inventory.
    if (has_trait(PF_HOARDER))
    {
        int pen = int((volume_capacity()-volume_carried()) / 2);
        if (pen > 70)
        {
            pen = 70;
        }
        if (pen <= 0)
        {
            pen = 0;
        }
        if (has_disease("took_xanax"))
        {
            pen = int(pen / 7);
        }
        else if (has_disease("took_prozac"))
        {
            pen = int(pen / 2);
        }
        add_morale(MORALE_PERM_HOARDER, -pen, -pen, 5, 5, true);
    }

    // Cross-dressers get a morale bonus for each body part covered in an
    // item of the opposite gender(MALE_TYPICAL/FEMALE_TYPICAL item flags).
    if (has_trait(PF_CROSSDRESSER))
    {
        int bonus = 0;
        std::string required_flag = male ? "FEMALE_TYPICAL" : "MALE_TYPICAL";

        unsigned char covered = 0; // body parts covered by stuff with opposite gender flags
        for(int i=0; i<worn.size(); i++) {
            if(worn[i].has_flag(required_flag)) {
                it_armor* item_type = (it_armor*) worn[i].type;
                covered |= item_type->covers;
            }
        }
        if(covered & mfb(bp_torso)) {
            bonus += 6;
        }
        if(covered & mfb(bp_legs)) {
            bonus += 4;
        }
        if(covered & mfb(bp_feet)) {
            bonus += 2;
        }
        if(covered & mfb(bp_hands)) {
            bonus += 2;
        }
        if(covered & mfb(bp_head)) {
            bonus += 3;
        }
        
        if(bonus) {
            add_morale(MORALE_PERM_CROSSDRESSER, bonus, bonus, 5, 5, true);
        }
    }

    // Masochists get a morale bonus from pain.
    if (has_trait(PF_MASOCHIST))
    {
        int bonus = pain / 2.5;
        if (bonus > 25)
        {
            bonus = 25;
        }
        if (has_disease("took_prozac"))
        {
            bonus = int(bonus / 3);
        }
        if (bonus != 0)
        {
            add_morale(MORALE_PERM_MASOCHIST, bonus, bonus, 5, 5, true);
        }
    }

    // Optimist gives a base +4 to morale.
    // The +25% boost from optimist also applies here, for a net of +5.
    if (has_trait(PF_OPTIMISTIC))
    {
        add_morale(MORALE_PERM_OPTIMIST, 4, 4, 5, 5, true);
    }
}

void player::update_mental_focus()
{
    int focus_gain_rate = calc_focus_equilibrium() - focus_pool;

    // handle negative gain rates in a symmetric manner
    int base_change = 1;
    if (focus_gain_rate < 0)
    {
        base_change = -1;
        focus_gain_rate = -focus_gain_rate;
    }

    // for every 100 points, we have a flat gain of 1 focus.
    // for every n points left over, we have an n% chance of 1 focus
    int gain = focus_gain_rate / 100;
    if (rng(1, 100) <= (focus_gain_rate % 100))
    {
        gain++;
    }

    focus_pool += (gain * base_change);
}

// written mostly by FunnyMan3595 in Github issue #613 (DarklingWolf's repo),
// with some small edits/corrections by Soron
int player::calc_focus_equilibrium()
{
    // Factor in pain, since it's harder to rest your mind while your body hurts.
    int eff_morale = morale_level() - pain;
    int focus_gain_rate = 100;
    it_book* reading;
    // apply a penalty when improving skills via books
    if (activity.type == ACT_READ)
    {
        if (this->activity.index == -2)
        {
            reading = dynamic_cast<it_book *>(weapon.type);
        }
        else
        {
            reading = dynamic_cast<it_book *>(inv.item_by_letter(activity.invlet).type);
        }
        // only apply a penalty when we're actually learning something
        if (skillLevel(reading->type) < (int)reading->level)
        {
            focus_gain_rate -= 50;
        }
    }

    if (eff_morale < -99)
    {
        // At very low morale, focus goes up at 1% of the normal rate.
        focus_gain_rate = 1;
    }
    else if (eff_morale <= 50)
    {
        // At -99 to +50 morale, each point of morale gives 1% of the normal rate.
        focus_gain_rate += eff_morale;
    }
    else
    {
        /* Above 50 morale, we apply strong diminishing returns.
         * Each block of 50% takes twice as many morale points as the previous one:
         * 150% focus gain at 50 morale (as before)
         * 200% focus gain at 150 morale (100 more morale)
         * 250% focus gain at 350 morale (200 more morale)
         * ...
         * Cap out at 400% focus gain with 3,150+ morale, mostly as a sanity check.
         */

        int block_multiplier = 1;
        int morale_left = eff_morale;
        while (focus_gain_rate < 400)
        {
            if (morale_left > 50 * block_multiplier)
            {
                // We can afford the entire block.  Get it and continue.
                morale_left -= 50 * block_multiplier;
                focus_gain_rate += 50;
                block_multiplier *= 2;
            }
            else
            {
                // We can't afford the entire block.  Each block_multiplier morale
                // points give 1% focus gain, and then we're done.
                focus_gain_rate += morale_left / block_multiplier;
                break;
            }
        }
    }

    // This should be redundant, but just in case...
    if (focus_gain_rate < 1)
    {
        focus_gain_rate = 1;
    }
    else if (focus_gain_rate > 400)
    {
        focus_gain_rate = 400;
    }

    return focus_gain_rate;
}

/* Here lies the intended effects of body temperature

Assumption 1 : a naked person is comfortable at 19C/66.2F (31C/87.8F at rest).
Assumption 2 : a "lightly clothed" person is comfortable at 13C/55.4F (25C/77F at rest).
Assumption 3 : the player is always running, thus generating more heat.
Assumption 4 : frostbite cannot happen above 0C temperature.*
* In the current model, a naked person can get frostbite at 1C. This isn't true, but it's a compromise with using nice whole numbers.

Here is a list of warmth values and the corresponding temperatures in which the player is comfortable, and in which the player is very cold.

Warmth  Temperature (Comfortable)    Temperature (Very cold)    Notes
  0       19C /  66.2F               -11C /  12.2F               * Naked
 10       13C /  55.4F               -17C /   1.4F               * Lightly clothed
 20        7C /  44.6F               -23C /  -9.4F
 30        1C /  33.8F               -29C / -20.2F
 40       -5C /  23.0F               -35C / -31.0F
 50      -11C /  12.2F               -41C / -41.8F
 60      -17C /   1.4F               -47C / -52.6F
 70      -23C /  -9.4F               -53C / -63.4F
 80      -29C / -20.2F               -59C / -74.2F
 90      -35C / -31.0F               -65C / -85.0F
100      -41C / -41.8F               -71C / -95.8F
*/

void player::update_bodytemp(game *g)
{
    // NOTE : visit weather.h for some details on the numbers used
    // Converts temperature to Celsius/10(Wito plans on using degrees Kelvin later)
    int Ctemperature = 100*(g->temperature - 32) * 5/9;
    // Temperature norms
    // Ambient normal temperature is lower while asleep
    int ambient_norm = (has_disease("sleep") ? 3100 : 1900);
    // This adjusts the temperature scale to match the bodytemp scale
    int adjusted_temp = (Ctemperature - ambient_norm);
    // This gets incremented in the for loop and used in the morale calculation
    int morale_pen = 0;
    const trap_id trap_at_pos = g->m.tr_at(posx, posy);
    const ter_id ter_at_pos = g->m.ter(posx, posy);
    const furn_id furn_at_pos = g->m.furn(posx, posy);

    // Current temperature and converging temperature calculations
    for (int i = 0 ; i < num_bp ; i++)
    {
        // Skip eyes
        if (i == bp_eyes) { continue; }
        // Represents the fact that the body generates heat when it is cold. TODO : should this increase hunger?
        float homeostasis_adjustement = (temp_cur[i] > BODYTEMP_NORM ? 30.0 : 60.0);
        int clothing_warmth_adjustement = homeostasis_adjustement * warmth(body_part(i));
        // Disease name shorthand
        dis_type blister_pen = disease_for_body_part("blisters", i), hot_pen  = disease_for_body_part("hot", i);
        dis_type cold_pen = disease_for_body_part("cold", i), frost_pen = disease_for_body_part("frostbite", i);
        // Convergeant temperature is affected by ambient temperature, clothing warmth, and body wetness.
        temp_conv[i] = BODYTEMP_NORM + adjusted_temp + clothing_warmth_adjustement;
        // HUNGER
        temp_conv[i] -= hunger/6 + 100;
        // FATIGUE
        if (!has_disease("sleep")) { temp_conv[i] -= 1.5*fatigue; }
        else
        {
            int vpart = -1;
            vehicle *veh = g->m.veh_at (posx, posy, vpart);
            if      (furn_at_pos == f_bed)
            {
                temp_conv[i] += 1000;
            }
            else if (furn_at_pos == f_makeshift_bed ||
                     furn_at_pos == f_armchair ||
                     furn_at_pos == f_sofa)
            {
                temp_conv[i] += 500;
            }
            else if (trap_at_pos == tr_cot)
            {
                temp_conv[i] -= 500;
            }
            else if (trap_at_pos == tr_rollmat)
            {
                temp_conv[i] -= 1000;
            }
            else if (veh && veh->part_with_feature (vpart, vpf_seat) >= 0)
            {
                temp_conv[i] += 200;
            }
            else if (veh && veh->part_with_feature (vpart, vpf_bed) >= 0)
            {
                temp_conv[i] += 300;
            }
            else
            {
                temp_conv[i] -= 2000;
            }
        }
        // CONVECTION HEAT SOURCES (generates body heat, helps fight frostbite)
        int blister_count = 0; // If the counter is high, your skin starts to burn
        for (int j = -6 ; j <= 6 ; j++)
        {
            for (int k = -6 ; k <= 6 ; k++)
            {
                int heat_intensity = 0;
                field &local_field = g->m.field_at(posx + j, posy + k);
                if(local_field.findField(fd_fire))
                {
                    heat_intensity = local_field.findField(fd_fire)->getFieldDensity();
                }
                else if (g->m.tr_at(posx + j, posy + k) == tr_lava )
                {
                    heat_intensity = 3;
                }
                if (heat_intensity > 0 && g->u_see(posx + j, posy + k))
                {
                    // Ensure fire_dist >=1 to avoid divide-by-zero errors.
                    int fire_dist = std::max(1, std::max(j, k));
                    if (frostbite_timer[i] > 0)
                        { frostbite_timer[i] -= heat_intensity - fire_dist / 2;}
                    temp_conv[i] +=  300 * heat_intensity * heat_intensity / (fire_dist * fire_dist);
                    blister_count += heat_intensity / (fire_dist * fire_dist);
                }
            }
        }
        // TILES
        // Being on fire affects temp_cur (not temp_conv): this is super dangerous for the player
        if (has_disease("onfire")) { temp_cur[i] += 250; }
        field &local_field = g->m.field_at(posx, posy);
        if ((local_field.findField(fd_fire) && local_field.findField(fd_fire)->getFieldDensity() > 2)
            || trap_at_pos == tr_lava)
        {
            temp_cur[i] += 250;
        }
        // WEATHER
        if (g->weather == WEATHER_SUNNY && g->is_in_sunlight(posx, posy))
        {
            temp_conv[i] += 1000;
        }
        if (g->weather == WEATHER_CLEAR && g->is_in_sunlight(posx, posy))
        {
            temp_conv[i] += 500;
        }
        // DISEASES
        if (has_disease("flu") && i == bp_head) { temp_conv[i] += 1500; }
        if (has_disease("common_cold")) { temp_conv[i] -= 750; }
        // BIONICS
        // Bionic "Internal Climate Control" says it eases the effects of high and low ambient temps
        const int variation = BODYTEMP_NORM*0.5;
        if (in_climate_control(g)
            && temp_conv[i] < BODYTEMP_SCORCHING + variation
            && temp_conv[i] > BODYTEMP_FREEZING - variation)
        {
            if      (temp_conv[i] > BODYTEMP_SCORCHING)
            {
                temp_conv[i] = BODYTEMP_VERY_HOT;
            }
            else if (temp_conv[i] > BODYTEMP_VERY_HOT)
            {
                temp_conv[i] = BODYTEMP_HOT;
            }
            else if (temp_conv[i] > BODYTEMP_HOT)
            {
                temp_conv[i] = BODYTEMP_NORM;
            }
            else if (temp_conv[i] < BODYTEMP_FREEZING)
            {
                temp_conv[i] = BODYTEMP_VERY_COLD;
            }
            else if (temp_conv[i] < BODYTEMP_VERY_COLD)
            {
                temp_conv[i] = BODYTEMP_COLD;
            }
            else if (temp_conv[i] < BODYTEMP_COLD)
            {
                temp_conv[i] = BODYTEMP_NORM;
            }
        }
        // Bionic "Thermal Dissipation" says it prevents fire damage up to 2000F. 500 is picked at random...
        if (has_bionic("bio_heatsink") && blister_count < 500)
        {
            blister_count = (has_trait(PF_BARK) ? -100 : 0);
        }
        // BLISTERS : Skin gets blisters from intense heat exposure.
        if (blister_count - 10*resist(body_part(i)) > 20)
        {
            add_disease(dis_type(blister_pen), 1);
        }
        // BLOOD LOSS : Loss of blood results in loss of body heat
        int blood_loss = 0;
        if      (i == bp_legs)
        {
            blood_loss = (100 - 100*(hp_cur[hp_leg_l] + hp_cur[hp_leg_r]) / (hp_max[hp_leg_l] + hp_max[hp_leg_r]));
        }
        else if (i == bp_arms)
        {
            blood_loss = (100 - 100*(hp_cur[hp_arm_l] + hp_cur[hp_arm_r]) / (hp_max[hp_arm_l] + hp_max[hp_arm_r]));
        }
        else if (i == bp_torso)
        {
            blood_loss = (100 - 100* hp_cur[hp_torso] / hp_max[hp_torso]);
        }
        else if (i == bp_head)
        {
            blood_loss = (100 - 100* hp_cur[hp_head] / hp_max[hp_head]);
        }
        temp_conv[i] -= blood_loss*temp_conv[i]/200; // 1% bodyheat lost per 2% hp lost
        // EQUALIZATION
        switch (i)
        {
            case bp_torso :
                temp_equalizer(bp_torso, bp_arms);
                temp_equalizer(bp_torso, bp_legs);
                temp_equalizer(bp_torso, bp_head);
                break;
            case bp_head :
                temp_equalizer(bp_head, bp_torso);
                temp_equalizer(bp_head, bp_mouth);
                break;
            case bp_arms :
                temp_equalizer(bp_arms, bp_torso);
                temp_equalizer(bp_arms, bp_hands);
                break;
            case bp_legs :
                temp_equalizer(bp_legs, bp_torso);
                temp_equalizer(bp_legs, bp_feet);
                break;
            case bp_mouth : temp_equalizer(bp_mouth, bp_head); break;
            case bp_hands : temp_equalizer(bp_hands, bp_arms); break;
            case bp_feet  : temp_equalizer(bp_feet, bp_legs); break;
        }
        // MUTATIONS and TRAITS
        // Bark : lowers blister count to -100; harder to get blisters
        // Lightly furred
        if (has_trait(PF_LIGHTFUR))
        {
            temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 250 : 500);
        }
        // Furry
        if (has_trait(PF_FUR))
        {
            temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 750 : 1500);
        }
        // Disintergration
        if (has_trait(PF_ROT1)) { temp_conv[i] -= 250;}
        else if (has_trait(PF_ROT2)) { temp_conv[i] -= 750;}
        else if (has_trait(PF_ROT3)) { temp_conv[i] -= 1500;}
        // Radioactive
        if (has_trait(PF_RADIOACTIVE1)) { temp_conv[i] += 250; }
        else if (has_trait(PF_RADIOACTIVE2)) { temp_conv[i] += 750; }
        else if (has_trait(PF_RADIOACTIVE3)) { temp_conv[i] += 1500; }
        // Chemical Imbalance
        // Added linse in player::suffer()
        // FINAL CALCULATION : Increments current body temperature towards convergant.
        int temp_before = temp_cur[i];
        int temp_difference = temp_cur[i] - temp_conv[i]; // Negative if the player is warming up.
        // exp(-0.001) : half life of 60 minutes, exp(-0.002) : half life of 30 minutes,
        // exp(-0.003) : half life of 20 minutes, exp(-0.004) : half life of 15 minutes
        int rounding_error = 0;
        // If temp_diff is small, the player cannot warm up due to rounding errors. This fixes that.
        if (temp_difference < 0 && temp_difference > -600 )
        {
            rounding_error = 1;
        }
        if (temp_cur[i] != temp_conv[i])
        {
            if      ((ter_at_pos == t_water_sh || ter_at_pos == t_sewage)
                    && (i == bp_feet || i == bp_legs))
            {
                temp_cur[i] = temp_difference*exp(-0.004) + temp_conv[i] + rounding_error;
            }
            else if (ter_at_pos == t_water_dp)
            {
                temp_cur[i] = temp_difference*exp(-0.004) + temp_conv[i] + rounding_error;
            }
            else if (i == bp_torso || i == bp_head)
            {
                temp_cur[i] = temp_difference*exp(-0.003) + temp_conv[i] + rounding_error;
            }
            else
            {
                temp_cur[i] = temp_difference*exp(-0.002) + temp_conv[i] + rounding_error;
            }
        }
        int temp_after = temp_cur[i];
        // PENALTIES
        if      (temp_cur[i] < BODYTEMP_FREEZING)
        {
            add_disease(dis_type(cold_pen), 1, 3, 3); frostbite_timer[i] += 3;
        }
        else if (temp_cur[i] < BODYTEMP_VERY_COLD)
        {
            add_disease(dis_type(cold_pen), 1, 2, 3); frostbite_timer[i] += 2;
        }
        else if (temp_cur[i] < BODYTEMP_COLD)
        {
            // Frostbite timer does not go down if you are still cold.
            add_disease(dis_type(cold_pen), 1, 1, 3); frostbite_timer[i] += 1;
        }
        else if (temp_cur[i] > BODYTEMP_SCORCHING)
        {
            // If body temp rises over 15000, disease.cpp ("hot_head") acts weird and the player will die
            add_disease(dis_type(hot_pen),  1, 3, 3);
        }
        else if (temp_cur[i] > BODYTEMP_VERY_HOT)
        {
            add_disease(dis_type(hot_pen),  1, 2, 3);
        }
        else if (temp_cur[i] > BODYTEMP_HOT)
        {
            add_disease(dis_type(hot_pen),  1, 1, 3);
        }
        // MORALE : a negative morale_pen means the player is cold
        // Intensity multiplier is negative for cold, positive for hot
        int intensity_mult =
            - disease_intensity(dis_type(cold_pen)) + disease_intensity(dis_type(hot_pen));
        if (has_disease(dis_type(cold_pen)) || has_disease(dis_type(hot_pen)))
        {
            switch (i)
            {
                case bp_head :
                case bp_torso :
                case bp_mouth : morale_pen += 2*intensity_mult;
                case bp_arms :
                case bp_legs : morale_pen += 1*intensity_mult;
                case bp_hands:
                case bp_feet : morale_pen += 1*intensity_mult;
            }
        }
        // FROSTBITE (level 1 after 2 hours, level 2 after 4 hours)
        if      (frostbite_timer[i] >   0)
        {
            frostbite_timer[i]--;
        }
        if      (frostbite_timer[i] >= 240 && g->temperature < 32)
        {
            add_disease(dis_type(frost_pen), 1, 2, 2);
            // Warning message for the player
            if (disease_intensity(dis_type(frost_pen)) < 2
                &&  (i == bp_mouth || i == bp_hands || i == bp_feet))
            {
                g->add_msg((i == bp_mouth ? _("Your %s harden from the frostbite!")  : _("Your %s hardens from the frostbite!")), body_part_name(body_part(i), -1).c_str());
            }
            else if (frostbite_timer[i] >= 120 && g->temperature < 32)
            {
                add_disease(dis_type(frost_pen), 1, 1, 2);
                // Warning message for the player
                if (!has_disease(dis_type(frost_pen)))
                {
                    g->add_msg(_("You lose sensation in your %s."),
                        body_part_name(body_part(i), -1).c_str());
                }
            }
        }
        // Warn the player if condition worsens
        if  (temp_before > BODYTEMP_FREEZING && temp_after < BODYTEMP_FREEZING)
        {
            g->add_msg(_("You feel your %s beginning to go numb from the cold!"),
                body_part_name(body_part(i), -1).c_str());
        }
        else if (temp_before > BODYTEMP_VERY_COLD && temp_after < BODYTEMP_VERY_COLD)
        {
            g->add_msg(_("You feel your %s getting very cold."),
                body_part_name(body_part(i), -1).c_str());
        }
        else if (temp_before > BODYTEMP_COLD && temp_after < BODYTEMP_COLD)
        {
            g->add_msg(_("You feel your %s getting chilly."),
                body_part_name(body_part(i), -1).c_str());
        }
        else if (temp_before < BODYTEMP_SCORCHING && temp_after > BODYTEMP_SCORCHING)
        {
            g->add_msg(_("You feel your %s getting red hot from the heat!"),
                body_part_name(body_part(i), -1).c_str());
        }
        else if (temp_before < BODYTEMP_VERY_HOT && temp_after > BODYTEMP_VERY_HOT)
        {
            g->add_msg(_("You feel your %s getting very hot."),
                body_part_name(body_part(i), -1).c_str());
        }
        else if (temp_before < BODYTEMP_HOT && temp_after > BODYTEMP_HOT)
        {
            g->add_msg(_("You feel your %s getting warm."),
                body_part_name(body_part(i), -1).c_str());
        }
    }
    // Morale penalties, updated at the same rate morale is
    if (morale_pen < 0 && int(g->turn) % 10 == 0)
    {
        add_morale(MORALE_COLD, -2, -abs(morale_pen), 10, 5, true);
    }
    if (morale_pen > 0 && int(g->turn) % 10 == 0)
    {
        add_morale(MORALE_HOT,  -2, -abs(morale_pen), 10, 5, true);
    }
}

void player::temp_equalizer(body_part bp1, body_part bp2)
{
 // Body heat is moved around.
 // Shift in one direction only, will be shifted in the other direction seperately.
 int diff = (temp_cur[bp2] - temp_cur[bp1])*0.0001; // If bp1 is warmer, it will lose heat
 temp_cur[bp1] += diff;
}

int player::current_speed(game *g)
{
 int newmoves = 100; // Start with 100 movement points...
// Minus some for weight...
 int carry_penalty = 0;
 if (weight_carried() > int(weight_capacity() * .25))
  carry_penalty = 75 * double((weight_carried() - int(weight_capacity() * .25))/
                              (weight_capacity() * .75));

 newmoves -= carry_penalty;

 if (pain > pkill) {
  int pain_penalty = int((pain - pkill) * .7);
  if (pain_penalty > 60)
   pain_penalty = 60;
  newmoves -= pain_penalty;
 }
 if (pkill >= 10) {
  int pkill_penalty = int(pkill * .1);
  if (pkill_penalty > 30)
   pkill_penalty = 30;
  newmoves -= pkill_penalty;
 }

 if (abs(morale_level()) >= 100) {
  int morale_bonus = int(morale_level() / 25);
  if (morale_bonus < -10)
   morale_bonus = -10;
  else if (morale_bonus > 10)
   morale_bonus = 10;
  newmoves += morale_bonus;
 }

 if (radiation >= 40) {
  int rad_penalty = radiation / 40;
  if (rad_penalty > 20)
   rad_penalty = 20;
  newmoves -= rad_penalty;
 }

 if (thirst > 40)
  newmoves -= int((thirst - 40) / 10);
 if (hunger > 100)
  newmoves -= int((hunger - 100) / 10);

 newmoves += (stim > 40 ? 40 : stim);

 for (int i = 0; i < illness.size(); i++)
  newmoves += disease_speed_boost(illness[i]);

 if (has_trait(PF_QUICK))
  newmoves = int(newmoves * 1.10);

 if (g != NULL) {
  if (has_trait(PF_SUNLIGHT_DEPENDENT) && !g->is_in_sunlight(posx, posy))
   newmoves -= (g->light_level() >= 12 ? 5 : 10);
  if (has_trait(PF_COLDBLOOD3) && g->temperature < 60)
   newmoves -= int( (65 - g->temperature) / 2);
  else if (has_trait(PF_COLDBLOOD2) && g->temperature < 60)
   newmoves -= int( (65 - g->temperature) / 3);
  else if (has_trait(PF_COLDBLOOD) && g->temperature < 60)
   newmoves -= int( (65 - g->temperature) / 5);
 }

 if (has_artifact_with(AEP_SPEED_UP))
  newmoves += 20;
 if (has_artifact_with(AEP_SPEED_DOWN))
  newmoves -= 20;

 if (newmoves < 25)
  newmoves = 25;

 return newmoves;
}

int player::run_cost(int base_cost, bool diag)
{
    float movecost = float(base_cost);
    if (diag)
        movecost *= 0.7071; // because everything here assumes 100 is base
    bool flatground = movecost < 105;

    if (has_trait(PF_PARKOUR) && movecost > 100 ) {
        movecost *= .5;
        if (movecost < 100)
            movecost = 100;
    }

    if (hp_cur[hp_leg_l] == 0)
        movecost += 50;
    else if (hp_cur[hp_leg_l] < hp_max[hp_leg_l] * .40)
        movecost += 25;
    if (hp_cur[hp_leg_r] == 0)
        movecost += 50;
    else if (hp_cur[hp_leg_r] < hp_max[hp_leg_r] * .40)
        movecost += 25;

    if (has_trait(PF_FLEET) && flatground)
        movecost *= .85;
    if (has_trait(PF_FLEET2) && flatground)
        movecost *= .7;
    if (has_trait(PF_PADDED_FEET) && !wearing_something_on(bp_feet))
        movecost *= .9;
    if (has_trait(PF_LIGHT_BONES))
        movecost *= .9;
    if (has_trait(PF_HOLLOW_BONES))
        movecost *= .8;
    if (has_trait(PF_WINGS_INSECT))
        movecost -= 15;
    if (has_trait(PF_LEG_TENTACLES))
        movecost += 20;
    if (has_trait(PF_PONDEROUS1))
        movecost *= 1.1;
    if (has_trait(PF_PONDEROUS2))
        movecost *= 1.2;
    if (has_trait(PF_PONDEROUS3))
        movecost *= 1.3;

    movecost += encumb(bp_feet) * 5 + encumb(bp_legs) * 3;

    if (!wearing_something_on(bp_feet) && !has_trait(PF_PADDED_FEET) &&
            !has_trait(PF_HOOVES))
        movecost += 15;

    if (diag)
        movecost *= 1.4142;

    return int(movecost);
}

int player::swim_speed()
{
  int ret = 440 + 2 * weight_carried() - 50 * skillLevel("swimming");
 if (has_trait(PF_WEBBED))
  ret -= 60 + str_cur * 5;
 if (has_trait(PF_TAIL_FIN))
  ret -= 100 + str_cur * 10;
 if (has_trait(PF_SLEEK_SCALES))
  ret -= 100;
 if (has_trait(PF_LEG_TENTACLES))
  ret -= 60;
 ret += (50 - skillLevel("swimming") * 2) * encumb(bp_legs);
 ret += (80 - skillLevel("swimming") * 3) * encumb(bp_torso);
 if (skillLevel("swimming") < 10) {
  for (int i = 0; i < worn.size(); i++)
    ret += (worn[i].volume() * (10 - skillLevel("swimming"))) / 2;
 }
 ret -= str_cur * 6 + dex_cur * 4;
 if( worn_with_flag("FLOATATION") ) {
     ret = std::max(ret, 400);
     ret = std::min(ret, 200);
 }
// If (ret > 500), we can not swim; so do not apply the underwater bonus.
 if (underwater && ret < 500)
  ret -= 50;
 if (ret < 30)
  ret = 30;
 return ret;
}

nc_color player::color()
{
 if (has_disease("onfire"))
  return c_red;
 if (has_disease("stunned"))
  return c_ltblue;
 if (has_disease("boomered"))
  return c_pink;
 if (underwater)
  return c_blue;
 if (has_active_bionic("bio_cloak") || has_artifact_with(AEP_INVISIBLE))
  return c_dkgray;
 return c_white;
}

void player::load_info(game *g, std::string data)
{
 std::stringstream dump;
 dump << data;
 int inveh, vctrl;
 itype_id styletmp;
 std::string prof_ident;

 dump >> posx >> posy >> str_cur >> str_max >> dex_cur >> dex_max >>
         int_cur >> int_max >> per_cur >> per_max >> power_level >>
         max_power_level >> hunger >> thirst >> fatigue >> stim >>
         pain >> pkill >> radiation >> cash >> recoil >> driving_recoil >>
         inveh >> vctrl >> scent >> moves >> underwater >> dodges_left >>
         blocks_left >> oxygen >> active_mission >> focus_pool >> male >>
         prof_ident >> health >> styletmp;

 if (profession::exists(prof_ident)) {
  prof = profession::prof(prof_ident);
 } else {
  prof = profession::generic();
  debugmsg("Tried to use non-existent profession '%s'", prof_ident.c_str());
 }

 activity.load_info(dump);
 backlog.load_info(dump);

 in_vehicle = inveh != 0;
 controlling_vehicle = vctrl != 0;
 style_selected = styletmp;

 for (int i = 0; i < PF_MAX2; i++)
  dump >> my_traits[i];

 for (int i = 0; i < PF_MAX2; i++)
  dump >> my_mutations[i];

 for (int i = 0; i < NUM_MUTATION_CATEGORIES; i++)
  dump >> mutation_category_level[i];

 for (int i = 0; i < num_hp_parts; i++)
  dump >> hp_cur[i] >> hp_max[i];
 for (int i = 0; i < num_bp; i++)
  dump >> temp_cur[i] >> temp_conv[i] >> frostbite_timer[i];

 for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin(); aSkill != Skill::skills.end(); ++aSkill) {
   dump >> skillLevel(*aSkill);
 }

 int num_recipes;
 std::string rec_name;
 dump >> num_recipes;
 for (int i = 0; i < num_recipes; ++i)
 {
  dump >> rec_name;
  learned_recipes[rec_name] = g->recipe_by_name(rec_name);
 }

 int numstyles;
 itype_id styletype;
 dump >> numstyles;
 for (int i = 0; i < numstyles; i++) {
  dump >> styletype;
  styles.push_back( styletype );
 }

 int numill;
 disease illtmp;
 dump >> numill;
 for (int i = 0; i < numill; i++) {
  dump >> illtmp.type >> illtmp.duration >> illtmp.intensity;
  illness.push_back(illtmp);
 }

 int numadd = 0;
 int typetmp;
 addiction addtmp;
 dump >> numadd;
 for (int i = 0; i < numadd; i++) {
  dump >> typetmp >> addtmp.intensity >> addtmp.sated;
  addtmp.type = add_type(typetmp);
  addictions.push_back(addtmp);
 }

 int numbio = 0;
 bionic_id biotype;
 bionic biotmp;
 dump >> numbio;
 for (int i = 0; i < numbio; i++) {
  dump >> biotype >> biotmp.invlet >> biotmp.powered >> biotmp.charge;
  biotmp.id = biotype;
  my_bionics.push_back(biotmp);
 }

 int nummor;
 morale_point mortmp;
 dump >> nummor;
 for (int i = 0; i < nummor; i++) {
  // Load morale properties in structure order.
  int mortype;
  std::string item_id;
  dump >> mortype >> item_id;
  mortmp.type = morale_type(mortype);
  if (g->itypes.find(item_id) == g->itypes.end())
   mortmp.item_type = NULL;
  else
   mortmp.item_type = g->itypes[item_id];

  dump >> mortmp.bonus >> mortmp.duration >> mortmp.decay_start
       >> mortmp.age;
  morale.push_back(mortmp);
 }

 int nummis = 0;
 int mistmp;
 dump >> nummis;
 for (int i = 0; i < nummis; i++) {
  dump >> mistmp;
  active_missions.push_back(mistmp);
 }
 dump >> nummis;
 for (int i = 0; i < nummis; i++) {
  dump >> mistmp;
  completed_missions.push_back(mistmp);
 }
 dump >> nummis;
 for (int i = 0; i < nummis; i++) {
  dump >> mistmp;
  failed_missions.push_back(mistmp);
 }
}

std::string player::save_info()
{
 std::stringstream dump;
 dump << posx    << " " << posy    << " " << str_cur << " " << str_max << " " <<
         dex_cur << " " << dex_max << " " << int_cur << " " << int_max << " " <<
         per_cur << " " << per_max << " " << power_level << " " <<
         max_power_level << " " << hunger << " " << thirst << " " << fatigue <<
         " " << stim << " " << pain << " " << pkill << " " << radiation <<
         " " << cash << " " << recoil << " " << driving_recoil << " " <<
         (in_vehicle? 1 : 0) << " " << (controlling_vehicle? 1 : 0) << " " <<
         scent << " " << moves << " " << underwater << " " << dodges_left <<
         " " << blocks_left << " " << oxygen << " " << active_mission << " " <<
         focus_pool << " " << male << " " << prof->ident() << " " << health <<
         " " << style_selected << " " << activity.save_info() << " " <<
         backlog.save_info() << " ";

 for (int i = 0; i < PF_MAX2; i++)
  dump << my_traits[i] << " ";
 for (int i = 0; i < PF_MAX2; i++)
  dump << my_mutations[i] << " ";
 for (int i = 0; i < NUM_MUTATION_CATEGORIES; i++)
  dump << mutation_category_level[i] << " ";
 for (int i = 0; i < num_hp_parts; i++)
  dump << hp_cur[i] << " " << hp_max[i] << " ";
 for (int i = 0; i < num_bp; i++)
  dump << temp_cur[i] << " " << temp_conv[i] << " " << frostbite_timer[i] << " ";

 for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin(); aSkill != Skill::skills.end(); ++aSkill) {
   SkillLevel level = skillLevel(*aSkill);
   dump << level;
 }

 dump << learned_recipes.size() << " ";
 for (std::map<std::string, recipe*>::iterator iter = learned_recipes.begin();
      iter != learned_recipes.end();
      ++iter)
 {
  dump << iter->first << " ";
 }

 dump << styles.size() << " ";
 for (int i = 0; i < styles.size(); i++)
  dump << styles[i] << " ";

 dump << illness.size() << " ";
 for (int i = 0; i < illness.size();  i++)
  dump << illness[i].type << " " << illness[i].duration << " " << illness[i].intensity << " " ;

 dump << addictions.size() << " ";
 for (int i = 0; i < addictions.size(); i++)
  dump << int(addictions[i].type) << " " << addictions[i].intensity << " " <<
          addictions[i].sated << " ";

 dump << my_bionics.size() << " ";
 for (int i = 0; i < my_bionics.size(); i++)
  dump << my_bionics[i].id << " " << my_bionics[i].invlet << " " <<
          my_bionics[i].powered << " " << my_bionics[i].charge << " ";

 dump << morale.size() << " ";
 for (int i = 0; i < morale.size(); i++) {
  // Output morale properties in structure order.
  dump << morale[i].type << " ";
  if (morale[i].item_type == NULL)
   dump << "0";
  else
   dump << morale[i].item_type->id;
  dump << " " << morale[i].bonus << " " << morale[i].duration << " "
       << morale[i].decay_start << " " << morale[i].age << " ";
 }

 dump << " " << active_missions.size() << " ";
 for (int i = 0; i < active_missions.size(); i++)
  dump << active_missions[i] << " ";

 dump << " " << completed_missions.size() << " ";
 for (int i = 0; i < completed_missions.size(); i++)
  dump << completed_missions[i] << " ";

 dump << " " << failed_missions.size() << " ";
 for (int i = 0; i < failed_missions.size(); i++)
  dump << failed_missions[i] << " ";

 dump << std::endl;

 dump << inv.save_str_no_quant();

 for (int i = 0; i < worn.size(); i++) {
  dump << "W " << worn[i].save_info() << std::endl;
  for (int j = 0; j < worn[i].contents.size(); j++)
   dump << "S " << worn[i].contents[j].save_info() << std::endl;
 }
 if (!weapon.is_null())
  dump << "w " << weapon.save_info() << std::endl;
 for (int j = 0; j < weapon.contents.size(); j++)
  dump << "c " << weapon.contents[j].save_info() << std::endl;

 return dump.str();
}

void player::disp_info(game *g)
{
 int line;
 std::vector<std::string> effect_name;
 std::vector<std::string> effect_text;
 for (int i = 0; i < illness.size(); i++) {
  if (dis_name(illness[i]).size() > 0) {
   effect_name.push_back(dis_name(illness[i]));
   effect_text.push_back(dis_description(illness[i]));
  }
 }
 if (abs(morale_level()) >= 100) {
  bool pos = (morale_level() > 0);
  effect_name.push_back(pos ? _("Elated") : _("Depressed"));
  std::stringstream morale_text;
  if (abs(morale_level()) >= 200)
   morale_text << _("Dexterity") << (pos ? " +" : " ") <<
                   int(morale_level() / 200) << "   ";
  if (abs(morale_level()) >= 180)
   morale_text << _("Strength") << (pos ? " +" : " ") <<
                  int(morale_level() / 180) << "   ";
  if (abs(morale_level()) >= 125)
   morale_text << _("Perception") << (pos ? " +" : " ") <<
                  int(morale_level() / 125) << "   ";
  morale_text << _("Intelligence") << (pos ? " +" : " ") <<
                 int(morale_level() / 100) << "   ";
  effect_text.push_back(morale_text.str());
 }
 if (pain - pkill > 0) {
  effect_name.push_back(_("Pain"));
  std::stringstream pain_text;
  if (pain - pkill >= 15)
   pain_text << "Strength" << " -" << int((pain - pkill) / 15) << "   " << _("Dexterity") << " -" <<
                int((pain - pkill) / 15) << "   ";
  if (pain - pkill >= 20)
   pain_text << _("Perception") << " -" << int((pain - pkill) / 15) << "   ";
  pain_text << _("Intelligence") << " -" << 1 + int((pain - pkill) / 25);
  effect_text.push_back(pain_text.str());
 }
 if (stim > 0) {
  int dexbonus = int(stim / 10);
  int perbonus = int(stim /  7);
  int intbonus = int(stim /  6);
  if (abs(stim) >= 30) {
   dexbonus -= int(abs(stim - 15) /  8);
   perbonus -= int(abs(stim - 15) / 12);
   intbonus -= int(abs(stim - 15) / 14);
  }

  if (dexbonus < 0)
   effect_name.push_back(_("Stimulant Overdose"));
  else
   effect_name.push_back(_("Stimulant"));
  std::stringstream stim_text;
  stim_text << _("Speed") << " +" << stim << "   " << _("Intelligence") <<
               (intbonus > 0 ? " + " : " ") << intbonus << "   " << _("Perception") <<
               (perbonus > 0 ? " + " : " ") << perbonus << "   " << _("Dexterity")  <<
               (dexbonus > 0 ? " + " : " ") << dexbonus;
  effect_text.push_back(stim_text.str());
 } else if (stim < 0) {
  effect_name.push_back(_("Depressants"));
  std::stringstream stim_text;
  int dexpen = int(stim / 10);
  int perpen = int(stim /  7);
  int intpen = int(stim /  6);
// Since dexpen etc. are always less than 0, no need for + signs
  stim_text << _("Speed") << " " << stim << "   " << _("Intelligence") << " " << intpen <<
               "   " << _("Perception") << " " << perpen << "   " << "Dexterity" << " " << dexpen;
  effect_text.push_back(stim_text.str());
 }

 if ((has_trait(PF_TROGLO) && g->is_in_sunlight(posx, posy) &&
      g->weather == WEATHER_SUNNY) ||
     (has_trait(PF_TROGLO2) && g->is_in_sunlight(posx, posy) &&
      g->weather != WEATHER_SUNNY)) {
  effect_name.push_back(_("In Sunlight"));
  effect_text.push_back(_("The sunlight irritates you.\n\
Strength - 1;    Dexterity - 1;    Intelligence - 1;    Dexterity - 1"));
 } else if (has_trait(PF_TROGLO2) && g->is_in_sunlight(posx, posy)) {
  effect_name.push_back(_("In Sunlight"));
  effect_text.push_back(_("The sunlight irritates you badly.\n\
Strength - 2;    Dexterity - 2;    Intelligence - 2;    Dexterity - 2"));
 } else if (has_trait(PF_TROGLO3) && g->is_in_sunlight(posx, posy)) {
  effect_name.push_back(_("In Sunlight"));
  effect_text.push_back(_("The sunlight irritates you terribly.\n\
Strength - 4;    Dexterity - 4;    Intelligence - 4;    Dexterity - 4"));
 }

 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].sated < 0 &&
      addictions[i].intensity >= MIN_ADDICTION_LEVEL) {
   effect_name.push_back(addiction_name(addictions[i]));
   effect_text.push_back(addiction_text(addictions[i]));
  }
 }

 int maxy = (VIEWY*2)+1;
 if (maxy < 25)
  maxy = 25;

 int effect_win_size_y = 0;
 int trait_win_size_y = 0;
 int skill_win_size_y = 0;
 int infooffsetytop = 11;
 int infooffsetybottom = 15;
 std::vector<pl_flag> traitslist;

 effect_win_size_y = effect_name.size()+1;

 for(int i = 0; i < PF_MAX2; i++) {
  if(my_mutations[i]) {
   traitslist.push_back(pl_flag(i));
  }
 }

 trait_win_size_y = traitslist.size()+1;
 if (trait_win_size_y + infooffsetybottom > maxy ) {
  trait_win_size_y = maxy - infooffsetybottom;
 }

 skill_win_size_y = Skill::skill_count() + 1;
 if (skill_win_size_y + infooffsetybottom > maxy ) {
  skill_win_size_y = maxy - infooffsetybottom;
 }
/*
 std::stringstream ssTemp;
 ssTemp  << skill_win_size_y << " - " << trait_win_size_y << " - " << effect_win_size_y;
 debugmsg((ssTemp.str()).c_str());
*/
 WINDOW* w_grid_top    = newwin(infooffsetybottom, 81,  VIEW_OFFSET_Y,  VIEW_OFFSET_X);
 WINDOW* w_grid_skill  = newwin(skill_win_size_y + 1, 27, infooffsetybottom + VIEW_OFFSET_Y, 0 + VIEW_OFFSET_X);
 WINDOW* w_grid_trait  = newwin(trait_win_size_y + 1, 27, infooffsetybottom + VIEW_OFFSET_Y, 27 + VIEW_OFFSET_X);
 WINDOW* w_grid_effect = newwin(effect_win_size_y+ 1, 28, infooffsetybottom + VIEW_OFFSET_Y, 53 + VIEW_OFFSET_X);

 WINDOW* w_tip     = newwin(1, FULL_SCREEN_WIDTH,  VIEW_OFFSET_Y,  0 + VIEW_OFFSET_X);
 WINDOW* w_stats   = newwin(9, 26,  1 + VIEW_OFFSET_Y,  0 + VIEW_OFFSET_X);
 WINDOW* w_traits  = newwin(trait_win_size_y, 26, infooffsetybottom + VIEW_OFFSET_Y,  27 + VIEW_OFFSET_X);
 WINDOW* w_encumb  = newwin(9, 26,  1 + VIEW_OFFSET_Y, 27 + VIEW_OFFSET_X);
 WINDOW* w_effects = newwin(effect_win_size_y, 26, infooffsetybottom + VIEW_OFFSET_Y, 54 + VIEW_OFFSET_X);
 WINDOW* w_speed   = newwin(9, 26,  1 + VIEW_OFFSET_Y, 54 + VIEW_OFFSET_X);
 WINDOW* w_skills  = newwin(skill_win_size_y, 26, infooffsetybottom + VIEW_OFFSET_Y, 0 + VIEW_OFFSET_X);
 WINDOW* w_info    = newwin(3, FULL_SCREEN_WIDTH, infooffsetytop + VIEW_OFFSET_Y,  0 + VIEW_OFFSET_X);

 for (int i = 0; i < 81; i++) {
  //Horizontal line top grid
  mvwputch(w_grid_top, 10, i, c_ltgray, LINE_OXOX);
  mvwputch(w_grid_top, 14, i, c_ltgray, LINE_OXOX);

  //Vertical line top grid
  if (i <= infooffsetybottom) {
   mvwputch(w_grid_top, i, 26, c_ltgray, LINE_XOXO);
   mvwputch(w_grid_top, i, 53, c_ltgray, LINE_XOXO);
   mvwputch(w_grid_top, i, FULL_SCREEN_WIDTH, c_ltgray, LINE_XOXO);
  }

  //Horizontal line skills
  if (i <= 26) {
   mvwputch(w_grid_skill, skill_win_size_y, i, c_ltgray, LINE_OXOX);
  }

  //Vertical line skills
  if (i <= skill_win_size_y) {
   mvwputch(w_grid_skill, i, 26, c_ltgray, LINE_XOXO);
  }

  //Horizontal line traits
  if (i <= 26) {
   mvwputch(w_grid_trait, trait_win_size_y, i, c_ltgray, LINE_OXOX);
  }

  //Vertical line traits
  if (i <= trait_win_size_y) {
   mvwputch(w_grid_trait, i, 26, c_ltgray, LINE_XOXO);
  }

  //Horizontal line effects
  if (i <= 27) {
   mvwputch(w_grid_effect, effect_win_size_y, i, c_ltgray, LINE_OXOX);
  }

  //Vertical line effects
  if (i <= effect_win_size_y) {
   mvwputch(w_grid_effect, i, 0, c_ltgray, LINE_XOXO);
   mvwputch(w_grid_effect, i, 27, c_ltgray, LINE_XOXO);
  }
 }

 //Intersections top grid
 mvwputch(w_grid_top, 14, 26, c_ltgray, LINE_OXXX); // T
 mvwputch(w_grid_top, 14, 53, c_ltgray, LINE_OXXX); // T
 mvwputch(w_grid_top, 10, 26, c_ltgray, LINE_XXOX); // _|_
 mvwputch(w_grid_top, 10, 53, c_ltgray, LINE_XXOX); // _|_
 mvwputch(w_grid_top, 10, FULL_SCREEN_WIDTH, c_ltgray, LINE_XOXX); // -|
 mvwputch(w_grid_top, 14, FULL_SCREEN_WIDTH, c_ltgray, LINE_XOXX); // -|
 wrefresh(w_grid_top);

 mvwputch(w_grid_skill, skill_win_size_y, 26, c_ltgray, LINE_XOOX); // _|

 if (skill_win_size_y > trait_win_size_y)
  mvwputch(w_grid_skill, trait_win_size_y, 26, c_ltgray, LINE_XXXO); // |-
 else if (skill_win_size_y == trait_win_size_y)
  mvwputch(w_grid_skill, trait_win_size_y, 26, c_ltgray, LINE_XXOX); // _|_

 mvwputch(w_grid_trait, trait_win_size_y, 26, c_ltgray, LINE_XOOX); // _|

 if (trait_win_size_y > effect_win_size_y)
  mvwputch(w_grid_trait, effect_win_size_y, 26, c_ltgray, LINE_XXXO); // |-
 else if (trait_win_size_y == effect_win_size_y)
  mvwputch(w_grid_trait, effect_win_size_y, 26, c_ltgray, LINE_XXOX); // _|_
 else if (trait_win_size_y < effect_win_size_y) {
  mvwputch(w_grid_trait, trait_win_size_y, 26, c_ltgray, LINE_XOXX); // -|
  mvwputch(w_grid_trait, effect_win_size_y, 26, c_ltgray, LINE_XXOO); // |_
 }

 mvwputch(w_grid_effect, effect_win_size_y, 0, c_ltgray, LINE_XXOO); // |_
 mvwputch(w_grid_effect, effect_win_size_y, 27, c_ltgray, LINE_XOOX); // _|

 wrefresh(w_grid_skill);
 wrefresh(w_grid_effect);
 wrefresh(w_grid_trait);

 //-1 for header
 trait_win_size_y--;
 skill_win_size_y--;
 effect_win_size_y--;

// Print name and header
 mvwprintw(w_tip, 0, 0, "%s - %s", name.c_str(), (male ? _("Male") : _("Female")));
 mvwprintz(w_tip, 0, 39, c_ltred, _("| Press TAB to cycle, ESC or q to return."));
 wrefresh(w_tip);

// First!  Default STATS screen.
 const char* title_STATS = _("STATS");
 mvwprintz(w_stats, 0, 13 - utf8_width(title_STATS)/2, c_ltgray, title_STATS);
 mvwprintz(w_stats, 2,  2, c_ltgray, "                     ");
 mvwprintz(w_stats, 2,  2, c_ltgray, _("Strength:"));
 mvwprintz(w_stats, 2,  20, c_ltgray, str_max>9?"(%d)":" (%d)", str_max);
 mvwprintz(w_stats, 3,  2, c_ltgray, "                     ");
 mvwprintz(w_stats, 3,  2, c_ltgray, _("Dexterity:"));
 mvwprintz(w_stats, 3,  20, c_ltgray, dex_max>9?"(%d)":" (%d)", dex_max);
 mvwprintz(w_stats, 4,  2, c_ltgray, "                     ");
 mvwprintz(w_stats, 4,  2, c_ltgray, _("Intelligence:"));
 mvwprintz(w_stats, 4,  20, c_ltgray, int_max>9?"(%d)":" (%d)", int_max);
 mvwprintz(w_stats, 5,  2, c_ltgray, "                     ");
 mvwprintz(w_stats, 5,  2, c_ltgray, _("Perception:"));
 mvwprintz(w_stats, 5,  20, c_ltgray, per_max>9?"(%d)":" (%d)", per_max);

 nc_color status = c_white;

 if (str_cur <= 0)
  status = c_dkgray;
 else if (str_cur < str_max / 2)
  status = c_red;
 else if (str_cur < str_max)
  status = c_ltred;
 else if (str_cur == str_max)
  status = c_white;
 else if (str_cur < str_max * 1.5)
  status = c_ltgreen;
 else
  status = c_green;
 mvwprintz(w_stats,  2, (str_cur < 10 ? 17 : 16), status, "%d", str_cur);

 if (dex_cur <= 0)
  status = c_dkgray;
 else if (dex_cur < dex_max / 2)
  status = c_red;
 else if (dex_cur < dex_max)
  status = c_ltred;
 else if (dex_cur == dex_max)
  status = c_white;
 else if (dex_cur < dex_max * 1.5)
  status = c_ltgreen;
 else
  status = c_green;
 mvwprintz(w_stats,  3, (dex_cur < 10 ? 17 : 16), status, "%d", dex_cur);

 if (int_cur <= 0)
  status = c_dkgray;
 else if (int_cur < int_max / 2)
  status = c_red;
 else if (int_cur < int_max)
  status = c_ltred;
 else if (int_cur == int_max)
  status = c_white;
 else if (int_cur < int_max * 1.5)
  status = c_ltgreen;
 else
  status = c_green;
 mvwprintz(w_stats,  4, (int_cur < 10 ? 17 : 16), status, "%d", int_cur);

 if (per_cur <= 0)
  status = c_dkgray;
 else if (per_cur < per_max / 2)
  status = c_red;
 else if (per_cur < per_max)
  status = c_ltred;
 else if (per_cur == per_max)
  status = c_white;
 else if (per_cur < per_max * 1.5)
  status = c_ltgreen;
 else
  status = c_green;
 mvwprintz(w_stats,  5, (per_cur < 10 ? 17 : 16), status, "%d", per_cur);

 wrefresh(w_stats);

// Next, draw encumberment.
 std::string asText[] = {_("Torso"), _("Head"), _("Eyes"), _("Mouth"), _("Arms"), _("Hands"), _("Legs"), _("Feet")};
 body_part aBodyPart[] = {bp_torso, bp_head, bp_eyes, bp_mouth, bp_arms, bp_hands, bp_legs, bp_feet};
 int iEnc, iLayers, iArmorEnc, iWarmth;
 
 const char *title_ENCUMB = _("ENCUMBERANCE AND WARMTH");
 mvwprintz(w_encumb, 0, 13 - utf8_width(title_ENCUMB)/2, c_ltgray, title_ENCUMB);
 for (int i=0; i < 8; i++) {
  iEnc = iLayers = iArmorEnc = iWarmth = 0;
  iWarmth = warmth(body_part(i));
  iEnc = encumb(aBodyPart[i], iLayers, iArmorEnc);
  mvwprintz(w_encumb, i+1, 1, c_ltgray, "%s:", asText[i].c_str());
  mvwprintz(w_encumb, i+1, 8, c_ltgray, "(%d)", iLayers);
  mvwprintz(w_encumb, i+1, 11, c_ltgray, "%*s%d%s%d=", (iArmorEnc < 0 || iArmorEnc > 9 ? 1 : 2), " ", iArmorEnc, "+", iEnc-iArmorEnc);
  wprintz(w_encumb, encumb_color(iEnc), "%s%d", (iEnc < 0 || iEnc > 9 ? "" : " ") , iEnc);
  // Color the warmth value to let the player know what is sufficient
  nc_color color = c_ltgray;
  if (i == bp_eyes) continue; // Eyes don't count towards warmth
  else if (temp_conv[i] >  BODYTEMP_SCORCHING) color = c_red;
  else if (temp_conv[i] >  BODYTEMP_VERY_HOT)  color = c_ltred;
  else if (temp_conv[i] >  BODYTEMP_HOT)       color = c_yellow;
  else if (temp_conv[i] >  BODYTEMP_COLD)      color = c_green; // More than cold is comfortable
  else if (temp_conv[i] >  BODYTEMP_VERY_COLD) color = c_ltblue;
  else if (temp_conv[i] >  BODYTEMP_FREEZING)  color = c_cyan;
  else if (temp_conv[i] <= BODYTEMP_FREEZING)  color = c_blue;
  wprintz(w_encumb, color, "%*s(%d)", (iWarmth > 9 ? ((iWarmth > 99) ? 1: 2) : 3), " ", iWarmth);
 }
 wrefresh(w_encumb);

// Next, draw traits.
 const char *title_TRAITS = _("TRAITS");
 mvwprintz(w_traits, 0, 13 - utf8_width(title_TRAITS)/2, c_ltgray, title_TRAITS);
 for (int i = 0; i < traitslist.size() && i < trait_win_size_y; i++) {
  if (traits[traitslist[i]].points > 0)
   status = c_ltgreen;
  else if (traits[traitslist[i]].points < 0)
   status = c_ltred;
  else
   status = c_yellow;
  mvwprintz(w_traits, i+1, 1, status, traits[traitslist[i]].name.c_str());
 }

 wrefresh(w_traits);

// Next, draw effects.
 const char *title_EFFECTS = _("EFFECTS");
 mvwprintz(w_effects, 0, 13 - utf8_width(title_EFFECTS)/2, c_ltgray, title_EFFECTS);
 for (int i = 0; i < effect_name.size() && i < effect_win_size_y; i++) {
  mvwprintz(w_effects, i+1, 1, c_ltgray, effect_name[i].c_str());
 }
 wrefresh(w_effects);

// Next, draw skills.
 line = 1;
 std::vector<Skill*> skillslist;
 const char *title_SKILLS = _("SKILLS");
 mvwprintz(w_skills, 0, 13 - utf8_width(title_SKILLS)/2, c_ltgray, title_SKILLS);

 // sort skills by level
 for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin();
      aSkill != Skill::skills.end(); ++aSkill)
 {
  SkillLevel level = skillLevel(*aSkill);
  if (level < 0)
   continue;
  bool foundplace = false;
  for (std::vector<Skill*>::iterator i = skillslist.begin();
      i != skillslist.end(); ++i)
  {
   SkillLevel thislevel = skillLevel(*i);
   if (thislevel < level)
   {
    skillslist.insert(i, *aSkill);
    foundplace = true;
    break;
   }
  }
  if (!foundplace)
   skillslist.push_back(*aSkill);
 }

 for (std::vector<Skill*>::iterator aSkill = skillslist.begin();
      aSkill != skillslist.end(); ++aSkill)
 {
   SkillLevel level = skillLevel(*aSkill);

   // Default to not training and not rusting
   nc_color text_color = c_blue;
   bool training = level.isTraining();
   bool rusting = level.isRusting(g->turn);

   if(training && rusting)
   {
       text_color = c_ltred;
   }
   else if (training)
   {
       text_color = c_ltblue;
   }
   else if (rusting)
   {
       text_color = c_red;
   }

   if (line < skill_win_size_y + 1)
   {
     mvwprintz(w_skills, line, 1, text_color, "%s", ((*aSkill)->name() + ":").c_str());
     mvwprintz(w_skills, line, 19, text_color, "%-2d(%2d%%%%)", (int)level,
               (level.exercise() <  0 ? 0 : level.exercise()));
     line++;
   }
 }
 wrefresh(w_skills);

// Finally, draw speed.
 const char *title_SPEED = _("SPEED");
 mvwprintz(w_speed, 0, 13 - utf8_width(title_SPEED)/2, c_ltgray, title_SPEED);
 mvwprintz(w_speed, 1,  1, c_ltgray, _("Base Move Cost:"));
 mvwprintz(w_speed, 2,  1, c_ltgray, _("Current Speed:"));
 int newmoves = current_speed(g);
 int pen = 0;
 line = 3;
 if (weight_carried() > int(weight_capacity() * .25)) {
  pen = 75 * double((weight_carried() - int(weight_capacity() * .25)) /
                    (weight_capacity() * .75));
  mvwprintz(w_speed, line, 1, c_red, _("Overburdened        -%s%d%%%%"),
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 pen = int(morale_level() / 25);
 if (abs(pen) >= 4) {
  if (pen > 10)
   pen = 10;
  else if (pen < -10)
   pen = -10;
  if (pen > 0)
   mvwprintz(w_speed, line, 1, c_green, _("Good mood           +%s%d%%%%"),
             (pen < 10 ? " " : ""), pen);
  else
   mvwprintz(w_speed, line, 1, c_red, _("Depressed           -%s%d%%%%"),
             (abs(pen) < 10 ? " " : ""), abs(pen));
  line++;
 }
 pen = int((pain - pkill) * .7);
 if (pen > 60)
  pen = 60;
 if (pen >= 1) {
  mvwprintz(w_speed, line, 1, c_red, _("Pain                -%s%d%%%%"),
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if (pkill >= 10) {
  pen = int(pkill * .1);
  mvwprintz(w_speed, line, 1, c_red, _("Painkillers         -%s%d%%%%"),
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if (stim != 0) {
  pen = stim;
  if (pen > 0)
   mvwprintz(w_speed, line, 1, c_green, _("Stimulants          +%s%d%%%%"),
            (pen < 10 ? " " : ""), pen);
  else
   mvwprintz(w_speed, line, 1, c_red, _("Depressants         -%s%d%%%%"),
            (abs(pen) < 10 ? " " : ""), abs(pen));
  line++;
 }
 if (thirst > 40) {
  pen = int((thirst - 40) / 10);
  mvwprintz(w_speed, line, 1, c_red, _("Thirst              -%s%d%%%%"),
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if (hunger > 100) {
  pen = int((hunger - 100) / 10);
  mvwprintz(w_speed, line, 1, c_red, _("Hunger              -%s%d%%%%"),
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if (has_trait(PF_SUNLIGHT_DEPENDENT) && !g->is_in_sunlight(posx, posy)) {
  pen = (g->light_level() >= 12 ? 5 : 10);
  mvwprintz(w_speed, line, 1, c_red, _("Out of Sunlight     -%s%d%%%%"),
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if ((has_trait(PF_COLDBLOOD) || has_trait(PF_COLDBLOOD2) ||
      has_trait(PF_COLDBLOOD3)) && g->temperature < 65) {
  if (has_trait(PF_COLDBLOOD3))
   pen = int( (65 - g->temperature) / 2);
  else if (has_trait(PF_COLDBLOOD2))
   pen = int( (65 - g->temperature) / 3);
  else
   pen = int( (65 - g->temperature) / 2);
  mvwprintz(w_speed, line, 1, c_red, _("Cold-Blooded        -%s%d%%%%"),
            (pen < 10 ? " " : ""), pen);
  line++;
 }

 for (int i = 0; i < illness.size(); i++) {
  int move_adjust = disease_speed_boost(illness[i]);
  if (move_adjust != 0) {
   nc_color col = (move_adjust > 0 ? c_green : c_red);
   mvwprintz(w_speed, line,  1, col, dis_name(illness[i]).c_str());
   mvwprintz(w_speed, line, 21, col, (move_adjust > 0 ? "+" : "-"));
   move_adjust = abs(move_adjust);
   mvwprintz(w_speed, line, (move_adjust >= 10 ? 22 : 23), col, "%d%%%%",
             move_adjust);
   line++;
  }
 }
 if (has_trait(PF_QUICK)) {
  pen = int(newmoves * .1);
  mvwprintz(w_speed, line, 1, c_green, _("Quick               +%s%d%%%%"),
            (pen < 10 ? " " : ""), pen);
 }
 int runcost = run_cost(100);
 nc_color col = (runcost <= 100 ? c_green : c_red);
 mvwprintz(w_speed, 1, (runcost  >= 100 ? 21 : (runcost  < 10 ? 23 : 22)), col,
           "%d", runcost);
 col = (newmoves >= 100 ? c_green : c_red);
 mvwprintz(w_speed, 2, (newmoves >= 100 ? 21 : (newmoves < 10 ? 23 : 22)), col,
           "%d", newmoves);
 wrefresh(w_speed);

 refresh();
 int curtab = 1;
 int min, max;
 line = 0;
 bool done = false;

// Initial printing is DONE.  Now we give the player a chance to scroll around
// and "hover" over different items for more info.
 do {
  werase(w_info);
  switch (curtab) {
  case 1:	// Stats tab
   mvwprintz(w_stats, 0, 0, h_ltgray, _("                          "));
   mvwprintz(w_stats, 0, 13 - utf8_width(title_STATS)/2, h_ltgray, title_STATS);
   if (line == 0) {
    mvwprintz(w_stats, 2, 2, h_ltgray, _("Strength:"));

// display player current STR effects
    mvwprintz(w_stats, 6, 2, c_magenta, _("Base HP: %d              "),
             hp_max[1]);
    mvwprintz(w_stats, 7, 2, c_magenta, _("Carry weight: %d lbs     "),
             weight_capacity(false) / 4);
    mvwprintz(w_stats, 8, 2, c_magenta, _("Melee damage: %d         "),
             base_damage(false));

    mvwprintz(w_info, 0, 0, c_magenta, _("\
Strength affects your melee damage, the amount of weight you can carry, your\n\
total HP, your resistance to many diseases, and the effectiveness of actions\n\
which require brute force."));
   } else if (line == 1) {
    mvwprintz(w_stats, 3, 2, h_ltgray, _("Dexterity:"));
 // display player current DEX effects
    mvwprintz(w_stats, 6, 2, c_magenta, _("Melee to-hit bonus: +%d                      "),
             base_to_hit(false));
    mvwprintz(w_stats, 7, 2, c_magenta, "                                            ");
    mvwprintz(w_stats, 7, 2, c_magenta, _("Ranged penalty: -%d"),
             abs(ranged_dex_mod(false)));
    mvwprintz(w_stats, 8, 2, c_magenta, "                                            ");
    mvwprintz(w_stats, 8, 2, c_magenta,
             (throw_dex_mod(false) <= 0 ? _("Throwing bonus: %s%d") : _("Throwing penalty: %s%d")),
             (throw_dex_mod(false) <= 0 ? "+" : "-"),
             abs(throw_dex_mod(false)));
    mvwprintz(w_info, 0, 0, c_magenta, _("\
Dexterity affects your chance to hit in melee combat, helps you steady your\n\
gun for ranged combat, and enhances many actions that require finesse."));
   } else if (line == 2) {
    mvwprintz(w_stats, 4, 2, h_ltgray, _("Intelligence:"));
 // display player current INT effects
   mvwprintz(w_stats, 6, 2, c_magenta, _("Read times: %d%%%%           "),
             read_speed(false));
   mvwprintz(w_stats, 7, 2, c_magenta, _("Skill rust: %d%%%%           "),
             rust_rate(false));
   mvwprintz(w_stats, 8, 2, c_magenta, _("Crafting Bonus: %d          "),
             int_cur);

    mvwprintz(w_info, 0, 0, c_magenta, _("\
Intelligence is less important in most situations, but it is vital for more\n\
complex tasks like electronics crafting. It also affects how much skill you\n\
can pick up from reading a book."));
   } else if (line == 3) {
    mvwprintz(w_stats, 5, 2, h_ltgray, _("Perception:"));

       mvwprintz(w_stats, 6, 2,  c_magenta, _("Ranged penalty: -%d"),
             abs(ranged_per_mod(false)),"          ");
    mvwprintz(w_stats, 7, 2, c_magenta, _("Trap dection level: %d       "),
             per_cur);
    mvwprintz(w_stats, 8, 2, c_magenta, "                             ");
    mvwprintz(w_info, 0, 0, c_magenta, _("\
Perception is the most important stat for ranged combat. It's also used for\n\
detecting traps and other things of interest."));
   }
   wrefresh(w_stats);
   wrefresh(w_info);
   switch (input()) {
    case 'j':
     line++;
     if (line == 4)
      line = 0;
     break;
    case 'k':
     line--;
     if (line == -1)
      line = 3;
     break;
    case '\t':
     mvwprintz(w_stats, 0, 0, c_ltgray, _("                          "));
     mvwprintz(w_stats, 0, 13 - utf8_width(title_STATS)/2, c_ltgray, title_STATS);
     wrefresh(w_stats);
     line = 0;
     curtab++;
     break;
    case 'q':
    case KEY_ESCAPE:
     done = true;
   }
   mvwprintz(w_stats, 2, 2, c_ltgray, _("Strength:"));
   mvwprintz(w_stats, 3, 2, c_ltgray, _("Dexterity:"));
   mvwprintz(w_stats, 4, 2, c_ltgray, _("Intelligence:"));
   mvwprintz(w_stats, 5, 2, c_ltgray, _("Perception:"));
   wrefresh(w_stats);
   break;
  case 2:	// Encumberment tab
   mvwprintz(w_encumb, 0, 0, h_ltgray,  _("                          "));
   mvwprintz(w_encumb, 0, 13 - utf8_width(title_ENCUMB)/2, h_ltgray, title_ENCUMB);
   if (line == 0) {
    mvwprintz(w_encumb, 1, 1, h_ltgray, _("Torso"));
    mvwprintz(w_info, 0, 0, c_magenta, _("\
Melee skill %+d;      Dodge skill %+d;\n\
Swimming costs %+d movement points;\n\
Melee attacks cost %+d movement points"), -encumb(bp_torso), -encumb(bp_torso),
              encumb(bp_torso) * (80 - skillLevel("swimming") * 3), encumb(bp_torso) * 20);
   } else if (line == 1) {
    mvwprintz(w_encumb, 2, 1, h_ltgray, _("Head"));
    mvwprintz(w_info, 0, 0, c_magenta, _("\
Head encumberance has no effect; it simply limits how much you can put on."));
   } else if (line == 2) {
    mvwprintz(w_encumb, 3, 1, h_ltgray, _("Eyes"));
    mvwprintz(w_info, 0, 0, c_magenta, _("\
Perception %+d when checking traps or firing ranged weapons;\n\
Perception %+.1f when throwing items"), -encumb(bp_eyes),
double(double(-encumb(bp_eyes)) / 2));
   } else if (line == 3) {
    mvwprintz(w_encumb, 4, 1, h_ltgray, _("Mouth"));
    mvwprintz(w_info, 0, 0, c_magenta, _("\
Running costs %+d movement points"), encumb(bp_mouth) * 5);
   } else if (line == 4)
  {
    mvwprintz(w_encumb, 5, 1, h_ltgray, _("Arms"));
    mvwprintz(w_info, 0, 0, c_magenta, _("\
Arm encumbrance affects your accuracy with ranged weapons."));
   } else if (line == 5)
   {
    mvwprintz(w_encumb, 6, 1, h_ltgray, _("Hands"));
    mvwprintz(w_info, 0, 0, c_magenta, _("\
Reloading costs %+d movement points;\n\
Dexterity %+d when throwing items"), encumb(bp_hands) * 30, -encumb(bp_hands));
   } else if (line == 6) {
    mvwprintz(w_encumb, 7, 1, h_ltgray, _("Legs"));
    mvwprintz(w_info, 0, 0, c_magenta, _("\
Running costs %+d movement points;  Swimming costs %+d movement points;\n\
Dodge skill %+.1f"), encumb(bp_legs) * 3,
              encumb(bp_legs) *(50 - skillLevel("swimming") * 2),
                     double(double(-encumb(bp_legs)) / 2));
   } else if (line == 7) {
    mvwprintz(w_encumb, 8, 1, h_ltgray, _("Feet"));
    mvwprintz(w_info, 0, 0, c_magenta, _("\
Running costs %+d movement points"), encumb(bp_feet) * 5);
   }
   wrefresh(w_encumb);
   wrefresh(w_info);
   switch (input()) {
    case 'j':
     line++;
     if (line == 8)
      line = 0;
     break;
    case 'k':
     line--;
     if (line == -1)
      line = 7;
     break;
    case '\t':
     mvwprintz(w_encumb, 0, 0, c_ltgray,  _("                          "));
     mvwprintz(w_encumb, 0, 13 - utf8_width(title_ENCUMB)/2, c_ltgray, title_ENCUMB);
     wrefresh(w_encumb);
     line = 0;
     curtab++;
     break;
    case 'q':
    case KEY_ESCAPE:
     done = true;
   }
   mvwprintz(w_encumb, 1, 1, c_ltgray, _("Torso"));
   mvwprintz(w_encumb, 2, 1, c_ltgray, _("Head"));
   mvwprintz(w_encumb, 3, 1, c_ltgray, _("Eyes"));
   mvwprintz(w_encumb, 4, 1, c_ltgray, _("Mouth"));
   mvwprintz(w_encumb, 5, 1, c_ltgray, _("Arms"));
   mvwprintz(w_encumb, 6, 1, c_ltgray, _("Hands"));
   mvwprintz(w_encumb, 7, 1, c_ltgray, _("Legs"));
   mvwprintz(w_encumb, 8, 1, c_ltgray, _("Feet"));
   wrefresh(w_encumb);
   break;
  case 4:	// Traits tab
   mvwprintz(w_traits, 0, 0, h_ltgray,  _("                          "));
   mvwprintz(w_traits, 0, 13 - utf8_width(title_TRAITS)/2, h_ltgray, title_TRAITS);
   if (line <= (trait_win_size_y-1)/2) {
    min = 0;
    max = trait_win_size_y;
    if (traitslist.size() < max)
     max = traitslist.size();
   } else if (line >= traitslist.size() - (trait_win_size_y+1)/2) {
    min = (traitslist.size() < trait_win_size_y ? 0 : traitslist.size() - trait_win_size_y);
    max = traitslist.size();
   } else {
    min = line - (trait_win_size_y-1)/2;
    max = line + (trait_win_size_y+1)/2;
    if (traitslist.size() < max)
     max = traitslist.size();
    if (min < 0)
     min = 0;
   }

   for (int i = min; i < max; i++) {
    mvwprintz(w_traits, 1 + i - min, 1, c_ltgray, "                         ");
    if (traitslist[i] > PF_MAX2)
     status = c_ltblue;
    else if (traits[traitslist[i]].points > 0)
     status = c_ltgreen;
    else if (traits[traitslist[i]].points < 0)
     status = c_ltred;
    else
     status = c_yellow;
    if (i == line)
     mvwprintz(w_traits, 1 + i - min, 1, hilite(status),
               traits[traitslist[i]].name.c_str());
    else
     mvwprintz(w_traits, 1 + i - min, 1, status,
               traits[traitslist[i]].name.c_str());
   }
   if (line >= 0 && line < traitslist.size()) {
     fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH-2, c_magenta, "%s", traits[traitslist[line]].description.c_str());
   }
   wrefresh(w_traits);
   wrefresh(w_info);
   switch (input()) {
    case 'j':
     if (line < traitslist.size() - 1)
      line++;
     break;
    case 'k':
     if (line > 0)
      line--;
     break;
    case '\t':
     mvwprintz(w_traits, 0, 0, c_ltgray,  _("                          "));
     mvwprintz(w_traits, 0, 13 - utf8_width(title_TRAITS)/2, c_ltgray, title_TRAITS);
     for (int i = 0; i < traitslist.size() && i < trait_win_size_y; i++) {
      mvwprintz(w_traits, i + 1, 1, c_black, "                         ");
      if (traits[traitslist[i]].points > 0)
       status = c_ltgreen;
      else if (traits[traitslist[i]].points < 0)
       status = c_ltred;
      else
       status = c_yellow;
      mvwprintz(w_traits, i + 1, 1, status, traits[traitslist[i]].name.c_str());
     }
     wrefresh(w_traits);
     line = 0;
     curtab++;
     break;
    case 'q':
    case KEY_ESCAPE:
     done = true;
   }
   break;

  case 5:	// Effects tab
   mvwprintz(w_effects, 0, 0, h_ltgray,  _("                          "));
   mvwprintz(w_effects, 0, 13 - utf8_width(title_EFFECTS)/2, h_ltgray, title_EFFECTS);
   if (line <= (effect_win_size_y-1)/2) {
    min = 0;
    max = effect_win_size_y;
    if (effect_name.size() < max)
     max = effect_name.size();
   } else if (line >= effect_name.size() - (effect_win_size_y+1)/2) {
    min = (effect_name.size() < effect_win_size_y ? 0 : effect_name.size() - effect_win_size_y);
    max = effect_name.size();
   } else {
    min = line - (effect_win_size_y-1)/2;
    max = line + (effect_win_size_y+1)/2;
    if (effect_name.size() < max)
     max = effect_name.size();
    if (min < 0)
     min = 0;
   }

   for (int i = min; i < max; i++) {
    if (i == line)
     mvwprintz(w_effects, 1 + i - min, 1, h_ltgray, effect_name[i].c_str());
    else
     mvwprintz(w_effects, 1 + i - min, 1, c_ltgray, effect_name[i].c_str());
   }
   if (line >= 0 && line < effect_text.size()) {
    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH-2, c_magenta, "%s", effect_text[line].c_str());
   }
   wrefresh(w_effects);
   wrefresh(w_info);
   switch (input()) {
    case 'j':
     if (line < effect_name.size() - 1)
      line++;
     break;
    case 'k':
     if (line > 0)
      line--;
     break;
    case '\t':
     mvwprintz(w_effects, 0, 0, c_ltgray,  _("                          "));
     mvwprintz(w_effects, 0, 13 - utf8_width(title_EFFECTS)/2, c_ltgray, title_EFFECTS);
     for (int i = 0; i < effect_name.size() && i < 7; i++)
      mvwprintz(w_effects, i + 1, 1, c_ltgray, effect_name[i].c_str());
     wrefresh(w_effects);
     line = 0;
     curtab = 1;
     break;
    case 'q':
    case KEY_ESCAPE:
     done = true;
   }
   break;

  case 3:	// Skills tab
   mvwprintz(w_skills, 0, 0, h_ltgray,  _("                          "));
   mvwprintz(w_skills, 0, 13 - utf8_width(title_SKILLS)/2, h_ltgray, title_SKILLS);
   if (line <= (skill_win_size_y-1)/2) {
    min = 0;
    max = skill_win_size_y;
    if (skillslist.size() < max)
     max = skillslist.size();
   } else if (line >= skillslist.size() - (skill_win_size_y+1)/2) {
    min = (skillslist.size() < skill_win_size_y ? 0 : skillslist.size() - skill_win_size_y);
    max = skillslist.size();
   } else {
    min = line - (skill_win_size_y-1)/2;
    max = line + (skill_win_size_y+1)/2;
    if (skillslist.size() < max)
     max = skillslist.size();
    if (min < 0)
     min = 0;
   }

   Skill *selectedSkill;

   for (int i = min; i < max; i++)
   {
    Skill *aSkill = skillslist[i];
    SkillLevel level = skillLevel(aSkill);

    bool isLearning = level.isTraining();
    int exercise = level.exercise();
    bool rusting = level.isRusting(g->turn);

    if (i == line) {
      selectedSkill = aSkill;
     if (exercise >= 100)
      status = isLearning ? h_pink : h_magenta;
     else if (rusting)
      status = isLearning ? h_ltred : h_red;
     else
      status = isLearning ? h_ltblue : h_blue;
    } else {
     if (rusting)
      status = isLearning ? c_ltred : c_red;
     else
      status = isLearning ? c_ltblue : c_blue;
    }
    mvwprintz(w_skills, 1 + i - min, 1, c_ltgray, "                         ");
    mvwprintz(w_skills, 1 + i - min, 1, status, "%s:", aSkill->name().c_str());
    mvwprintz(w_skills, 1 + i - min,19, status, "%-2d(%2d%%%%)", (int)level, (exercise <  0 ? 0 : exercise));
   }
   werase(w_info);
   if (line >= 0 && line < skillslist.size()) {
    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH-2, c_magenta, "%s", selectedSkill->description().c_str());
   }
   wrefresh(w_skills);
   wrefresh(w_info);
   switch (input()) {
    case 'j':
     if (line < skillslist.size() - 1)
      line++;
     break;
    case 'k':
     if (line > 0)
      line--;
     break;
    case '\t':
      werase(w_skills);
     mvwprintz(w_skills, 0, 0, c_ltgray,  _("                          "));
     mvwprintz(w_skills, 0, 13 - utf8_width(title_SKILLS)/2, c_ltgray, title_SKILLS);
     for (int i = 0; i < skillslist.size() && i < skill_win_size_y; i++) {
      Skill *thisSkill = skillslist[i];
      SkillLevel level = skillLevel(thisSkill);
      bool isLearning = level.isTraining();
      bool rusting = level.isRusting(g->turn);

      if (rusting)
       status = isLearning ? c_ltred : c_red;
      else
       status = isLearning ? c_ltblue : c_blue;

      mvwprintz(w_skills, i + 1,  1, status, "%s:", thisSkill->name().c_str());
      mvwprintz(w_skills, i + 1, 19, status, "%d (%2d%%%%)", (int)level, (level.exercise() <  0 ? 0 : level.exercise()));
     }
     wrefresh(w_skills);
     line = 0;
     curtab++;
     break;
   case ' ':
     skillLevel(selectedSkill).toggleTraining();
     break;
    case 'q':
    case 'Q':
    case KEY_ESCAPE:
     done = true;
   }
  }
 } while (!done);

 werase(w_info);
 werase(w_tip);
 werase(w_stats);
 werase(w_encumb);
 werase(w_traits);
 werase(w_effects);
 werase(w_skills);
 werase(w_speed);
 werase(w_info);
 werase(w_grid_top);
 werase(w_grid_effect);
 werase(w_grid_skill);
 werase(w_grid_trait);

 delwin(w_info);
 delwin(w_tip);
 delwin(w_stats);
 delwin(w_encumb);
 delwin(w_traits);
 delwin(w_effects);
 delwin(w_skills);
 delwin(w_speed);
 delwin(w_grid_top);
 delwin(w_grid_effect);
 delwin(w_grid_skill);
 delwin(w_grid_trait);
 erase();
}

void player::disp_morale(game *g)
{
    // Ensure the player's persistent morale effects are up-to-date.
    apply_persistent_morale();

    // Create and draw the window itself.
    WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                        (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0,
                        (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);
    wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

    // Figure out how wide the name column needs to be.
    int name_column_width = 18;
    for (int i = 0; i < morale.size(); i++)
    {
        int length = morale[i].name(morale_data).length();
        if ( length > name_column_width)
        {
            name_column_width = length;
        }
    }

    // If it's too wide, truncate.
    if (name_column_width > 72)
    {
        name_column_width = 72;
    }

    // Start printing the number right after the name column.
    // We'll right-justify it later.
    int number_pos = name_column_width + 1;

    // Header
    mvwprintz(w, 1,  1, c_white, _("Morale Modifiers:"));
    mvwprintz(w, 2,  1, c_ltgray, _("Name"));
    mvwprintz(w, 2, name_column_width+2, c_ltgray, _("Value"));

    // Print out the morale entries.
    for (int i = 0; i < morale.size(); i++)
    {
        std::string name = morale[i].name(morale_data);
        int bonus = net_morale(morale[i]);

        // Trim the name if need be.
        if (name.length() > name_column_width)
        {
            name = name.erase(name_column_width-3, std::string::npos) + "...";
        }

        // Print out the name.
        mvwprintz(w, i + 3,  1, (bonus < 0 ? c_red : c_green), name.c_str());

        // Print out the number, right-justified.
        mvwprintz(w, i + 3, number_pos, (bonus < 0 ? c_red : c_green),
                  "% 6d", bonus);
    }

    // Print out the total morale, right-justified.
    int mor = morale_level();
    mvwprintz(w, 20, 1, (mor < 0 ? c_red : c_green), _("Total:"));
    mvwprintz(w, 20, number_pos, (mor < 0 ? c_red : c_green), "% 6d", mor);

    // Print out the focus gain rate, right-justified.
    double gain = (calc_focus_equilibrium() - focus_pool) / 100.0;
    mvwprintz(w, 22, 1, (gain < 0 ? c_red : c_green), "Focus gain:");
    mvwprintz(w, 22, number_pos-3, (gain < 0 ? c_red : c_green), "% 6.2f per minute", gain);

    // Make sure the changes are shown.
    wrefresh(w);

    // Wait for any keystroke.
    getch();

    // Close the window.
    werase(w);
    delwin(w);
}

void player::disp_status(WINDOW *w, game *g)
{
 mvwprintz(w, 0, 0, c_ltgray, _("Weapon: %s"), weapname().c_str());
 if (weapon.is_gun()) {
   int adj_recoil = recoil + driving_recoil;
       if (adj_recoil >= 36)
   mvwprintz(w, 0, 34, c_red,    _("Recoil"));
  else if (adj_recoil >= 20)
   mvwprintz(w, 0, 34, c_ltred,  _("Recoil"));
  else if (adj_recoil >= 4)
   mvwprintz(w, 0, 34, c_yellow, _("Recoil"));
  else if (adj_recoil > 0)
   mvwprintz(w, 0, 34, c_ltgray, _("Recoil"));
 }

 // Print the current weapon mode
 if (weapon.mode == "NULL")
  mvwprintz(w, 1, 0, c_red,    _("Normal"));
 else if (weapon.mode == "MODE_BURST")
  mvwprintz(w, 1, 0, c_red,    _("Burst"));
 else {
  item* gunmod = weapon.active_gunmod();
  if (gunmod != NULL)
   mvwprintz(w, 1, 0, c_red, gunmod->type->name.c_str());
 }

 if (hunger > 2800)
  mvwprintz(w, 2, 0, c_red,    _("Starving!"));
 else if (hunger > 1400)
  mvwprintz(w, 2, 0, c_ltred,  _("Near starving"));
 else if (hunger > 300)
  mvwprintz(w, 2, 0, c_ltred,  _("Famished"));
 else if (hunger > 100)
  mvwprintz(w, 2, 0, c_yellow, _("Very hungry"));
 else if (hunger > 40)
  mvwprintz(w, 2, 0, c_yellow, _("Hungry"));
 else if (hunger < 0)
  mvwprintz(w, 2, 0, c_green,  _("Full"));

 // Find hottest/coldest bodypart
 int min = 0, max = 0;
 for (int i = 0; i < num_bp ; i++ ){
  if      (temp_cur[i] > BODYTEMP_HOT  && temp_cur[i] > temp_cur[max]) max = i;
  else if (temp_cur[i] < BODYTEMP_COLD && temp_cur[i] < temp_cur[min]) min = i;
 }
 // Compare which is most extreme
 int print;
 if (temp_cur[max] - BODYTEMP_NORM > BODYTEMP_NORM + temp_cur[min]) print = max;
 else print = min;
 // Assign zones to temp_cur and temp_conv for comparison
 int cur_zone = 0;
 if      (temp_cur[print] >  BODYTEMP_SCORCHING) cur_zone = 7;
 else if (temp_cur[print] >  BODYTEMP_VERY_HOT)  cur_zone = 6;
 else if (temp_cur[print] >  BODYTEMP_HOT)       cur_zone = 5;
 else if (temp_cur[print] >  BODYTEMP_COLD)      cur_zone = 4;
 else if (temp_cur[print] >  BODYTEMP_VERY_COLD) cur_zone = 3;
 else if (temp_cur[print] >  BODYTEMP_FREEZING)  cur_zone = 2;
 else if (temp_cur[print] <= BODYTEMP_FREEZING)  cur_zone = 1;
 int conv_zone = 0;
 if      (temp_conv[print] >  BODYTEMP_SCORCHING) conv_zone = 7;
 else if (temp_conv[print] >  BODYTEMP_VERY_HOT)  conv_zone = 6;
 else if (temp_conv[print] >  BODYTEMP_HOT)       conv_zone = 5;
 else if (temp_conv[print] >  BODYTEMP_COLD)      conv_zone = 4;
 else if (temp_conv[print] >  BODYTEMP_VERY_COLD) conv_zone = 3;
 else if (temp_conv[print] >  BODYTEMP_FREEZING)  conv_zone = 2;
 else if (temp_conv[print] <= BODYTEMP_FREEZING)  conv_zone = 1;
 // delta will be positive if temp_cur is rising
 int delta = conv_zone - cur_zone;
 // Decide if temp_cur is rising or falling
 const char *temp_message = "Error";
 if      (delta >   2) temp_message = _(" (Rising!!)");
 else if (delta ==  2) temp_message = _(" (Rising!)");
 else if (delta ==  1) temp_message = _(" (Rising)");
 else if (delta ==  0) temp_message = "";
 else if (delta == -1) temp_message = _(" (Falling)");
 else if (delta == -2) temp_message = _(" (Falling!)");
 else if (delta <  -2) temp_message = _(" (Falling!!)");
 // Print the hottest/coldest bodypart, and if it is rising or falling in temperature
 if      (temp_cur[print] >  BODYTEMP_SCORCHING)
  mvwprintz(w, 1, 9, c_red,   _("Scorching!%s"), temp_message);
 else if (temp_cur[print] >  BODYTEMP_VERY_HOT)
  mvwprintz(w, 1, 9, c_ltred, _("Very hot!%s"), temp_message);
 else if (temp_cur[print] >  BODYTEMP_HOT)
  mvwprintz(w, 1, 9, c_yellow,_("Warm%s"), temp_message);
 else if (temp_cur[print] >  BODYTEMP_COLD) // If you're warmer than cold, you are comfortable
  mvwprintz(w, 1, 9, c_green, _("Comfortable%s"), temp_message);
 else if (temp_cur[print] >  BODYTEMP_VERY_COLD)
  mvwprintz(w, 1, 9, c_ltblue,_("Chilly%s"), temp_message);
 else if (temp_cur[print] >  BODYTEMP_FREEZING)
  mvwprintz(w, 1, 9, c_cyan,  _("Very cold!%s"), temp_message);
 else if (temp_cur[print] <= BODYTEMP_FREEZING)
  mvwprintz(w, 1, 9, c_blue,  _("Freezing!%s"), temp_message);

 mvwprintz(w, 1, 32, c_yellow, _("Sound:%d"), volume);
 volume = 0;

      if (thirst > 520)
  mvwprintz(w, 2, 15, c_ltred,  _("Parched"));
 else if (thirst > 240)
  mvwprintz(w, 2, 15, c_ltred,  _("Dehydrated"));
 else if (thirst > 80)
  mvwprintz(w, 2, 15, c_yellow, _("Very thirsty"));
 else if (thirst > 40)
  mvwprintz(w, 2, 15, c_yellow, _("Thirsty"));
 else if (thirst < 0)
  mvwprintz(w, 2, 15, c_green,  _("Slaked"));

      if (fatigue > 575)
  mvwprintz(w, 2, 30, c_red,    _("Exhausted"));
 else if (fatigue > 383)
  mvwprintz(w, 2, 30, c_ltred,  _("Dead tired"));
 else if (fatigue > 191)
  mvwprintz(w, 2, 30, c_yellow, _("Tired"));

 mvwprintz(w, 2, 41, c_white, _("Focus: "));
 nc_color col_xp = c_dkgray;
 if (focus_pool >= 100)
  col_xp = c_white;
 else if (focus_pool >  0)
  col_xp = c_ltgray;
 mvwprintz(w, 2, 48, col_xp, "%d", focus_pool);

 nc_color col_pain = c_yellow;
 if (pain - pkill >= 60)
  col_pain = c_red;
 else if (pain - pkill >= 40)
  col_pain = c_ltred;
 if (pain - pkill > 0)
  mvwprintz(w, 3, 0, col_pain, _("Pain: %d"), pain - pkill);

 vehicle *veh = g->m.veh_at (posx, posy);
 int dmor = 0;

 int morale_cur = morale_level ();
 nc_color col_morale = c_white;
 if (morale_cur >= 10)
  col_morale = c_green;
 else if (morale_cur <= -10)
  col_morale = c_red;
 if (morale_cur >= 100)
  mvwprintz(w, 3, 10 + dmor, col_morale, ":D");
 else if (morale_cur >= 10)
  mvwprintz(w, 3, 10 + dmor, col_morale, ":)");
 else if (morale_cur > -10)
  mvwprintz(w, 3, 10 + dmor, col_morale, ":|");
 else if (morale_cur > -100)
  mvwprintz(w, 3, 10 + dmor, col_morale, "):");
 else
  mvwprintz(w, 3, 10 + dmor, col_morale, "D:");

 if (in_vehicle && veh) {
  veh->print_fuel_indicator (w, 3, 49);
  nc_color col_indf1 = c_ltgray;

  float strain = veh->strain();
  nc_color col_vel = strain <= 0? c_ltblue :
                     (strain <= 0.2? c_yellow :
                     (strain <= 0.4? c_ltred : c_red));

  bool has_turrets = false;
  for (int p = 0; p < veh->parts.size(); p++) {
   if (veh->part_flag (p, vpf_turret)) {
    has_turrets = true;
    break;
   }
  }

  if (has_turrets) {
   mvwprintz(w, 3, 25, col_indf1, "Gun:");
   mvwprintz(w, 3, 29, veh->turret_mode ? c_ltred : c_ltblue,
                       veh->turret_mode ? "auto" : "off ");
  }

  if (veh->cruise_on) {
   if(OPTIONS[OPT_USE_METRIC_SYS]) {
    mvwprintz(w, 3, 33, col_indf1, "{Km/h....>....}");
    mvwprintz(w, 3, 38, col_vel, "%4d", int(veh->velocity * 0.0161f));
    mvwprintz(w, 3, 43, c_ltgreen, "%4d", int(veh->cruise_velocity * 0.0161f));
   }
   else {
    mvwprintz(w, 3, 34, col_indf1, "{mph....>....}");
    mvwprintz(w, 3, 38, col_vel, "%4d", veh->velocity / 100);
    mvwprintz(w, 3, 43, c_ltgreen, "%4d", veh->cruise_velocity / 100);
   }
  } else {
   if(OPTIONS[OPT_USE_METRIC_SYS]) {
    mvwprintz(w, 3, 33, col_indf1, "  {Km/h....}  ");
    mvwprintz(w, 3, 40, col_vel, "%4d", int(veh->velocity * 0.0161f));
   }
   else {
    mvwprintz(w, 3, 34, col_indf1, "  {mph....}  ");
    mvwprintz(w, 3, 40, col_vel, "%4d", veh->velocity / 100);
   }
  }

  if (veh->velocity != 0) {
   nc_color col_indc = veh->skidding? c_red : c_green;
   int dfm = veh->face.dir() - veh->move.dir();
   mvwprintz(w, 3, 21, col_indc, dfm < 0? "L" : ".");
   mvwprintz(w, 3, 22, col_indc, dfm == 0? "0" : ".");
   mvwprintz(w, 3, 23, col_indc, dfm > 0? "R" : ".");
  }
 } else {  // Not in vehicle
  nc_color col_str = c_white, col_dex = c_white, col_int = c_white,
           col_per = c_white, col_spd = c_white;
  if (str_cur < str_max)
   col_str = c_red;
  if (str_cur > str_max)
   col_str = c_green;
  if (dex_cur < dex_max)
   col_dex = c_red;
  if (dex_cur > dex_max)
   col_dex = c_green;
  if (int_cur < int_max)
   col_int = c_red;
  if (int_cur > int_max)
   col_int = c_green;
  if (per_cur < per_max)
   col_per = c_red;
  if (per_cur > per_max)
   col_per = c_green;
  int spd_cur = current_speed();
  if (current_speed() < 100)
   col_spd = c_red;
  if (current_speed() > 100)
   col_spd = c_green;

  mvwprintz(w, 3, 13, col_str, _("Str %s%d"), str_cur >= 10 ? "" : " ", str_cur);
  mvwprintz(w, 3, 20, col_dex, _("Dex %s%d"), dex_cur >= 10 ? "" : " ", dex_cur);
  mvwprintz(w, 3, 27, col_int, _("Int %s%d"), int_cur >= 10 ? "" : " ", int_cur);
  mvwprintz(w, 3, 34, col_per, _("Per %s%d"), per_cur >= 10 ? "" : " ", per_cur);
  mvwprintz(w, 3, 41, col_spd, _("Spd %s%d"), spd_cur >= 10 ? "" : " ", spd_cur);
  mvwprintz(w, 3, 50, c_white, "%d", movecounter);
 }
}

bool player::has_trait(int flag) const
{
 if (flag == PF_NULL)
  return true;
 return my_mutations[flag]; //Looks for active mutations and traits
}

bool player::has_base_trait(int flag) const
{
 if (flag == PF_NULL)
  return true;
 return my_traits[flag]; //Looks only at base traits
}

void player::toggle_trait(int flag)
{
 my_traits[flag] = !my_traits[flag]; //Toggles a base trait on the player
 my_mutations[flag] = !my_mutations[flag]; //Toggles corresponding trait in mutations list as well.
}

void player::toggle_mutation(int flag)
{
 my_mutations[flag] = !my_mutations[flag]; //Toggles a mutation on the player
}

bool player::in_climate_control(game *g)
{
    bool regulated_area=false;
    // Check
    if(has_active_bionic("bio_climate")) { return true; }
    for (int i = 0; i < worn.size(); i++)
    {
        if ((dynamic_cast<it_armor*>(worn[i].type))->is_power_armor() &&
            (has_active_item("UPS_on") || has_active_item("adv_UPS_on") || has_active_bionic("bio_power_armor_interface") || has_active_bionic("bio_power_armor_interface_mkII")))
        {
            return true;
        }
    }
    if(int(g->turn) >= next_climate_control_check)
    {
        next_climate_control_check=int(g->turn)+20;  // save cpu and similate acclimation.
        int vpart = -1;
        vehicle *veh = g->m.veh_at(posx, posy, vpart);
        if(veh)
        {
            regulated_area=(
                veh->is_inside(vpart) &&    // Already checks for opened doors
                veh->total_power(true) > 0  // Out of gas? No AC for you!
            );  // TODO: (?) Force player to scrounge together an AC unit
        }
        // TODO: AC check for when building power is implmented
        last_climate_control_ret=regulated_area;
        if(!regulated_area) { next_climate_control_check+=40; }  // Takes longer to cool down / warm up with AC, than it does to step outside and feel cruddy.
    }
    else
    {
        return ( last_climate_control_ret ? true : false );
    }
    return regulated_area;
}

bool player::has_bionic(bionic_id b) const
{
 for (int i = 0; i < my_bionics.size(); i++) {
  if (my_bionics[i].id == b)
   return true;
 }
 return false;
}

bool player::has_active_bionic(bionic_id b) const
{
 for (int i = 0; i < my_bionics.size(); i++) {
  if (my_bionics[i].id == b)
   return (my_bionics[i].powered);
 }
 return false;
}

void player::add_bionic(bionic_id b)
{
 for (int i = 0; i < my_bionics.size(); i++) {
  if (my_bionics[i].id == b)
   return;	// No duplicates!
 }
 char newinv;
 if (my_bionics.size() == 0)
  newinv = 'a';
 else
  newinv = my_bionics[my_bionics.size() - 1].invlet + 1;
 my_bionics.push_back(bionic(b, newinv));
}

void player::charge_power(int amount)
{
 power_level += amount;
 if (power_level > max_power_level)
  power_level = max_power_level;
 if (power_level < 0)
  power_level = 0;
}

float player::active_light()
{
    float lumination = 0;

    int flashlight = active_item_charges("flashlight_on");
    int torch = active_item_charges("torch_lit");
    int gasoline_lantern = active_item_charges("gasoline_lantern_on");
    if (flashlight > 0)
    {
        lumination = std::min(100, flashlight * 5);    // Will do for now
    }
    else if (torch > 0)
    {
        lumination = std::min(100, torch * 5);
    }
    else if (active_item_charges("pda_flashlight") > 0)
    {
        lumination = 6;
    }
    else if (gasoline_lantern > 0)
    {
        lumination = 5;
    }
    else if (has_active_bionic("bio_flashlight"))
    {
        lumination = 60;
    }
    else if (has_artifact_with(AEP_GLOW))
    {
        lumination = 25;
    }

    return lumination;
}

int player::sight_range(int light_level)
{
 int ret = light_level;
 if ( has_nv() && ret < 12)
  ret = 12;
 if (has_trait(PF_NIGHTVISION) && ret < 12)
  ret += 1;
 if (has_trait(PF_NIGHTVISION2) && ret < 12)
  ret += 4;
 if (has_trait(PF_NIGHTVISION3) && ret < 12)
  ret = 12;
 if (underwater && !has_bionic("bio_membrane") && !has_trait(PF_MEMBRANE) &&
     !is_wearing("goggles_swim"))
  ret = 1;
 if (has_disease("boomered"))
  ret = 1;
 if (has_disease("in_pit"))
  ret = 1;
 if (has_disease("blind"))
  ret = 0;
 if (ret > 4 && has_trait(PF_MYOPIC) && !is_wearing("glasses_eye") &&
     !is_wearing("glasses_monocle") && !is_wearing("glasses_bifocal"))
  ret = 4;
 return ret;
}

int player::unimpaired_range()
{
 int ret = DAYLIGHT_LEVEL;
 if (has_disease("in_pit"))
  ret = 1;
 if (has_disease("blind"))
  ret = 0;
 return ret;
}

int player::overmap_sight_range(int light_level)
{
 int sight = sight_range(light_level);
 if (sight < SEEX)
  return 0;
 if (sight <= SEEX * 4)
  return (sight / (SEEX / 2));
 if (has_amount("binoculars", 1))
  return 20;

 return 10;
}

int player::clairvoyance()
{
 if (has_artifact_with(AEP_CLAIRVOYANCE))
  return 3;
 if (has_artifact_with(AEP_SUPER_CLAIRVOYANCE))
  return 40;
 return 0;
}

bool player::sight_impaired()
{
 return has_disease("boomered") ||
  (underwater && !has_bionic("bio_membrane") && !has_trait(PF_MEMBRANE)
              && !is_wearing("goggles_swim")) ||
  (has_trait(PF_MYOPIC) && !is_wearing("glasses_eye")
                        && !is_wearing("glasses_monocle")
                        && !is_wearing("glasses_bifocal"));
}

bool player::has_two_arms() const
{
 if (has_bionic("bio_blaster") || hp_cur[hp_arm_l] < 10 || hp_cur[hp_arm_r] < 10)
  return false;
 return true;
}

bool player::avoid_trap(trap* tr)
{
  int myroll = dice(3, dex_cur + skillLevel("dodge") * 1.5);
 int traproll;
 if (per_cur - encumb(bp_eyes) >= tr->visibility)
  traproll = dice(3, tr->avoidance);
 else
  traproll = dice(6, tr->avoidance);
 if (has_trait(PF_LIGHTSTEP))
  myroll += dice(2, 6);
 if (myroll >= traproll)
  return true;
 return false;
}

bool player::has_nv()
{
    static bool nv = false;

    if( !nv_cached ) {
        nv_cached = true;
        nv = ((is_wearing("goggles_nv") && (has_active_item("UPS_on") ||
                                            has_active_item("adv_UPS_on"))) ||
              has_active_bionic("bio_night_vision"));
    }

    return nv;
}

void player::pause(game *g)
{
 moves = 0;
 if (recoil > 0) {
   if (str_cur + 2 * skillLevel("gun") >= recoil)
   recoil = 0;
  else {
    recoil -= str_cur + 2 * skillLevel("gun");
   recoil = int(recoil / 2);
  }
 }

// Meditation boost for Toad Style
 if (weapon.type->id == "style_toad" && activity.type == ACT_NULL) {
  int arm_amount = 1 + (int_cur - 6) / 3 + (per_cur - 6) / 3;
  int arm_max = (int_cur + per_cur) / 2;
  if (arm_amount > 3)
   arm_amount = 3;
  if (arm_max > 20)
   arm_max = 20;
  add_disease("armor_boost", 2, arm_amount, arm_max);
 }
}

int player::throw_range(signed char ch)
{
 item tmp;
 if (ch == -1)
  tmp = weapon;
 else if (ch == -2)
  return -1;
 else
  tmp = inv.item_by_letter(ch);

 if (tmp.weight() > int(str_cur * 15))
  return 0;
 int ret = int((str_cur * 8) / (tmp.weight() > 0 ? tmp.weight() : 10));
 ret -= int(tmp.volume() / 10);
 if (has_active_bionic("bio_railgun") && (tmp.made_of("iron") || tmp.made_of("steel")))
    ret *= 2;
 if (ret < 1)
  return 1;
// Cap at double our strength + skill
 if (ret > str_cur * 1.5 + skillLevel("throw"))
   return str_cur * 1.5 + skillLevel("throw");
 return ret;
}

int player::ranged_dex_mod(bool real_life)
{
    const int dex = (real_life ? dex_cur : dex_max);

    if (dex >= 12) { return 0; }
    return 12 - dex;
}

int player::ranged_per_mod(bool real_life)
{
 const int per = (real_life ? per_cur : per_max);

 if (per >= 12) { return 0; }
 return 12 - per;
}

int player::throw_dex_mod(bool real_life)
{
 int dex = (real_life ? dex_cur : dex_max);
 if (dex == 8 || dex == 9)
  return 0;
 if (dex >= 10)
  return (real_life ? 0 - rng(0, dex - 9) : 9 - dex);

 int deviation = 0;
 if (dex < 4)
  deviation = 4 * (8 - dex);
 else if (dex < 6)
  deviation = 3 * (8 - dex);
 else
  deviation = 2 * (8 - dex);

 return (real_life ? rng(0, deviation) : deviation);
}

int player::read_speed(bool real_life)
{
 int intel = (real_life ? int_cur : int_max);
 int ret = 1000 - 50 * (intel - 8);
 if (has_trait(PF_FASTREADER))
  ret *= .8;
 if (ret < 100)
  ret = 100;
 return (real_life ? ret : ret / 10);
}

int player::rust_rate(bool real_life)
{
 if (OPTIONS[OPT_SKILL_RUST] == 4) return 0;
 int intel = (real_life ? int_cur : int_max);
 int ret = (OPTIONS[OPT_SKILL_RUST] < 2 ? 500 : 500 - 35 * (intel - 8));
 if (has_trait(PF_FORGETFUL))
  ret *= 1.33;
 if (ret < 0)
  ret = 0;
 return (real_life ? ret : ret / 10);
}

int player::talk_skill()
{
    int ret = int_cur + per_cur + skillLevel("speech") * 3;
    if (has_trait(PF_UGLY))
        ret -= 3;
    else if (has_trait(PF_DEFORMED))
        ret -= 6;
    else if (has_trait(PF_DEFORMED2))
        ret -= 12;
    else if (has_trait(PF_DEFORMED3))
        ret -= 18;
    else if (has_trait(PF_PRETTY))
        ret += 1;
    else if (has_trait(PF_BEAUTIFUL))
        ret += 2;
    else if (has_trait(PF_BEAUTIFUL2))
        ret += 4;
    else if (has_trait(PF_BEAUTIFUL3))
        ret += 6;
    return ret;
}

int player::intimidation()
{
 int ret = str_cur * 2;
 if (weapon.is_gun())
  ret += 10;
 if (weapon.damage_bash() >= 12 || weapon.damage_cut() >= 12)
  ret += 5;
 if (has_trait(PF_DEFORMED2))
  ret += 3;
 else if (has_trait(PF_DEFORMED3))
  ret += 6;
 else if (has_trait(PF_PRETTY))
  ret -= 1;
 else if (has_trait(PF_BEAUTIFUL) || has_trait(PF_BEAUTIFUL2) || has_trait(PF_BEAUTIFUL3))
  ret -= 4;
 if (stim > 20)
  ret += 2;
 if (has_disease("drunk"))
  ret -= 4;

 return ret;
}

int player::hit(game *g, body_part bphurt, int side, int dam, int cut)
{
 int painadd = 0;
 if (has_disease("sleep")) {
  g->add_msg(_("You wake up!"));
  rem_disease("sleep");
 } else if (has_disease("lying_down"))
  rem_disease("lying_down");

 absorb(g, bphurt, dam, cut);

 dam += cut;
 if (dam <= 0)
  return dam;

 if( g->u_see( this->posx, this->posy ) ) {
     hit_animation(this->posx - g->u.posx + VIEWX - g->u.view_offset_x,
                   this->posy - g->u.posy + VIEWY - g->u.view_offset_y,
                   red_background(this->color()), '@');
 }

 rem_disease("speed_boost");
 if (dam >= 6)
  rem_disease("armor_boost");

 if (!is_npc())
  g->cancel_activity_query(_("You were hurt!"));

 if (has_artifact_with(AEP_SNAKES) && dam >= 6) {
  int snakes = int(dam / 6);
  std::vector<point> valid;
  for (int x = posx - 1; x <= posx + 1; x++) {
   for (int y = posy - 1; y <= posy + 1; y++) {
    if (g->is_empty(x, y))
     valid.push_back( point(x, y) );
   }
  }
  if (snakes > valid.size())
   snakes = valid.size();
  if (snakes == 1)
   g->add_msg(_("A snake sprouts from your body!"));
  else if (snakes >= 2)
   g->add_msg(_("Some snakes sprout from your body!"));
  monster snake(g->mtypes[mon_shadow_snake]);
  for (int i = 0; i < snakes; i++) {
   int index = rng(0, valid.size() - 1);
   point sp = valid[index];
   valid.erase(valid.begin() + index);
   snake.spawn(sp.x, sp.y);
   snake.friendly = -1;
   g->z.push_back(snake);
  }
 }

 if (has_trait(PF_PAINRESIST))
  painadd = (sqrt(double(cut)) + dam + cut) / (rng(4, 6));
 else
  painadd = (sqrt(double(cut)) + dam + cut) / 4;
 pain += painadd;

 switch (bphurt) {
 case bp_eyes:
  pain++;
  if (dam > 5 || cut > 0) {
   int minblind = int((dam + cut) / 10);
   if (minblind < 1)
    minblind = 1;
   int maxblind = int((dam + cut) /  4);
   if (maxblind > 5)
    maxblind = 5;
   add_disease("blind", rng(minblind, maxblind));
  }

 case bp_mouth: // Fall through to head damage
 case bp_head:
  pain++;
  hp_cur[hp_head] -= dam;
  if (hp_cur[hp_head] < 0)
   hp_cur[hp_head] = 0;
 break;
 case bp_torso:
  recoil += int(dam / 5);
  hp_cur[hp_torso] -= dam;
  if (hp_cur[hp_torso] < 0)
   hp_cur[hp_torso] = 0;
 break;
 case bp_hands: // Fall through to arms
 case bp_arms:
  if (side == 1 || side == 3 || weapon.is_two_handed(this))
   recoil += int(dam / 3);
  if (side == 0 || side == 3) {
   hp_cur[hp_arm_l] -= dam;
   if (hp_cur[hp_arm_l] < 0)
    hp_cur[hp_arm_l] = 0;
  }
  if (side == 1 || side == 3) {
   hp_cur[hp_arm_r] -= dam;
   if (hp_cur[hp_arm_r] < 0)
    hp_cur[hp_arm_r] = 0;
  }
 break;
 case bp_feet: // Fall through to legs
 case bp_legs:
  if (side == 0 || side == 3) {
   hp_cur[hp_leg_l] -= dam;
   if (hp_cur[hp_leg_l] < 0)
    hp_cur[hp_leg_l] = 0;
  }
  if (side == 1 || side == 3) {
   hp_cur[hp_leg_r] -= dam;
   if (hp_cur[hp_leg_r] < 0)
    hp_cur[hp_leg_r] = 0;
  }
 break;
 default:
  debugmsg("Wacky body part hit!");
 }
 if (has_trait(PF_ADRENALINE) && !has_disease("adrenaline") &&
     (hp_cur[hp_head] < 25 || hp_cur[hp_torso] < 15))
  add_disease("adrenaline", 200);

 return dam;
}

void player::hurt(game *g, body_part bphurt, int side, int dam)
{
 int painadd = 0;
 if (has_disease("sleep") && rng(0, dam) > 2) {
  g->add_msg(_("You wake up!"));
  rem_disease("sleep");
 } else if (has_disease("lying_down"))
  rem_disease("lying_down");

 if (dam <= 0)
  return;

 if (!is_npc())
  g->cancel_activity_query(_("You were hurt!"));

 if (has_trait(PF_PAINRESIST))
  painadd = dam / 3;
 else
  painadd = dam / 2;
 pain += painadd;

 switch (bphurt) {
 case bp_eyes:	// Fall through to head damage
 case bp_mouth:	// Fall through to head damage
 case bp_head:
  pain++;
  hp_cur[hp_head] -= dam;
  if (hp_cur[hp_head] < 0)
   hp_cur[hp_head] = 0;
 break;
 case bp_torso:
  hp_cur[hp_torso] -= dam;
  if (hp_cur[hp_torso] < 0)
   hp_cur[hp_torso] = 0;
 break;
 case bp_hands:	// Fall through to arms
 case bp_arms:
  if (side == 0 || side == 3) {
   hp_cur[hp_arm_l] -= dam;
   if (hp_cur[hp_arm_l] < 0)
    hp_cur[hp_arm_l] = 0;
  }
  if (side == 1 || side == 3) {
   hp_cur[hp_arm_r] -= dam;
   if (hp_cur[hp_arm_r] < 0)
    hp_cur[hp_arm_r] = 0;
  }
 break;
 case bp_feet:	// Fall through to legs
 case bp_legs:
  if (side == 0 || side == 3) {
   hp_cur[hp_leg_l] -= dam;
   if (hp_cur[hp_leg_l] < 0)
    hp_cur[hp_leg_l] = 0;
  }
  if (side == 1 || side == 3) {
   hp_cur[hp_leg_r] -= dam;
   if (hp_cur[hp_leg_r] < 0)
    hp_cur[hp_leg_r] = 0;
  }
 break;
 default:
  debugmsg("Wacky body part hurt!");
 }
 if (has_trait(PF_ADRENALINE) && !has_disease("adrenaline") &&
     (hp_cur[hp_head] < 25 || hp_cur[hp_torso] < 15))
  add_disease("adrenaline", 200);
}

void player::heal(body_part healed, int side, int dam)
{
 hp_part healpart;
 switch (healed) {
 case bp_eyes:	// Fall through to head damage
 case bp_mouth:	// Fall through to head damage
 case bp_head:
  healpart = hp_head;
 break;
 case bp_torso:
  healpart = hp_torso;
 break;
 case bp_hands:
// Shouldn't happen, but fall through to arms
  debugmsg("Heal against hands!");
 case bp_arms:
  if (side == 0)
   healpart = hp_arm_l;
  else
   healpart = hp_arm_r;
 break;
 case bp_feet:
// Shouldn't happen, but fall through to legs
  debugmsg("Heal against feet!");
 case bp_legs:
  if (side == 0)
   healpart = hp_leg_l;
  else
   healpart = hp_leg_r;
 break;
 default:
  debugmsg("Wacky body part healed!");
  healpart = hp_torso;
 }
 hp_cur[healpart] += dam;
 if (hp_cur[healpart] > hp_max[healpart])
  hp_cur[healpart] = hp_max[healpart];
}

void player::heal(hp_part healed, int dam)
{
 hp_cur[healed] += dam;
 if (hp_cur[healed] > hp_max[healed])
  hp_cur[healed] = hp_max[healed];
}

void player::healall(int dam)
{
 for (int i = 0; i < num_hp_parts; i++) {
  if (hp_cur[i] > 0) {
   hp_cur[i] += dam;
   if (hp_cur[i] > hp_max[i])
    hp_cur[i] = hp_max[i];
  }
 }
}

void player::hurtall(int dam)
{
 for (int i = 0; i < num_hp_parts; i++) {
  int painadd = 0;
  hp_cur[i] -= dam;
   if (hp_cur[i] < 0)
     hp_cur[i] = 0;
  if (has_trait(PF_PAINRESIST))
   painadd = dam / 3;
  else
   painadd = dam / 2;
  pain += painadd;
 }
}

void player::hitall(game *g, int dam, int vary)
{
 if (has_disease("sleep")) {
  g->add_msg(_("You wake up!"));
  rem_disease("sleep");
 } else if (has_disease("lying_down"))
  rem_disease("lying_down");

 for (int i = 0; i < num_hp_parts; i++) {
  int ddam = vary? dam * rng (100 - vary, 100) / 100 : dam;
  int cut = 0;
  absorb(g, (body_part) i, ddam, cut);
  int painadd = 0;
  hp_cur[i] -= ddam;
   if (hp_cur[i] < 0)
     hp_cur[i] = 0;
  if (has_trait(PF_PAINRESIST))
   painadd = dam / 3 / 4;
  else
   painadd = dam / 2 / 4;
  pain += painadd;
 }
}

void player::knock_back_from(game *g, int x, int y)
{
 if (x == posx && y == posy)
  return; // No effect
 point to(posx, posy);
 if (x < posx)
  to.x++;
 if (x > posx)
  to.x--;
 if (y < posy)
  to.y++;
 if (y > posy)
  to.y--;

// First, see if we hit a monster
 int mondex = g->mon_at(to.x, to.y);
 if (mondex != -1) {
  monster *z = &(g->z[mondex]);
  hit(g, bp_torso, 0, z->type->size, 0);
  add_disease("stunned", 1);
  if ((str_max - 6) / 4 > z->type->size) {
   z->knock_back_from(g, posx, posy); // Chain reaction!
   z->hurt((str_max - 6) / 4);
   z->add_effect(ME_STUNNED, 1);
  } else if ((str_max - 6) / 4 == z->type->size) {
   z->hurt((str_max - 6) / 4);
   z->add_effect(ME_STUNNED, 1);
  }

  g->add_msg_player_or_npc( this, _("You bounce off a %s!"), _("<npcname> bounces off a %s!"),
                            z->name().c_str() );

  return;
 }

 int npcdex = g->npc_at(to.x, to.y);
 if (npcdex != -1) {
  npc *p = g->active_npc[npcdex];
  hit(g, bp_torso, 0, 3, 0);
  add_disease("stunned", 1);
  p->hit(g, bp_torso, 0, 3, 0);
  g->add_msg_player_or_npc( this, _("You bounce off %s!"), _("<npcname> bounces off %s!"), p->name.c_str() );
  return;
 }

// If we're still in the function at this point, we're actually moving a tile!
 if (g->m.move_cost(to.x, to.y) == 0) { // Wait, it's a wall (or water)

  if (g->m.has_flag(liquid, to.x, to.y)) {
   if (!is_npc())
    g->plswim(to.x, to.y);
// TODO: NPCs can't swim!
  } else { // It's some kind of wall.
   hurt(g, bp_torso, 0, 3);
   add_disease("stunned", 2);
   g->add_msg_player_or_npc( this, _("You bounce off a %s!"), _("<npcname> bounces off a %s!"),
                             g->m.tername(to.x, to.y).c_str() );
  }

 } else { // It's no wall
  posx = to.x;
  posy = to.y;
 }
}

int player::hp_percentage()
{
 int total_cur = 0, total_max = 0;
// Head and torso HP are weighted 3x and 2x, respectively
 total_cur = hp_cur[hp_head] * 3 + hp_cur[hp_torso] * 2;
 total_max = hp_max[hp_head] * 3 + hp_max[hp_torso] * 2;
 for (int i = hp_arm_l; i < num_hp_parts; i++) {
  total_cur += hp_cur[i];
  total_max += hp_max[i];
 }
 return (100 * total_cur) / total_max;
}

void player::recalc_hp()
{
    int new_max_hp[num_hp_parts];
    for (int i = 0; i < num_hp_parts; i++)
    {
        new_max_hp[i] = 60 + str_max * 3;
        if (has_trait(PF_TOUGH))
        {
            new_max_hp[i] *= 1.2;
        }
        if (has_trait(PF_HARDCORE))
        {
            new_max_hp[i] *= 0.25;
        }
    }
    if (has_trait(PF_GLASSJAW))
    {
        new_max_hp[hp_head] *= 0.8;
    }
    for (int i = 0; i < num_hp_parts; i++)
    {
        hp_cur[i] *= (float)new_max_hp[i]/(float)hp_max[i];
        hp_max[i] = new_max_hp[i];
    }
}

void player::get_sick(game *g)
{
 if (health > 0 && rng(0, health + 10) < health)
  health--;
 if (health < 0 && rng(0, 10 - health) < (0 - health))
  health++;
 if (one_in(12))
  health -= 1;

 if (g->debugmon)
  debugmsg("Health: %d", health);

 if (has_trait(PF_DISIMMUNE))
  return;

 if (!has_disease("flu") && !has_disease("common_cold") &&
     one_in(900 + 10 * health + (has_trait(PF_DISRESISTANT) ? 300 : 0))) {
  if (one_in(6))
   infect("flu", bp_mouth, 3, rng(40000, 80000), g);
  else
   infect("common_cold", bp_mouth, 3, rng(20000, 60000), g);
 }
}

void player::infect(dis_type type, body_part vector, int strength,
                    int duration, game *g)
{
 if (dice(strength, 3) > dice(resist(vector), 3))
  add_disease(type, duration);
}

void player::add_disease(dis_type type, int duration,
                         int intensity, int max_intensity)
{
 if (duration == 0)
  return;
 bool found = false;
 int i = 0;
 while ((i < illness.size()) && !found) {
  if (illness[i].type == type) {
   illness[i].duration += duration;
   illness[i].intensity += intensity;
   if (max_intensity != -1 && illness[i].intensity > max_intensity)
    illness[i].intensity = max_intensity;
   found = true;
  }
  i++;
 }
 if (!found) {
  if (!is_npc())
   dis_msg(g, type);
  disease tmp(type, duration, intensity);
  illness.push_back(tmp);
 }
// activity.type = ACT_NULL;
}

void player::rem_disease(dis_type type)
{
 for (int i = 0; i < illness.size(); i++) {
  if (illness[i].type == type)
   illness.erase(illness.begin() + i);
 }
}

bool player::has_disease(dis_type type) const
{
 for (int i = 0; i < illness.size(); i++) {
  if (illness[i].type == type)
   return true;
 }
 return false;
}

int player::disease_level(dis_type type)
{
 for (int i = 0; i < illness.size(); i++) {
  if (illness[i].type == type)
   return illness[i].duration;
 }
 return 0;
}

int player::disease_intensity(dis_type type)
{
 for (int i = 0; i < illness.size(); i++) {
  if (illness[i].type == type)
   return illness[i].intensity;
 }
 return 0;
}

void player::add_addiction(add_type type, int strength)
{
 if (type == ADD_NULL)
  return;
 int timer = 1200;
 if (has_trait(PF_ADDICTIVE)) {
  strength = int(strength * 1.5);
  timer = 800;
 }
 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].type == type) {
        if (addictions[i].sated <   0)
    addictions[i].sated = timer;
   else if (addictions[i].sated < 600)
    addictions[i].sated += timer;	// TODO: Make this variable?
   else
    addictions[i].sated += int((3000 - addictions[i].sated) / 2);
   if ((rng(0, strength) > rng(0, addictions[i].intensity * 5) ||
       rng(0, 500) < strength) && addictions[i].intensity < 20)
    addictions[i].intensity++;
   return;
  }
 }
 if (rng(0, 100) < strength) {
  addiction tmp(type, 1);
  addictions.push_back(tmp);
 }
}

bool player::has_addiction(add_type type) const
{
 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].type == type &&
      addictions[i].intensity >= MIN_ADDICTION_LEVEL)
   return true;
 }
 return false;
}

void player::rem_addiction(add_type type)
{
 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].type == type) {
   addictions.erase(addictions.begin() + i);
   return;
  }
 }
}

int player::addiction_level(add_type type)
{
 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].type == type)
   return addictions[i].intensity;
 }
 return 0;
}

void player::siphon(game *g, vehicle *veh, ammotype desired_liquid)
{
    int liquid_amount = veh->drain( desired_liquid , veh->fuel_capacity( desired_liquid ));
    item used_item(g->itypes[ default_ammo(desired_liquid) ], g->turn);
    used_item.charges = liquid_amount;
    g->add_msg(_("Siphoned %d units of %s from the vehicle."), liquid_amount, used_item.name.c_str());
    while (!g->handle_liquid(used_item, false, false)) { } // handle the liquid until it's all gone
}

void player::cauterize(game *g) {
 rem_disease("bleed");
 rem_disease("bite");
 pain += 15;
 g->add_msg_if_player(this,_("You cauterize yourself. It hurts like hell!"));
}

void player::suffer(game *g)
{
    for (int i = 0; i < my_bionics.size(); i++)
    {
        if (my_bionics[i].powered)
        {
            activate_bionic(i, g);
        }
    }
    if (underwater)
    {
        if (!has_trait(PF_GILLS))
        {
            oxygen--;
        }
        if (oxygen < 0)
        {
            if (has_bionic("bio_gills") && power_level > 0)
            {
                oxygen += 5;
                power_level--;
            }
            else
            {
                g->add_msg(_("You're drowning!"));
                hurt(g, bp_torso, 0, rng(1, 4));
            }
        }
    }
    for (int i = 0; i < illness.size(); i++)
    {
        dis_effect(g, *this, illness[i]);
        illness[i].duration--;
        if (illness[i].duration < MIN_DISEASE_AGE)// Cap permanent disease age
        {
            illness[i].duration = MIN_DISEASE_AGE;
        }
        if (illness[i].duration == 0)
        {
            illness.erase(illness.begin() + i);
            i--;
        }
    }
    if (!has_disease("sleep"))
    {
        int timer = -3600;
        if (has_trait(PF_ADDICTIVE))
        {
            timer = -4000;
        }
        for (int i = 0; i < addictions.size(); i++)
        {
            if (addictions[i].sated <= 0 &&
                addictions[i].intensity >= MIN_ADDICTION_LEVEL)
            {
                addict_effect(g, addictions[i]);
            }
            addictions[i].sated--;
            if (!one_in(addictions[i].intensity - 2) && addictions[i].sated > 0)
            {
                addictions[i].sated -= 1;
            }
            if (addictions[i].sated < timer - (100 * addictions[i].intensity))
            {
                if (addictions[i].intensity <= 2)
                {
                    addictions.erase(addictions.begin() + i);
                    i--;
                }
                else
                {
                    addictions[i].intensity = int(addictions[i].intensity / 2);
                    addictions[i].intensity--;
                    addictions[i].sated = 0;
                }
            }
        }
        if (has_trait(PF_CHEMIMBALANCE))
        {
            if (one_in(3600))
            {
                g->add_msg(_("You suddenly feel sharp pain for no reason."));
                pain += 3 * rng(1, 3);
            }
            if (one_in(3600))
            {
                int pkilladd = 5 * rng(-1, 2);
                if (pkilladd > 0)
                {
                    g->add_msg(_("You suddenly feel numb."));
                }
                else if (pkilladd < 0)
                {
                    g->add_msg(_("You suddenly ache."));
                }
                pkill += pkilladd;
            }
            if (one_in(3600))
            {
                g->add_msg(_("You feel dizzy for a moment."));
                moves -= rng(10, 30);
            }
            if (one_in(3600))
            {
                int hungadd = 5 * rng(-1, 3);
                if (hungadd > 0)
                {
                    g->add_msg(_("You suddenly feel hungry."));
                }
                else
                {
                    g->add_msg(_("You suddenly feel a little full."));
                }
                hunger += hungadd;
            }
            if (one_in(3600))
            {
                g->add_msg(_("You suddenly feel thirsty."));
                thirst += 5 * rng(1, 3);
            }
            if (one_in(3600))
            {
                g->add_msg(_("You feel fatigued all of a sudden."));
                fatigue += 10 * rng(2, 4);
            }
            if (one_in(4800))
            {
                if (one_in(3))
                {
                    add_morale(MORALE_FEELING_GOOD, 20, 100);
                }
                else
                {
                    add_morale(MORALE_FEELING_BAD, -20, -100);
                }
            }
            if (one_in(3600))
            {
                if (one_in(3))
                {
                    g->add_msg(_("You suddenly feel very cold."));
                    for (int i = 0 ; i < num_bp ; i++)
                    {
                        temp_cur[i] = BODYTEMP_VERY_COLD;
                    }
                }
                else
                {
                    g->add_msg(_("You suddenly feel cold."));
                    for (int i = 0 ; i < num_bp ; i++)
                    {
                        temp_cur[i] = BODYTEMP_COLD;
                    }
                }
            }
            if (one_in(3600))
            {
                if (one_in(3))
                {
                    g->add_msg(_("You suddenly feel very hot."));
                    for (int i = 0 ; i < num_bp ; i++)
                    {
                        temp_cur[i] = BODYTEMP_VERY_HOT;
                    }
                }
                else
                {
                    g->add_msg(_("You suddenly feel hot."));
                    for (int i = 0 ; i < num_bp ; i++)
                    {
                        temp_cur[i] = BODYTEMP_HOT;
                    }
                }
            }
        }
        if ((has_trait(PF_SCHIZOPHRENIC) || has_artifact_with(AEP_SCHIZO)) &&
            one_in(2400))
        { // Every 4 hours or so
            monster phantasm;
            int i;
            switch(rng(0, 11))
            {
                case 0:
                    add_disease("hallu", 3600);
                    break;
                case 1:
                    add_disease("visuals", rng(15, 60));
                    break;
                case 2:
                    g->add_msg(_("From the south you hear glass breaking."));
                    break;
                case 3:
                    g->add_msg(_("YOU SHOULD QUIT THE GAME IMMEDIATELY."));
                    add_morale(MORALE_FEELING_BAD, -50, -150);
                    break;
                case 4:
                    for (i = 0; i < 10; i++) {
                        g->add_msg("XXXXXXXXXXXXXXXXXXXXXXXXXXX");
                    }
                    break;
                case 5:
                    g->add_msg(_("You suddenly feel so numb..."));
                    pkill += 25;
                    break;
                case 6:
                    g->add_msg(_("You start to shake uncontrollably."));
                    add_disease("shakes", 10 * rng(2, 5));
                    break;
                case 7:
                    for (i = 0; i < 10; i++)
                    {
                        phantasm = monster(g->mtypes[mon_hallu_zom + rng(0, 3)]);
                        phantasm.spawn(posx + rng(-10, 10), posy + rng(-10, 10));
                        if (g->mon_at(phantasm.posx, phantasm.posy) == -1)
                            g->z.push_back(phantasm);
                    }
                    break;
                case 8:
                    g->add_msg(_("It's a good time to lie down and sleep."));
                    add_disease("lying_down", 200);
                    break;
                case 9:
                    g->add_msg(_("You have the sudden urge to SCREAM!"));
                    g->sound(posx, posy, 10 + 2 * str_cur, "AHHHHHHH!");
                    break;
                case 10:
                    g->add_msg(std::string(name + name + name + name + name + name + name +
                        name + name + name + name + name + name + name +
                        name + name + name + name + name + name).c_str());
                    break;
                case 11:
                    add_disease("formication", 600);
                    break;
            }
        }
  if (has_trait(PF_JITTERY) && !has_disease("shakes")) {
   if (stim > 50 && one_in(300 - stim))
    add_disease("shakes", 300 + stim);
   else if (hunger > 80 && one_in(500 - hunger))
    add_disease("shakes", 400);
  }

  if (has_trait(PF_MOODSWINGS) && one_in(3600)) {
   if (rng(1, 20) > 9)	// 55% chance
    add_morale(MORALE_MOODSWING, -100, -500);
   else			// 45% chance
    add_morale(MORALE_MOODSWING, 100, 500);
  }

  if (has_trait(PF_VOMITOUS) && one_in(4200))
   vomit(g);

  if (has_trait(PF_SHOUT1) && one_in(3600))
   g->sound(posx, posy, 10 + 2 * str_cur, _("You shout loudly!"));
  if (has_trait(PF_SHOUT2) && one_in(2400))
   g->sound(posx, posy, 15 + 3 * str_cur, _("You scream loudly!"));
  if (has_trait(PF_SHOUT3) && one_in(1800))
   g->sound(posx, posy, 20 + 4 * str_cur, _("You let out a piercing howl!"));
 }	// Done with while-awake-only effects

 if (has_trait(PF_ASTHMA) && one_in(3600 - stim * 50)) {
  bool auto_use = has_charges("inhaler", 1);
  if (underwater) {
   oxygen = int(oxygen / 2);
   auto_use = false;
  }
  if (has_disease("sleep")) {
   rem_disease("sleep");
   g->add_msg(_("Your asthma wakes you up!"));
   auto_use = false;
  }
  if (auto_use)
   use_charges("inhaler", 1);
  else {
   add_disease("asthma", 50 * rng(1, 4));
   if (!is_npc())
    g->cancel_activity_query(_("You have an asthma attack!"));
  }
 }

 if (has_trait(PF_LEAVES) && g->is_in_sunlight(posx, posy) && one_in(600))
  hunger--;

 if (pain > 0) {
  if (has_trait(PF_PAINREC1) && one_in(600))
   pain--;
  if (has_trait(PF_PAINREC2) && one_in(300))
   pain--;
  if (has_trait(PF_PAINREC3) && one_in(150))
   pain--;
 }

 if (has_trait(PF_ALBINO) && g->is_in_sunlight(posx, posy) && one_in(20)) {
  g->add_msg(_("The sunlight burns your skin!"));
  if (has_disease("sleep")) {
   rem_disease("sleep");
   g->add_msg(_("You wake up!"));
  }
  hurtall(1);
 }

 if ((has_trait(PF_TROGLO) || has_trait(PF_TROGLO2)) &&
     g->is_in_sunlight(posx, posy) && g->weather == WEATHER_SUNNY) {
  str_cur--;
  dex_cur--;
  int_cur--;
  per_cur--;
 }
 if (has_trait(PF_TROGLO2) && g->is_in_sunlight(posx, posy)) {
  str_cur--;
  dex_cur--;
  int_cur--;
  per_cur--;
 }
 if (has_trait(PF_TROGLO3) && g->is_in_sunlight(posx, posy)) {
  str_cur -= 4;
  dex_cur -= 4;
  int_cur -= 4;
  per_cur -= 4;
 }

 if (has_trait(PF_SORES)) {
  for (int i = bp_head; i < num_bp; i++) {
   if (pain < 5 + 4 * abs(encumb(body_part(i))))
    pain = 5 + 4 * abs(encumb(body_part(i)));
  }
 }

 if (has_trait(PF_SLIMY) && !in_vehicle) {
   g->m.add_field(g, posx, posy, fd_slime, 1);
 }

 if (has_trait(PF_WEB_WEAVER) && !in_vehicle && one_in(3)) {
   g->m.add_field(g, posx, posy, fd_web, 1); //this adds density to if its not already there.
 }

 if (has_trait(PF_RADIOGENIC) && int(g->turn) % 50 == 0 && radiation >= 10) {
  radiation -= 10;
  healall(1);
 }

 if (has_trait(PF_RADIOACTIVE1)) {
  if (g->m.radiation(posx, posy) < 10 && one_in(50))
   g->m.radiation(posx, posy)++;
 }
 if (has_trait(PF_RADIOACTIVE2)) {
  if (g->m.radiation(posx, posy) < 20 && one_in(25))
   g->m.radiation(posx, posy)++;
 }
 if (has_trait(PF_RADIOACTIVE3)) {
  if (g->m.radiation(posx, posy) < 30 && one_in(10))
   g->m.radiation(posx, posy)++;
 }

 if (has_trait(PF_UNSTABLE) && one_in(28800))	// Average once per 2 days
  mutate(g);
 if (has_artifact_with(AEP_MUTAGENIC) && one_in(28800))
  mutate(g);
 if (has_artifact_with(AEP_FORCE_TELEPORT) && one_in(600))
  g->teleport(this);

 int localRadiation = g->m.radiation(posx, posy);

 if (localRadiation) {
   bool power_armored = false, has_helmet = false;

   power_armored = is_wearing_power_armor(&has_helmet);

   if (power_armored && has_helmet) {
     radiation += 0; // Power armor protects completely from radiation
   } else if (power_armored || is_wearing("hazmat_suit")) {
     radiation += rng(0, localRadiation / 40);
   } else {
     radiation += rng(0, localRadiation / 16);
   }

   // Apply rads to any radiation badges.
   std::vector<item *> possessions = inv_dump();
   for( std::vector<item *>::iterator it = possessions.begin(); it != possessions.end(); ++it ) {
       if( (*it)->type->id == "rad_badge" ) {
           // Actual irridation levels of badges and the player aren't precisely matched.
           // This is intentional.
           int before = (*it)->irridation;
           (*it)->irridation += rng(0, localRadiation / 16);
           if( inv.has_item(*it) ) { continue; }
           for( int i = 0; i < sizeof(rad_dosage_thresholds)/sizeof(rad_dosage_thresholds[0]); i++ ){
               if( before < rad_dosage_thresholds[i] &&
                   (*it)->irridation >= rad_dosage_thresholds[i] ) {
                   g->add_msg_if_player( this, _("Your radiation badge changes from %s to %s!"),
                                         rad_threshold_colors[i - 1].c_str(),
                                         rad_threshold_colors[i].c_str() );
               }
           }
       }
   }
 }

 if( int(g->turn) % 150 == 0 )
 {
     if (radiation < 0) radiation = 0;
     else if (radiation > 2000) radiation = 2000;
     if (OPTIONS[OPT_RAD_MUTATION] && rng(60, 2500) < radiation)
     {
         mutate(g);
         radiation /= 2;
         radiation -= 5;
     }
     else if (radiation > 100 && rng(1, 1500) < radiation)
     {
         vomit(g);
         radiation -= 50;
     }
 }

 if( radiation > 150 && !(int(g->turn) % 90) )
 {
     hurtall(radiation / 100);
 }

// Negative bionics effects
 if (has_bionic("bio_dis_shock") && one_in(1200)) {
  g->add_msg(_("You suffer a painful electrical discharge!"));
  pain++;
  moves -= 150;
 }
 if (has_bionic("bio_dis_acid") && one_in(1500)) {
  g->add_msg(_("You suffer a burning acidic discharge!"));
  hurtall(1);
 }
 if (has_bionic("bio_drain") && power_level > 0 && one_in(600)) {
  g->add_msg(_("Your batteries discharge slightly."));
  power_level--;
 }
 if (has_bionic("bio_noise") && one_in(500)) {
  g->add_msg(_("A bionic emits a crackle of noise!"));
  g->sound(posx, posy, 60, "");
 }
 if (has_bionic("bio_power_weakness") && max_power_level > 0 &&
     power_level >= max_power_level * .75)
  str_cur -= 3;

// Artifact effects
 if (has_artifact_with(AEP_ATTENTION))
  add_disease("attention", 3);

 if (dex_cur < 0)
  dex_cur = 0;
 if (str_cur < 0)
  str_cur = 0;
 if (per_cur < 0)
  per_cur = 0;
 if (int_cur < 0)
  int_cur = 0;

 // check for limb mending every 1000 turns (~1.6 hours)
 if(g->turn.get_turn() % 1000 == 0) {
  mend(g);
 }
}

void player::mend(game *g)
{
 // Wearing splints can slowly mend a broken limb back to 1 hp.
 // 2 weeks is faster than a fracture would heal IRL,
 // but 3 weeks average (a generous estimate) was tedious and no fun.
 for(int i = 0; i < num_hp_parts; i++) {
  int broken = (hp_cur[i] <= 0);
  if(broken) {
   double mending_odds = 200.0; // 2 weeks, on average. (~20160 minutes / 100 minutes)
   double healing_factor = 1.0;
   // Studies have shown that alcohol and tobacco use delay fracture healing time
   if(has_disease("cig") | addiction_level(ADD_CIG)) {
    healing_factor *= 0.5;
   }
   if(has_disease("drunk") | addiction_level(ADD_ALCOHOL)) {
    healing_factor *= 0.5;
   }

   // Bed rest speeds up mending
   if(has_disease("sleep")) {
    healing_factor *= 4.0;
   } else if(fatigue > 383) {
    // but being dead tired does not...
    healing_factor *= 0.75;
   }

   // Being healthy helps.
   if(health > 0) {
    healing_factor *= 2.0;
   }

   // And being well fed...
   if(hunger < 0) {
    healing_factor *= 2.0;
   }

   if(thirst < 0) {
    healing_factor *= 2.0;
   }

   // Mutagenic healing factor!
   if(has_trait(PF_REGEN)) {
    healing_factor *= 16.0;
   } else if (has_trait(PF_FASTHEALER2)) {
    healing_factor *= 4.0;
   } else if (has_trait(PF_FASTHEALER)) {
    healing_factor *= 2.0;
   }

   bool mended = false;
   int side = 0;
   body_part part;
   switch(i) {
    case hp_arm_r:
     side = 1;
     // fall-through
    case hp_arm_l:
     part = bp_arms;
     mended = is_wearing("arm_splint") && x_in_y(healing_factor, mending_odds);
     break;
    case hp_leg_r:
     side = 1;
     // fall-through
    case hp_leg_l:
     part = bp_legs;
     mended = is_wearing("leg_splint") && x_in_y(healing_factor, mending_odds);
     break;
    default:
     // No mending for you!
     break;
   }
   if(mended) {
    hp_cur[i] = 1;
    g->add_msg(_("Your %s has started to mend!"),
      body_part_name(part, side).c_str());
   }
  }
 }
}

void player::vomit(game *g)
{
 g->add_msg(_("You throw up heavily!"));
 hunger += rng(30, 50);
 thirst += rng(30, 50);
 moves -= 100;
 for (int i = 0; i < illness.size(); i++) {
  if (illness[i].type == "foodpoison") {
   illness[i].duration -= 300;
   if (illness[i].duration < 0)
    rem_disease(illness[i].type);
  } else if (illness[i].type == "drunk") {
   illness[i].duration -= rng(1, 5) * 100;
   if (illness[i].duration < 0)
    rem_disease(illness[i].type);
  }
 }
 rem_disease("pkill1");
 rem_disease("pkill2");
 rem_disease("pkill3");
 rem_disease("sleep");
}

int player::weight_carried()
{
    int ret = 0;
    ret += weapon.weight();
    for (int i = 0; i < worn.size(); i++)
    {
        ret += worn[i].weight();
    }
    ret += inv.weight();
    return ret;
}

int player::volume_carried()
{
    return inv.volume();
}

int player::weight_capacity(bool real_life)
{
 int str = (real_life ? str_cur : str_max);
 int ret = 400 + str * 35;
 if (has_trait(PF_BADBACK))
  ret = int(ret * .65);
 if (has_trait(PF_LIGHT_BONES))
  ret = int(ret * .80);
 if (has_trait(PF_HOLLOW_BONES))
  ret = int(ret * .60);
 if (has_artifact_with(AEP_CARRY_MORE))
  ret += 200;
 return ret;
}

int player::volume_capacity()
{
 int ret = 2;	// A small bonus (the overflow)
 it_armor *armor;
 for (int i = 0; i < worn.size(); i++) {
  armor = dynamic_cast<it_armor*>(worn[i].type);
  ret += armor->storage;
 }
 if (has_bionic("bio_storage"))
  ret += 6;
 if (has_trait(PF_SHELL))
  ret += 16;
 if (has_trait(PF_PACKMULE))
  ret = int(ret * 1.4);
 return ret;
}

bool player::can_pickVolume(int volume)
{
    return (volume_carried() + volume <= volume_capacity());
}
bool player::can_pickWeight(int weight)
{
    return (weight_carried() + weight <= weight_capacity());
}

// --- Library functions ---
// This stuff could be moved elsewhere, but there
// doesn't seem to be a good place to put it right now.

// Basic logistic function.
double logistic(double t)
{
    return 1 / (1 + exp(-t));
}

const double LOGI_CUTOFF = 4;
const double LOGI_MIN = logistic(-LOGI_CUTOFF);
const double LOGI_MAX = logistic(+LOGI_CUTOFF);
const double LOGI_RANGE = LOGI_MAX - LOGI_MIN;

// Logistic curve [-6,6], flipped and scaled to
// range from 1 to 0 as pos goes from min to max.
double logistic_range(int min, int max, int pos)
{
    // Anything beyond [min,max] gets clamped.
    if (pos < min)
    {
        return 1.0;
    }
    else if (pos > max)
    {
        return 0.0;
    }

    // Normalize the pos to [0,1]
    double range = max - min;
    double unit_pos = (pos - min) / range;

    // Scale and flip it to [+LOGI_CUTOFF,-LOGI_CUTOFF]
    double scaled_pos = LOGI_CUTOFF - 2 * LOGI_CUTOFF * unit_pos;

    // Get the raw logistic value.
    double raw_logistic = logistic(scaled_pos);

    // Scale the output to [0,1]
    return (raw_logistic - LOGI_MIN) / LOGI_RANGE;
}
// --- End ---


int player::net_morale(morale_point effect)
{
    double bonus = effect.bonus;

    // If the effect is old enough to have started decaying,
    // reduce it appropriately.
    if (effect.age > effect.decay_start)
    {
        bonus *= logistic_range(effect.decay_start,
                                effect.duration, effect.age);
    }

    // Optimistic characters focus on the good things in life,
    // and downplay the bad things.
    if (has_trait(PF_OPTIMISTIC))
    {
        if (bonus >= 0)
        {
            bonus *= 1.25;
        }
        else
        {
            bonus *= 0.75;
        }
    }

    return bonus;
}

int player::morale_level()
{
    // Add up all of the morale bonuses (and penalties).
    int ret = 0;
    for (int i = 0; i < morale.size(); i++)
    {
        ret += net_morale(morale[i]);
    }

    // Prozac reduces negative morale by 75%.
    if (has_disease("took_prozac") && ret < 0)
    {
        ret = int(ret / 4);
    }

    return ret;
}

void player::add_morale(morale_type type, int bonus, int max_bonus,
                        int duration, int decay_start,
                        bool cap_existing, itype* item_type)
{
    bool placed = false;

    // Search for a matching morale entry.
    for (int i = 0; i < morale.size() && !placed; i++)
    {
        if (morale[i].type == type && morale[i].item_type == item_type)
        {
            // Found a match!
            placed = true;

            // Scale the morale bonus to its current level.
            if (morale[i].age > morale[i].decay_start)
            {
                morale[i].bonus *= logistic_range(morale[i].decay_start,
                                                  morale[i].duration, morale[i].age);
            }

            // If we're capping the existing effect, we can use the new duration
            // and decay start.
            if (cap_existing)
            {
                morale[i].duration = duration;
                morale[i].decay_start = decay_start;
            }
            else
            {
                // Otherwise, we need to figure out whether the existing effect had
                // more remaining duration and decay-resistance than the new one does.
                if (morale[i].duration - morale[i].age <= duration)
                {
                    morale[i].duration = duration;
                }
                else
                {
                    // This will give a longer duration than above.
                    morale[i].duration -= morale[i].age;
                }

                if (morale[i].decay_start - morale[i].age <= decay_start)
                {
                    morale[i].decay_start = decay_start;
                }
                else
                {
                    // This will give a later decay start than above.
                    morale[i].decay_start -= morale[i].age;
                }
            }

            // Now that we've finished using it, reset the age to 0.
            morale[i].age = 0;

            // Is the current morale level for this entry below its cap, if any?
            if (abs(morale[i].bonus) < abs(max_bonus) || max_bonus == 0)
            {
                // Add the requested morale boost.
                morale[i].bonus += bonus;

                // If we passed the cap, pull back to it.
                if (abs(morale[i].bonus) > abs(max_bonus) && max_bonus != 0)
                {
                    morale[i].bonus = max_bonus;
                }
            }
            else if (cap_existing)
            {
                // The existing bonus is above the new cap.  Reduce it.
                morale[i].bonus = max_bonus;
            }
        }
    }

    // No matching entry, so add a new one
    if (!placed)
    {
        morale_point tmp(type, item_type, bonus, duration, decay_start, 0);
        morale.push_back(tmp);
    }
}

int player::has_morale( morale_type type ) const
{
    for( int i = 0; i < morale.size(); i++ ) {
        if( morale[i].type == type ) {
            return morale[i].bonus;
        }
    }
    return 0;
}

void player::rem_morale(morale_type type, itype* item_type)
{
 for (int i = 0; i < morale.size(); i++) {
  if (morale[i].type == type && morale[i].item_type == item_type) {
    morale.erase(morale.begin() + i);
    break;
  }
 }
}

item& player::i_add(item it, game *g)
{
 itype_id item_type_id = "null";
 if( it.type ) item_type_id = it.type->id;

 last_item = item_type_id;

 if (it.is_food() || it.is_ammo() || it.is_gun()  || it.is_armor() ||
     it.is_book() || it.is_tool() || it.is_weap() || it.is_food_container())
  inv.unsort();

 if (g != NULL && it.is_artifact() && it.is_tool()) {
  it_artifact_tool *art = dynamic_cast<it_artifact_tool*>(it.type);
  g->add_artifact_messages(art->effects_carried);
 }
 return inv.add_item(it);
}

bool player::has_active_item(itype_id id)
{
    if (weapon.type->id == id && weapon.active)
    {
        return true;
    }
    return inv.has_active_item(id);
}

int player::active_item_charges(itype_id id)
{
    int max = 0;
    if (weapon.type->id == id && weapon.active)
    {
        max = weapon.charges;
    }

    int inv_max = inv.max_active_item_charges(id);
    if (inv_max > max)
    {
        max = inv_max;
    }
    return max;
}

void player::process_active_items(game *g)
{
 if (weapon.is_artifact() && weapon.is_tool())
  g->process_artifact(&weapon, this, true);
 else if (weapon.active) {
  if (weapon.has_flag("CHARGE")) { // We're chargin it up!
   if (weapon.charges == 8) {
    bool maintain = false;
    if (use_charges_if_avail("adv_UPS_on", 2) || use_charges_if_avail("UPS_on", 4)) {
     maintain = true;
    } else if (use_charges_if_avail("adv_UPS_off", 2) || use_charges_if_avail("UPS_off", 4)) {
     maintain = true;
    }
    if (maintain) {
     if (one_in(20)) {
      g->add_msg(_("Your %s discharges!"), weapon.tname().c_str());
      point target(posx + rng(-12, 12), posy + rng(-12, 12));
      std::vector<point> traj = line_to(posx, posy, target.x, target.y, 0);
      g->fire(*this, target.x, target.y, traj, false);
     } else
      g->add_msg(_("Your %s beeps alarmingly."), weapon.tname().c_str());
    }
   } else {
    if (use_charges_if_avail("adv_UPS_on", (1 + weapon.charges)/2) || use_charges_if_avail("UPS_on", 1 + weapon.charges)) {
     weapon.poison++;
    } else if (use_charges_if_avail("adv_UPS_off", (1 + weapon.charges)/2) || use_charges_if_avail("UPS_off", 1 + weapon.charges)) {
     weapon.poison++;
    } else {
     g->add_msg(_("Your %s spins down."), weapon.tname().c_str());
     if (weapon.poison <= 0) {
      weapon.charges--;
      weapon.poison = weapon.charges - 1;
     } else
      weapon.poison--;
     if (weapon.charges == 0)
      weapon.active = false;
    }
    if (weapon.poison >= weapon.charges) {
     weapon.charges++;
     weapon.poison = 0;
    }
   }
   return;
  } // if (weapon.has_flag("CHARGE"))
  if (!process_single_active_item(g, &weapon))
  {
   weapon = get_combat_style();
  }
 }

    std::vector<item*> inv_active = inv.active_items();
    for (std::vector<item*>::iterator iter = inv_active.begin(); iter != inv_active.end(); ++iter)
    {
        item *tmp_it = *iter;
        if (tmp_it->is_artifact() && tmp_it->is_tool())
        {
            g->process_artifact(tmp_it, this);
        }
        if (!process_single_active_item(g, tmp_it))
        {
            inv.remove_item(tmp_it);
        }
    }

// worn items
  for (int i = 0; i < worn.size(); i++) {
    if (worn[i].is_artifact())
    g->process_artifact(&(worn[i]), this);
  }
}

// returns false if the item needs to be removed
bool player::process_single_active_item(game *g, item *it)
{
    if (it->active ||
        (it->is_container() && it->contents.size() > 0 && it->contents[0].active))
    {
        if (it->is_food())
        {
            if (it->has_flag("HOT"))
            {
                it->item_counter--;
                if (it->item_counter == 0)
                {
                    it->item_tags.erase("HOT");
                    it->active = false;
                }
            }
        }
        else if (it->is_food_container())
        {
            if (it->contents[0].has_flag("HOT"))
            {
                it->contents[0].item_counter--;
                if (it->contents[0].item_counter == 0)
                {
                    it->contents[0].item_tags.erase("HOT");
                    it->contents[0].active = false;
                }
            }
        }
        else if (it->is_tool())
        {
            iuse use;
            it_tool* tmp = dynamic_cast<it_tool*>(it->type);
            (use.*tmp->use)(g, this, it, true);
            if (tmp->turns_per_charge > 0 && int(g->turn) % tmp->turns_per_charge == 0)
            {
                it->charges--;
            }
            if (it->charges <= 0)
            {
                (use.*tmp->use)(g, this, it, false);
                if (tmp->revert_to == "null")
                {
                    return false;
                }
                else
                {
                    it->type = g->itypes[tmp->revert_to];
                }
            }
        }
        else if (it->type->id == "corpse")
        {
            if (it->ready_to_revive(g))
            {
                g->add_msg_if_player(this, _("Oh dear god, a corpse you're carrying has started moving!"));
                g->revive_corpse(posx, posy, it);
                return false;
            }
        }
        else
        {
            debugmsg("%s is active, but has no known active function.", it->tname().c_str());
        }
    }
    return true;
}

item player::remove_weapon()
{
 if (weapon.has_flag("CHARGE") && weapon.active) { //unwield a charged charge rifle.
  weapon.charges = 0;
  weapon.active = false;
 }
 item tmp = weapon;
 weapon = get_combat_style();
// We need to remove any boosts related to our style
 rem_disease("attack_boost");
 rem_disease("dodge_boost");
 rem_disease("damage_boost");
 rem_disease("speed_boost");
 rem_disease("armor_boost");
 rem_disease("viper_combo");
 return tmp;
}

void player::remove_mission_items(int mission_id)
{
 if (mission_id == -1)
  return;
 if (weapon.mission_id == mission_id) {
  remove_weapon();
 } else {
  for (int i = 0; i < weapon.contents.size(); i++) {
   if (weapon.contents[i].mission_id == mission_id)
    remove_weapon();
  }
 }
 inv.remove_mission_items(mission_id);
}

item player::i_rem(char let)
{
 item tmp;
 if (weapon.invlet == let) {
  if (std::find(martial_arts_itype_ids.begin(), martial_arts_itype_ids.end(), weapon.type->id) != martial_arts_itype_ids.end()){
   return ret_null;
  }
  tmp = weapon;
  weapon = get_combat_style();
  return tmp;
 }
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].invlet == let) {
   tmp = worn[i];
   worn.erase(worn.begin() + i);
   return tmp;
  }
 }
 if (!inv.item_by_letter(let).is_null())
  return inv.remove_item_by_letter(let);
 return ret_null;
}

item player::i_rem(itype_id type)
{
    item ret;
    if (weapon.type->id == type)
    {
        return remove_weapon();
    }
    return inv.remove_item_by_type(type);
}

item& player::i_at(char let)
{
 if (let == KEY_ESCAPE)
  return ret_null;
 if (weapon.invlet == let)
  return weapon;
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].invlet == let)
   return worn[i];
 }
 return inv.item_by_letter(let);
}

item& player::i_of_type(itype_id type)
{
 if (weapon.type->id == type)
  return weapon;
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].type->id == type)
   return worn[i];
 }
 return inv.item_by_type(type);
 return ret_null;
}

item player::get_combat_style()
{
 item tmp;
 bool pickstyle = (!styles.empty());
 if (pickstyle) {
  tmp = item( g->itypes[style_selected], 0 );
  tmp.invlet = ':';
  return tmp;
 } else {
  return ret_null;
 }
}

std::vector<item *> player::inv_dump()
{
 std::vector<item *> ret;
 if (std::find(standard_itype_ids.begin(), standard_itype_ids.end(), weapon.type->id) != standard_itype_ids.end()){
  ret.push_back(&weapon);
 }
 for (int i = 0; i < worn.size(); i++)
  ret.push_back(&worn[i]);
 inv.dump(ret);
 return ret;
}

item player::i_remn(char invlet)
{
 return inv.remove_item_by_letter(invlet);
}

std::list<item> player::use_amount(itype_id it, int quantity, bool use_container)
{
 std::list<item> ret;
 bool used_weapon_contents = false;
 for (int i = 0; i < weapon.contents.size(); i++) {
  if (weapon.contents[i].type->id == it) {
   ret.push_back(weapon.contents[i]);
   quantity--;
   weapon.contents.erase(weapon.contents.begin() + i);
   i--;
   used_weapon_contents = true;
  }
 }
 if (use_container && used_weapon_contents)
  remove_weapon();

 if (weapon.type->id == it) {
  quantity--;
  ret.push_back(remove_weapon());
 }

 std::list<item> tmp = inv.use_amount(it, quantity, use_container);
 ret.splice(ret.end(), tmp);
 return ret;
}

bool player::use_charges_if_avail(itype_id it, int quantity)
{
    if (has_charges(it, quantity))
    {
        use_charges(it, quantity);
        return true;
    }
    return false;
}

bool player::has_fire(const int quantity)
{
// TODO: Replace this with a "tool produces fire" flag.

    if (has_charges("torch_lit", 1)) {
        return true;
    } else if (has_charges("candle_lit", 1)) {
        return true;
    } else if (has_bionic("bio_tools")) {
        return true;
    } else if (has_bionic("bio_lighter")) {
        return true;
    } else if (has_bionic("bio_laser")) {
        return true;
    } else if (has_charges("matches", quantity)) {
        return true;
    } else if (has_charges("lighter", quantity)) {
        return true;
    } else if (has_charges("flamethrower", quantity)) {
        return true;
    } else if (has_charges("flamethrower_simple", quantity)) {
        return true;
    } else if (has_charges("hotplate", quantity)) {
        return true;
    } else if (has_charges("welder", quantity)) {
        return true;
    }
    return false;
}

void player::use_fire(const int quantity)
{
//Ok, so checks for nearby fires first,
//then held lit torch or candle, bio tool/lighter/laser
//tries to use 1 charge of lighters, matches, flame throwers
// (home made, military), hotplate, welder in that order.
// bio_lighter, bio_laser, bio_tools, has_bionic("bio_tools"

    if (has_charges("torch_lit", 1)) {
        return;
    } else if (has_charges("candle_lit", 1)) {
        return;
    } else if (has_bionic("bio_tools")) {
        return;
    } else if (has_bionic("bio_lighter")) {
        return;
    } else if (has_bionic("bio_laser")) {
        return;
    } else if (has_charges("matches", quantity)) {
        use_charges("matches", quantity);
        return;
    } else if (has_charges("lighter", quantity)) {
        use_charges("lighter", quantity);
        return;
    } else if (has_charges("flamethrower", quantity)) {
        use_charges("flamethrower", quantity);
        return;
    } else if (has_charges("flamethrower_simple", quantity)) {
        use_charges("flamethrower_simple", quantity);
        return;
    } else if (has_charges("hotplate", quantity)) {
        use_charges("welder", quantity);
        return;
    } else if (has_charges("welder", quantity)) {
        use_charges("welder", quantity);
        return;
    }
}

// does NOT return anything if the item is integrated toolset or fire!
std::list<item> player::use_charges(itype_id it, int quantity)
{
 std::list<item> ret;
 // the first two cases *probably* don't need to be tracked for now...
 if (it == "toolset") {
  power_level -= quantity;
  if (power_level < 0)
   power_level = 0;
  return ret;
 }
 if (it == "fire")
 {
     use_fire(quantity);
     return ret;
 }

// Start by checking weapon contents
 for (int i = 0; i < weapon.contents.size(); i++) {
  if (weapon.contents[i].type->id == it) {
   if (weapon.contents[i].charges > 0 &&
       weapon.contents[i].charges <= quantity) {
    ret.push_back(weapon.contents[i]);
    quantity -= weapon.contents[i].charges;
    if (weapon.contents[i].destroyed_at_zero_charges()) {
     weapon.contents.erase(weapon.contents.begin() + i);
     i--;
    } else
     weapon.contents[i].charges = 0;
    if (quantity == 0)
     return ret;
   } else {
    item tmp = weapon.contents[i];
    tmp.charges = quantity;
    ret.push_back(tmp);
    weapon.contents[i].charges -= quantity;
    return ret;
   }
  }
 }

 if (weapon.type->id == it) {
  if (weapon.charges > 0 && weapon.charges <= quantity) {
   ret.push_back(weapon);
   quantity -= weapon.charges;
   if (weapon.destroyed_at_zero_charges())
    remove_weapon();
   else
    weapon.charges = 0;
   if (quantity == 0)
    return ret;
   } else {
    item tmp = weapon;
    tmp.charges = quantity;
    ret.push_back(tmp);
    weapon.charges -= quantity;
    return ret;
   }
  }

 std::list<item> tmp = inv.use_charges(it, quantity);
 ret.splice(ret.end(), tmp);
 return ret;
}

int player::butcher_factor()
{
 int lowest_factor = 999;
 if (has_bionic("bio_tools"))
 	lowest_factor=100;
 int inv_factor = inv.butcher_factor();
 if (inv_factor < lowest_factor) {
  lowest_factor = inv_factor;
 }
 if (weapon.damage_cut() >= 10 && !weapon.has_flag("SPEAR")) {
  int factor = weapon.volume() * 5 - weapon.weight() * 1.5 -
               weapon.damage_cut();
  if (weapon.damage_cut() <= 20)
   factor *= 2;
  if (factor < lowest_factor)
   lowest_factor = factor;
 }
 return lowest_factor;
}

item* player::pick_usb()
{
 std::vector<item*> drives = inv.all_items_by_type("usb_drive");

 if (drives.empty())
  return NULL; // None available!

 std::vector<std::string> selections;
 for (int i = 0; i < drives.size() && i < 9; i++)
  selections.push_back( drives[i]->tname() );

 int select = menu_vec(false, _("Choose drive:"), selections);

 return drives[ select - 1 ];
}

bool player::is_wearing(itype_id it)
{
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].type->id == it)
   return true;
 }
 return false;
}

bool player::worn_with_flag( std::string flag ) const
{
    for (int i = 0; i < worn.size(); i++) {
        if (worn[i].has_flag( flag )) {
            return true;
        }
    }
    return false;
}

bool player::has_artifact_with(art_effect_passive effect)
{
 if (weapon.is_artifact() && weapon.is_tool()) {
  it_artifact_tool *tool = dynamic_cast<it_artifact_tool*>(weapon.type);
  for (int i = 0; i < tool->effects_wielded.size(); i++) {
   if (tool->effects_wielded[i] == effect)
    return true;
  }
  for (int i = 0; i < tool->effects_carried.size(); i++) {
   if (tool->effects_carried[i] == effect)
    return true;
  }
 }
 if (inv.has_artifact_with(effect)) {
  return true;
 }
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].is_artifact()) {
   it_artifact_armor *armor = dynamic_cast<it_artifact_armor*>(worn[i].type);
   for (int i = 0; i < armor->effects_worn.size(); i++) {
    if (armor->effects_worn[i] == effect)
     return true;
   }
  }
 }
 return false;
}

bool player::has_amount(itype_id it, int quantity)
{
    if (it == "toolset")
    {
        return has_bionic("bio_tools");
    }
    return (amount_of(it) >= quantity);
}

int player::amount_of(itype_id it)
{
    if (it == "toolset" && has_bionic("bio_tools"))
    {
        return 1;
    }
    if (it == "apparatus")
    {
        if (has_amount("crackpipe", 1) || has_amount("can_drink", 1) )
        {
            return 1;
        }
    }
    int quantity = 0;
    if (weapon.type->id == it)
    {
        quantity++;
    }

    for (int i = 0; i < weapon.contents.size(); i++)
    {
        if (weapon.contents[i].type->id == it)
        {
            quantity++;
        }
    }
    quantity += inv.amount_of(it);
    return quantity;
}

bool player::has_charges(itype_id it, int quantity)
{
    if (it == "fire" || it == "apparatus")
    {
        return has_fire(quantity);
    }
    return (charges_of(it) >= quantity);
}

int player::charges_of(itype_id it)
{
 if (it == "toolset") {
  if (has_bionic("bio_tools"))
   return power_level;
  else
   return 0;
 }
 int quantity = 0;
 if (weapon.type->id == it)
  quantity += weapon.charges;
 for (int i = 0; i < weapon.contents.size(); i++) {
  if (weapon.contents[i].type->id == it)
   quantity += weapon.contents[i].charges;
 }
 quantity += inv.charges_of(it);
 return quantity;
}

bool player::has_watertight_container()
{
 if (!inv.watertight_container().is_null()) {
  return true;
 }
 if (weapon.is_container() && weapon.contents.empty()) {
   if (weapon.has_flag("WATERTIGHT") && weapon.has_flag("SEALS"))
    return true;
 }

 return false;
}

bool player::has_matching_liquid(itype_id it)
{
 if (inv.has_liquid(it)) {
  return true;
 }
 if (weapon.is_container() && !weapon.contents.empty()) {
  if (weapon.contents[0].type->id == it) { // liquid matches
    it_container* container = dynamic_cast<it_container*>(weapon.type);
    int holding_container_charges;

    if (weapon.contents[0].type->is_food()) {
      it_comest* tmp_comest = dynamic_cast<it_comest*>(weapon.contents[0].type);

      if (tmp_comest->add == ADD_ALCOHOL) // 1 contains = 20 alcohol charges
        holding_container_charges = container->contains * 20;
      else
        holding_container_charges = container->contains;
    }
    else if (weapon.contents[0].type->is_ammo())
      holding_container_charges = container->contains * 200;
    else
      holding_container_charges = container->contains;

  if (weapon.contents[0].charges < holding_container_charges)
    return true;
  }
}

 return false;
}

bool player::has_weapon_or_armor(char let) const
{
 if (weapon.invlet == let)
  return true;
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].invlet == let)
   return true;
 }
 return false;
}

bool player::has_item_with_flag( std::string flag ) const
{
    //check worn items for flag
    if (worn_with_flag( flag ))
    {
        return true;
    }

    //check weapon for flag
    if (weapon.has_flag( flag ))
    {
        return true;
    }

    //check inventory items for flag
    if (inv.has_flag( flag ))
    {
        return true;
    }

    return false;
}

bool player::has_item(char let)
{
 return (has_weapon_or_armor(let) || !inv.item_by_letter(let).is_null());
}

bool player::has_item(item *it)
{
 if (it == &weapon)
  return true;
 for (int i = 0; i < worn.size(); i++) {
  if (it == &(worn[i]))
   return true;
 }
 return inv.has_item(it);
}

bool player::has_mission_item(int mission_id)
{
    if (mission_id == -1)
    {
        return false;
    }
    if (weapon.mission_id == mission_id)
    {
        return true;
    }
    for (int i = 0; i < weapon.contents.size(); i++)
    {
        if (weapon.contents[i].mission_id == mission_id)
        return true;
    }
    if (inv.has_mission_item(mission_id))
    {
        return true;
    }
    return false;
}

char player::lookup_item(char let)
{
 if (weapon.invlet == let)
  return -1;

 if (inv.item_by_letter(let).is_null())
  return -2; // -2 is for "item not found"

 return let;
}

hint_rating player::rate_action_eat(item *it)
{
 //TODO more cases, for HINT_IFFY
 if (it->is_food_container() || it->is_food()) {
  return HINT_GOOD;
 }
 return HINT_CANT;
}

bool player::eat(game *g, signed char ch)
{
    it_comest *comest = NULL;
    item *eaten = NULL;
    int which = -3; // Helps us know how to delete the item which got eaten
    if (ch == -2)
    {
        g->add_msg(_("You do not have that item."));
        return false;
    }
    else if (ch == -1)
    {
        if (weapon.is_food_container(this))
        {
            eaten = &weapon.contents[0];
            which = -2;
            if (weapon.contents[0].is_food())
                comest = dynamic_cast<it_comest*>(weapon.contents[0].type);
        }
        else if (weapon.is_food(this))
        {
            eaten = &weapon;
            which = -1;
            if (weapon.is_food())
                comest = dynamic_cast<it_comest*>(weapon.type);
        }
        else
        {
            g->add_msg_if_player(this,_("You can't eat your %s."), weapon.tname(g).c_str());
            if(is_npc())
                debugmsg("%s tried to eat a %s", name.c_str(), weapon.tname(g).c_str());
            return false;
        }
    }
    else
    {
        item& it = inv.item_by_letter(ch);
        if (it.is_food_container(this))
        {
            eaten = &(it.contents[0]);
            which = 1;
            if (it.contents[0].is_food())
                comest = dynamic_cast<it_comest*>(it.contents[0].type);
        }
        else if (it.is_food(this))
        {
            eaten = &it;
            which = 0;
            if (it.is_food())
                comest = dynamic_cast<it_comest*>(it.type);
        }
        else
        {
            g->add_msg_if_player(this,_("You can't eat your %s."), it.tname(g).c_str());
            if(is_npc())
                debugmsg("%s tried to eat a %s", name.c_str(), it.tname(g).c_str());
            return false;
        }
    if (eaten == NULL)
        return false;

    if (eaten->is_ammo())   // For when bionics let you eat fuel
    {
        const int factor = 20;
        int max_change = max_power_level - power_level;
        if (max_change == 0)
            g->add_msg_if_player(this,_("Your internal power storage is fully powered."));
        charge_power(eaten->charges / factor);
        eaten->charges -= max_change * factor; //negative charges seem to be okay
        eaten->charges++; //there's a flat subtraction later
    }
    else if (!eaten->type->is_food() && !eaten->is_food_container(this))
    {
            // For when bionics let you burn organic materials
        if (eaten->type->is_book()) {
            it_book* book = dynamic_cast<it_book*>(eaten->type);
            if (book->type != NULL && !query_yn(_("Really eat %s?"), book->name.c_str()))
                return false;
        }
        int charge = (eaten->volume() + eaten->weight()) / 2;
        if (eaten->type->m1 == "leather" || eaten->type->m2 == "leather")
            charge /= 4;
        if (eaten->type->m1 == "wood"    || eaten->type->m2 == "wood")
            charge /= 2;
        charge_power(charge);
    }
    else     // It's real food!  i.e. an it_comest
    {
        // Remember, comest points to the it_comest data
        if (comest == NULL)
        {
            debugmsg("player::eat(%s); comest is NULL!", eaten->tname(g).c_str());
            return false;
        }
        if (comest->tool != "null")
        {
            bool has = has_amount(comest->tool, 1);
            if (g->itypes[comest->tool]->count_by_charges())
                has = has_charges(comest->tool, 1);
            if (!has) {
                g->add_msg_if_player(this,_("You need a %s to consume that!"),
                           g->itypes[comest->tool]->name.c_str());
            return false;
            }
        }
        bool overeating = (!has_trait(PF_GOURMAND) && hunger < 0 &&
                           comest->nutr >= 5);
        bool spoiled = eaten->rotten(g);

        last_item = itype_id(eaten->type->id);

        if (overeating && !is_npc() &&
                !query_yn(_("You're full.  Force yourself to eat?")))
            return false;

        if (has_trait(PF_CARNIVORE) && eaten->made_of("veggy") && comest->nutr > 0)
        {
            g->add_msg_if_player(this, _("You can't stand the thought of eating veggies."));
            return false;
        }
        if (!has_trait(PF_CANNIBAL) && eaten->made_of("hflesh")&& !is_npc() &&
                !query_yn(_("The thought of eating that makes you feel sick. Really do it?")))
            return false;

        if (has_trait(PF_VEGETARIAN) && eaten->made_of("flesh") && !is_npc() &&
                !query_yn(_("Really eat that meat? Your stomach won't be happy.")))
            return false;

        if (spoiled)
        {
            if (is_npc())
                return false;
            if (!has_trait(PF_SAPROVORE) &&
                    !query_yn(_("This %s smells awful!  Eat it?"), eaten->tname(g).c_str()))
                return false;
            g->add_msg(_("Ick, this %s doesn't taste so good..."),eaten->tname(g).c_str());
            if (!has_trait(PF_SAPROVORE) && (!has_bionic("bio_digestion") || one_in(3)))
                add_disease("foodpoison", rng(60, (comest->nutr + 1) * 60));
            hunger -= rng(0, comest->nutr);
            thirst -= comest->quench;
            if (!has_trait(PF_SAPROVORE) && !has_bionic("bio_digestion"))
                health -= 3;
        }
        else
        {
            hunger -= comest->nutr;
            thirst -= comest->quench;
            if (has_bionic("bio_digestion"))
                hunger -= rng(0, comest->nutr);
            else if (!has_trait(PF_GOURMAND))
            {
                if ((overeating && rng(-200, 0) > hunger))
                    vomit(g);
            }
            health += comest->healthy;
        }
        // At this point, we've definitely eaten the item, so use up some turns.
        if (has_trait(PF_GOURMAND))
            moves -= 150;
        else
            moves -= 250;
        // If it's poisonous... poison us.  TODO: More several poison effects
        if (eaten->poison >= rng(2, 4))
            add_disease("poison", eaten->poison * 100);
        if (eaten->poison > 0)
            add_disease("foodpoison", eaten->poison * 300);

        if (comest->comesttype == "DRINK") {
            g->add_msg_player_or_npc( this, _("You drink your %s."), _("<npcname> drinks a %s."),
                                      eaten->tname(g).c_str());
        }
        else if (comest->comesttype == "FOOD") {
            g->add_msg_player_or_npc( this, _("You eat your %s."), _("<npcname> eats a %s."),
                                      eaten->tname(g).c_str());
        }
    }

        if (g->itypes[comest->tool]->is_tool())
            use_charges(comest->tool, 1); // Tools like lighters get used
        if (comest->stim > 0)
        {
            if (comest->stim < 10 && stim < comest->stim)
            {
                stim += comest->stim;
                if (stim > comest->stim)
                    stim = comest->stim;
            }
            else if (comest->stim >= 10 && stim < comest->stim * 3)
                stim += comest->stim;
        }

        iuse use;
        if (comest->use != &iuse::none)
        {
            (use.*comest->use)(g, this, eaten, false);
        }
        add_addiction(comest->add, comest->addict);
        if (addiction_craving(comest->add) != MORALE_NULL)
            rem_morale(addiction_craving(comest->add));

        if (has_bionic("bio_ethanol") && comest->use == &iuse::alcohol)
            charge_power(rng(2, 8));
        if (has_bionic("bio_ethanol") && comest->use == &iuse::alcohol_weak)
            charge_power(rng(1, 4));

        if (eaten->made_of("hflesh")) {
          if (has_trait(PF_CANNIBAL)) {
              g->add_msg_if_player(this, _("You feast upon the human flesh."));
              add_morale(MORALE_CANNIBAL, 15, 100);
          } else {
              g->add_msg_if_player(this, _("You feel horrible for eating a person.."));
              add_morale(MORALE_CANNIBAL, -60, -400, 600, 300);
          }
        }
        if (has_trait(PF_VEGETARIAN) && (eaten->made_of("flesh") || eaten->made_of("hflesh")))
        {
            g->add_msg_if_player(this,_("Almost instantly you feel a familiar pain in your stomach"));
            add_morale(MORALE_VEGETARIAN, -75, -400, 300, 240);
        }
        if ((has_trait(PF_HERBIVORE) || has_trait(PF_RUMINANT)) &&
                eaten->made_of("flesh"))
        {
            if (!one_in(3))
                vomit(g);
            if (comest->quench >= 2)
                thirst += int(comest->quench / 2);
            if (comest->nutr >= 2)
                hunger += int(comest->nutr * .75);
        }
        if (eaten->has_flag("HOT") && eaten->has_flag("EATEN_HOT"))
            add_morale(MORALE_FOOD_HOT, 5, 10);
        if (has_trait(PF_GOURMAND))
        {
            if (comest->fun < -2)
                add_morale(MORALE_FOOD_BAD, comest->fun * 2, comest->fun * 4, 60, 30, comest);
            else if (comest->fun > 0)
                add_morale(MORALE_FOOD_GOOD, comest->fun * 3, comest->fun * 6, 60, 30, comest);
            if (hunger < -60 || thirst < -60)
                g->add_msg_if_player(this,_("You can't finish it all!"));
            if (hunger < -60)
                hunger = -60;
            if (thirst < -60)
                thirst = -60;
        }
        else
        {
            if (comest->fun < 0)
                add_morale(MORALE_FOOD_BAD, comest->fun * 2, comest->fun * 6, 60, 30, comest);
            else if (comest->fun > 0)
                add_morale(MORALE_FOOD_GOOD, comest->fun * 2, comest->fun * 4, 60, 30, comest);
            if (hunger < -20 || thirst < -20)
                g->add_msg_if_player(this,_("You can't finish it all!"));
            if (hunger < -20)
                hunger = -20;
            if (thirst < -20)
                thirst = -20;
        }
    }

    eaten->charges--;
    if (eaten->charges <= 0)
    {
        if (which == -1)
            weapon = get_combat_style();
        else if (which == -2)
        {
            weapon.contents.erase(weapon.contents.begin());
            g->add_msg_if_player(this,_("You are now wielding an empty %s."), weapon.tname(g).c_str());
        }
        else if (which == 0)
            inv.remove_item_by_letter(ch);
        else if (which >= 0)
        {
            item& it = inv.item_by_letter(ch);
            it.contents.erase(it.contents.begin());
            if (!is_npc())
            {
                switch ((int)OPTIONS[OPT_DROP_EMPTY])
                {
                case 0:
                    g->add_msg(_("%c - an empty %s"), it.invlet,
                               it.tname(g).c_str());
                    break;
                case 1:
                    if (it.is_container())
                    {
                        if (!(it.has_flag("WATERTIGHT") && it.has_flag("SEALS")))
                        {
                            g->add_msg(_("You drop the empty %s."), it.tname(g).c_str());
                            g->m.add_item_or_charges(posx, posy, inv.remove_item_by_letter(it.invlet));
                        }
                        else
                            g->add_msg(_("%c - an empty %s"), it.invlet,
                                       it.tname(g).c_str());
                    }
                    else if (it.type->id == "wrapper") // hack because wrappers aren't containers
                    {
                        g->add_msg(_("You drop the empty %s."), it.tname(g).c_str());
                        g->m.add_item_or_charges(posx, posy, inv.remove_item_by_letter(it.invlet));
                    }
                    break;
                case 2:
                    g->add_msg(_("You drop the empty %s."), it.tname(g).c_str());
                    g->m.add_item_or_charges(posx, posy, inv.remove_item_by_letter(it.invlet));
                    break;
                }
            }
            if (inv.stack_by_letter(it.invlet).size() > 0)
                inv.restack(this);
            inv.unsort();
        }
    }
    return true;
}

bool player::wield(game *g, signed char ch, bool autodrop)
{
 if (weapon.has_flag("NO_UNWIELD")) {
  g->add_msg(_("You cannot unwield your %s!  Withdraw them with 'p'."),
             weapon.tname().c_str());
  return false;
 }
 if (ch == -3) {
  bool pickstyle = (!styles.empty());
  if (weapon.is_style())
   remove_weapon();
  else if (!is_armed()) {
   if (!pickstyle) {
    g->add_msg(_("You are already wielding nothing."));
    return false;
   }
  } else if (autodrop || volume_carried() + weapon.volume() < volume_capacity()) {
   inv.push_back(remove_weapon());
   inv.unsort();
   moves -= 20;
   recoil = 0;
   if (!pickstyle)
    return true;
  } else if (query_yn(_("No space in inventory for your %s.  Drop it?"),
                      weapon.tname(g).c_str())) {
   g->m.add_item_or_charges(posx, posy, remove_weapon());
   recoil = 0;
   if (!pickstyle)
    return true;
  } else
   return false;

  if (pickstyle) {
   weapon = get_combat_style();
   return true;
  }
 }
 if (ch == 0) {
  g->add_msg(_("You're already wielding that!"));
  return false;
 } else if (ch == -2) {
  g->add_msg(_("You don't have that item."));
  return false;
 }

 item& it = inv.item_by_letter(ch);
 if (it.is_two_handed(this) && !has_two_arms()) {
  g->add_msg(_("You cannot wield a %s with only one arm."),
             it.tname(g).c_str());
  return false;
 }
 if (!is_armed()) {
  weapon = inv.remove_item_by_letter(ch);
  if (weapon.is_artifact() && weapon.is_tool()) {
   it_artifact_tool *art = dynamic_cast<it_artifact_tool*>(weapon.type);
   g->add_artifact_messages(art->effects_wielded);
  }
  moves -= 30;
  last_item = itype_id(weapon.type->id);
  return true;
 } else if (volume_carried() + weapon.volume() - it.volume() <
            volume_capacity()) {
  item tmpweap = remove_weapon();
  weapon = inv.remove_item_by_letter(ch);
  inv.push_back(tmpweap);
  inv.unsort();
  moves -= 45;
  if (weapon.is_artifact() && weapon.is_tool()) {
   it_artifact_tool *art = dynamic_cast<it_artifact_tool*>(weapon.type);
   g->add_artifact_messages(art->effects_wielded);
  }
  last_item = itype_id(weapon.type->id);
  return true;
 } else if (query_yn(_("No space in inventory for your %s.  Drop it?"),
                     weapon.tname(g).c_str())) {
  g->m.add_item_or_charges(posx, posy, remove_weapon());
  weapon = it;
  inv.remove_item_by_letter(weapon.invlet);
  inv.unsort();
  moves -= 30;
  if (weapon.is_artifact() && weapon.is_tool()) {
   it_artifact_tool *art = dynamic_cast<it_artifact_tool*>(weapon.type);
   g->add_artifact_messages(art->effects_wielded);
  }
  last_item = itype_id(weapon.type->id);
  return true;
 }

 return false;

}

void player::pick_style(game *g) // Style selection menu
{
 std::vector<std::string> options;
 options.push_back(_("No style"));
 for (int i = 0; i < styles.size(); i++) {
  if(!g->itypes[styles[i]]) {
    debugmsg ("Bad hand to hand style: %d",i);
  } else {
    options.push_back( g->itypes[styles[i]]->name );
  }
 }
 int selection = menu_vec(false, _("Select a style"), options);
 if (selection >= 2)
  style_selected = styles[selection - 2];
 else
  style_selected = "null";
}

hint_rating player::rate_action_wear(item *it)
{
 //TODO flag already-worn items as HINT_IFFY

 if (!it->is_armor()) {
  return HINT_CANT;
 }

 it_armor* armor = dynamic_cast<it_armor*>(it->type);

 // are we trying to put on power armor? If so, make sure we don't have any other gear on.
 if (armor->is_power_armor() && worn.size()) {
  if (armor->covers & mfb(bp_torso)) {
   return HINT_IFFY;
  } else if (armor->covers & mfb(bp_head) && !((it_armor *)worn[0].type)->is_power_armor()) {
   return HINT_IFFY;
  }
 }
 // are we trying to wear something over power armor? We can't have that, unless it's a backpack, or similar.
 if (worn.size() && ((it_armor *)worn[0].type)->is_power_armor() && !(armor->covers & mfb(bp_head))) {
  if (!(armor->covers & mfb(bp_torso) && armor->color == c_green)) {
   return HINT_IFFY;
  }
 }

 // Make sure we're not wearing 2 of the item already
 int count = 0;
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].type->id == it->type->id)
   count++;
 }
 if (count == 2) {
  return HINT_IFFY;
 }
 if (has_trait(PF_WOOLALLERGY) && it->made_of("wool")) {
  return HINT_IFFY; //should this be HINT_CANT? I kinda think not, because HINT_CANT is more for things that can NEVER happen
 }
 if (armor->covers & mfb(bp_head) && encumb(bp_head) != 0) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_hands) && has_trait(PF_WEBBED)) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_hands) && has_trait(PF_TALONS)) {
  return HINT_IFFY;
 }
 if ( armor->covers & mfb(bp_hands) && (has_trait(PF_ARM_TENTACLES)
        || has_trait(PF_ARM_TENTACLES_4) || has_trait(PF_ARM_TENTACLES_8)) ) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_mouth) && has_trait(PF_BEAK)) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_feet) && has_trait(PF_HOOVES)) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_feet) && has_trait(PF_LEG_TENTACLES)) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_head) && has_trait(PF_HORNS_CURLED)) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_torso) && has_trait(PF_SHELL)) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_head) && !it->made_of("wool") &&
     !it->made_of("cotton") && !it->made_of("leather") &&
     (has_trait(PF_HORNS_POINTED) || has_trait(PF_ANTENNAE) ||
      has_trait(PF_ANTLERS))) {
  return HINT_IFFY;
 }
 // Checks to see if the player is wearing not cotton or not wool, ie leather/plastic shoes
 if (armor->covers & mfb(bp_feet) && wearing_something_on(bp_feet) && !(it->made_of("wool") || it->made_of("cotton"))) {
  for (int i = 0; i < worn.size(); i++) {
   item *worn_item = &worn[i];
   it_armor *worn_armor = dynamic_cast<it_armor*>(worn_item->type);
   if( worn_armor->covers & mfb(bp_feet) && !(worn_item->made_of("wool") || worn_item->made_of("cotton"))) {
    return HINT_IFFY;
   }
  }
 }
 return HINT_GOOD;
}

bool player::wear(game *g, char let, bool interactive)
{
    item* to_wear = NULL;
    int index = -1;
    if (weapon.invlet == let)
    {
        to_wear = &weapon;
        index = -2;
    }
    else
    {
        to_wear = &inv.item_by_letter(let);
    }

    if (to_wear == NULL)
    {
        if(interactive)
        {
            g->add_msg(_("You don't have item '%c'."), let);
        }

        return false;
    }

    if (!wear_item(g, to_wear, interactive))
    {
        return false;
    }

    if (index == -2)
    {
        weapon = get_combat_style();
    }
    else
    {
        inv.remove_item(to_wear);
    }

    return true;
}

bool player::wear_item(game *g, item *to_wear, bool interactive)
{
    it_armor* armor = NULL;

    if (to_wear->is_armor())
    {
        armor = dynamic_cast<it_armor*>(to_wear->type);
    }
    else
    {
        g->add_msg(_("Putting on a %s would be tricky."), to_wear->tname(g).c_str());
        return false;
    }

    // are we trying to put on power armor? If so, make sure we don't have any other gear on.
    if (armor->is_power_armor())
    {
        for (std::vector<item>::iterator it = worn.begin(); it != worn.end(); it++)
        {
            if ((dynamic_cast<it_armor*>(it->type))->covers & armor->covers)
            {
                if(interactive)
                {
                    g->add_msg(_("You can't wear power armor over other gear!"));
                }
                return false;
            }
        }

        if (!(armor->covers & mfb(bp_torso)))
        {
            bool power_armor = false;

            if (worn.size())
            {
                for (std::vector<item>::iterator it = worn.begin(); it != worn.end(); it++)
                {
                    if (dynamic_cast<it_armor*>(it->type)->power_armor)
                    {
                        power_armor = true;
                        break;
                    }
                }
            }

            if (!power_armor)
            {
                if(interactive)
                {
                    g->add_msg(_("You can only wear power armor components with power armor!"));
                }
                return false;
            }
        }

        for (int i = 0; i < worn.size(); i++)
        {
            if (((it_armor *)worn[i].type)->is_power_armor() && worn[i].type == armor)
            {
                if(interactive)
                {
                    g->add_msg(_("You cannot wear more than one %s!"), to_wear->tname().c_str());
                }
                return false;
            }
        }
    }
    else
    {
        // Only helmets can be worn with power armor, except other power armor components
        if (worn.size() && ((it_armor *)worn[0].type)->is_power_armor() && !(armor->covers & (mfb(bp_head) | mfb(bp_eyes))))
        {
            if(interactive)
            {
                g->add_msg(_("You can't wear %s with power armor!"), to_wear->tname().c_str());
            }
            return false;
        }
    }

    if (!to_wear->has_flag("OVERSIZE"))
    {
        // Make sure we're not wearing 2 of the item already
        int count = 0;

        for (int i = 0; i < worn.size(); i++)
        {
            if (worn[i].type->id == to_wear->type->id)
            {
                count++;
            }
        }

        if (count == 2)
        {
            if(interactive)
            {
                g->add_msg(_("You can't wear more than two %s at once."), to_wear->tname().c_str());
            }
            return false;
        }

        if (has_trait(PF_WOOLALLERGY) && to_wear->made_of("wool"))
        {
            if(interactive)
            {
                g->add_msg(_("You can't wear that, it's made of wool!"));
            }
            return false;
        }

        if (armor->covers & mfb(bp_head) && encumb(bp_head) != 0)
        {
            if(interactive)
            {
                g->add_msg(wearing_something_on(bp_head) ? _("You can't wear another helmet!") : _("You can't wear a helmet!"));
            }
            return false;
        }

        if (armor->covers & mfb(bp_hands) && has_trait(PF_WEBBED))
        {
            if(interactive)
            {
                g->add_msg(_("You cannot put %s over your webbed hands."), armor->name.c_str());
            }
            return false;
        }

        if ( armor->covers & mfb(bp_hands) && (has_trait(PF_ARM_TENTACLES) || has_trait(PF_ARM_TENTACLES_4) || has_trait(PF_ARM_TENTACLES_8)) )
        {
            if(interactive)
            {
                g->add_msg(_("You cannot put %s over your tentacles."), armor->name.c_str());
            }
            return false;
        }

        if (armor->covers & mfb(bp_hands) && has_trait(PF_TALONS))
        {
            if(interactive)
            {
                g->add_msg(_("You cannot put %s over your talons."), armor->name.c_str());
            }
            return false;
        }

        if (armor->covers & mfb(bp_mouth) && has_trait(PF_BEAK))
        {
            if(interactive)
            {
                g->add_msg(_("You cannot put a %s over your beak."), armor->name.c_str());
            }
            return false;
        }

        if (armor->covers & mfb(bp_feet) && has_trait(PF_HOOVES))
        {
            if(interactive)
            {
                g->add_msg(_("You cannot wear footwear on your hooves."));
            }
            return false;
        }

        if (armor->covers & mfb(bp_feet) && has_trait(PF_LEG_TENTACLES))
        {
            if(interactive)
            {
                g->add_msg(_("You cannot wear footwear on your tentacles."));
            }
            return false;
        }

        if (armor->covers & mfb(bp_head) && has_trait(PF_HORNS_CURLED))
        {
            if(interactive)
            {
                g->add_msg(_("You cannot wear headgear over your horns."));
            }
            return false;
        }

        if (armor->covers & mfb(bp_torso) && has_trait(PF_SHELL))
        {
            if(interactive)
            {
                g->add_msg(_("You cannot wear anything over your shell."));
            }
            return false;
        }

        if (armor->covers & mfb(bp_head) && !to_wear->made_of("wool") && !to_wear->made_of("cotton") && !to_wear->made_of("leather") && (has_trait(PF_HORNS_POINTED) || has_trait(PF_ANTENNAE) || has_trait(PF_ANTLERS)))
        {
            if(interactive)
            {
                g->add_msg(_("You cannot wear a helmet over your %s."), (has_trait(PF_HORNS_POINTED) ? _("horns") : (has_trait(PF_ANTENNAE) ? _("antennae") : _("antlers"))));
            }
            return false;
        }

        // Checks to see if the player is wearing not cotton or not wool, ie leather/plastic shoes
        if (armor->covers & mfb(bp_feet) && wearing_something_on(bp_feet) && !(to_wear->made_of("wool") || to_wear->made_of("cotton")))
        {
            for (int i = 0; i < worn.size(); i++)
            {
                item *worn_item = &worn[i];
                it_armor *worn_armor = dynamic_cast<it_armor*>(worn_item->type);

                if (worn_armor->covers & mfb(bp_feet) && !(worn_item->made_of("wool") || worn_item->made_of("cotton")))
                {
                    if(interactive)
                    {
                        g->add_msg(_("You're already wearing footwear!"));
                    }
                    return false;
                }
            }
        }
    }

    last_item = itype_id(to_wear->type->id);
    worn.push_back(*to_wear);

    if(interactive)
    {
        g->add_msg(_("You put on your %s."), to_wear->tname(g).c_str());
        moves -= 350; // TODO: Make this variable?

        if (to_wear->is_artifact())
        {
            it_artifact_armor *art = dynamic_cast<it_artifact_armor*>(to_wear->type);
            g->add_artifact_messages(art->effects_worn);
        }

        for (body_part i = bp_head; i < num_bp; i = body_part(i + 1))
        {
            if (armor->covers & mfb(i) && encumb(i) >= 4)
            {
                g->add_msg(
                    (i == bp_head || i == bp_torso) ? 
                    _("Your %s is very encumbered! %s"):_("Your %s are very encumbered! %s"),
                    body_part_name(body_part(i), 2).c_str(), encumb_text(body_part(i)).c_str());
            }
        }
    }

    return true;
}

hint_rating player::rate_action_takeoff(item *it) {
 if (!it->is_armor()) {
  return HINT_CANT;
 }

 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].invlet == it->invlet) { //surely there must be an easier way to do this?
   return HINT_GOOD;
  }
 }

 return HINT_IFFY;
}

bool player::takeoff(game *g, char let, bool autodrop)
{
 if (weapon.invlet == let) {
     return wield(g, -3, autodrop);
 } else {
  for (int i = 0; i < worn.size(); i++) {
   if (worn[i].invlet == let) {
     if ((dynamic_cast<it_armor*>(worn[i].type))->is_power_armor() &&
         ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp_torso))) {
       // We're trying to take off power armor, but cannot do that if we have a power armor component on!
       bool removed_armor = false;
       for (int j = 0; j < worn.size(); j++) {
         if ((dynamic_cast<it_armor*>(worn[j].type))->is_power_armor() &&
             (worn[j].invlet != let)) {
             if( autodrop ) {
                 g->m.add_item_or_charges(posx, posy, worn[j]);
                 worn.erase(worn.begin() + j);
                 removed_armor = true;
             } else {
                 g->add_msg(_("You can't take off power armor while wearing other power armor components."));
                 return false;
             }
         }
       }
       if( removed_armor ) {
           // We've invalidated our index into worn[], so rescan from the beginning.
           i = -1;
           continue;
       }
     }
    if (autodrop || volume_capacity() - (dynamic_cast<it_armor*>(worn[i].type))->storage >
        volume_carried() + worn[i].type->volume) {
     inv.push_back(worn[i]);
     worn.erase(worn.begin() + i);
     inv.unsort();
     return true;
    } else if (query_yn(_("No room in inventory for your %s.  Drop it?"),
                        worn[i].tname(g).c_str())) {
     g->m.add_item_or_charges(posx, posy, worn[i]);
     worn.erase(worn.begin() + i);
     return true;
    } else
     return false;
   }
  }
  g->add_msg(_("You are not wearing that item."));
  return false;
 }
}

void player::sort_armor(game *g)
{
    if (worn.size() == 0)
    {
        g->add_msg(_("You are not wearing anything!"));
        return;
    }

    // work out required window sizes
    int info_win_y = 5;
    int arm_info_win_y = 4;
    int worn_win_y = worn.size() + 5;

    int torso_win_y = 3;
    int arms_win_y = 3;
    int hands_win_y = 3;
    int legs_win_y = 3;
    int eyes_win_y = 3;
    int mouth_win_y = 3;

    // color array for damae
    nc_color dam_color[] = {c_green, c_ltgreen, c_yellow, c_magenta, c_ltred, c_red};

    for (int i = 0; i < worn.size(); i++)
    {
        if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp_torso))
            torso_win_y++;
        if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp_arms))
            arms_win_y++;
        if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp_hands))
            hands_win_y++;
        if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp_legs))
            legs_win_y++;
        if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp_eyes))
            eyes_win_y++;
        if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp_mouth))
            mouth_win_y++;
    }

    int iCol1WinX = 28;
    int iCol2WinX = 26;
    int iCol3WinX = 26;

    int iCenterOffsetX = TERMX/2 - FULL_SCREEN_WIDTH/2;
    int iCenterOffsetY = TERMY/2 - (info_win_y + arm_info_win_y + worn_win_y)/2;

    WINDOW* w_info       = newwin(info_win_y, FULL_SCREEN_WIDTH, iCenterOffsetY + VIEW_OFFSET_Y, 0 + VIEW_OFFSET_X + iCenterOffsetX);
    WINDOW* w_arm_info   = newwin(arm_info_win_y, FULL_SCREEN_WIDTH,  iCenterOffsetY + info_win_y - 1 + VIEW_OFFSET_Y, 0 + VIEW_OFFSET_X + iCenterOffsetX);
    WINDOW* w_all_worn   = newwin(worn_win_y, iCol1WinX, iCenterOffsetY + arm_info_win_y + info_win_y - 2 + VIEW_OFFSET_Y,  0 + VIEW_OFFSET_X + iCenterOffsetX);
    WINDOW* w_torso_worn = newwin(torso_win_y, iCol2WinX, iCenterOffsetY + arm_info_win_y + info_win_y - 2 + VIEW_OFFSET_Y, iCol1WinX + VIEW_OFFSET_X + iCenterOffsetX);
    WINDOW* w_arms_worn  = newwin(arms_win_y, iCol2WinX, iCenterOffsetY + arm_info_win_y + info_win_y - 3 + torso_win_y + VIEW_OFFSET_Y, iCol1WinX + VIEW_OFFSET_X + iCenterOffsetX);
    WINDOW* w_hands_worn = newwin(hands_win_y, iCol2WinX, iCenterOffsetY + arm_info_win_y + info_win_y - 4 + torso_win_y + arms_win_y  + VIEW_OFFSET_Y, iCol1WinX + VIEW_OFFSET_X + iCenterOffsetX);
    WINDOW* w_legs_worn  = newwin(legs_win_y, iCol3WinX, iCenterOffsetY + arm_info_win_y + info_win_y - 2 + VIEW_OFFSET_Y, iCol1WinX + iCol2WinX + VIEW_OFFSET_X + iCenterOffsetX);
    WINDOW* w_eyes_worn  = newwin(eyes_win_y, iCol3WinX, iCenterOffsetY + arm_info_win_y + info_win_y - 3 + legs_win_y + VIEW_OFFSET_Y, iCol1WinX + iCol2WinX + VIEW_OFFSET_X + iCenterOffsetX);
    WINDOW* w_mouth_worn = newwin(mouth_win_y, iCol3WinX, iCenterOffsetY + arm_info_win_y + info_win_y - 4 + legs_win_y + eyes_win_y  + VIEW_OFFSET_Y, iCol1WinX + iCol2WinX + VIEW_OFFSET_X + iCenterOffsetX);

    wborder(w_info, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
    wborder(w_arm_info, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_XXXO, LINE_XOXX, LINE_XXXO, LINE_XOXX );
    wborder(w_all_worn, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_XXXO, LINE_OXXX, LINE_XXOO, LINE_XOOX );
    wborder(w_torso_worn, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXX, LINE_OXXX, LINE_XXXO, LINE_XOXX);
    wborder(w_eyes_worn, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_XXXO, LINE_XOXX, LINE_XXXO, LINE_XOXX);
    wborder(w_mouth_worn, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_XXXO, LINE_XOXX, LINE_XXOO, LINE_XOOX);
    wborder(w_arms_worn, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_XXXO, LINE_XOXX, LINE_XXXO, LINE_XOXX);
    wborder(w_hands_worn, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_XXXO, LINE_XOXX, LINE_XXOO, LINE_XOOX);
    wborder(w_legs_worn, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXX, LINE_XOXX, LINE_XXXO, LINE_XOXX);

    // Print name and header
    mvwprintz(w_info, 1, 1, c_white, _("CLOTHING SORTING        Press jk to move up/down, s to select items to move."));
    mvwprintz(w_info, 2, 1, c_white, _("Press r to assign special inventory letters to clothing, ESC or q to return."));
    mvwprintz(w_info, 3, 1, c_white, _("Color key: "));
    wprintz(w_info, c_ltgray, _("(Reinforced) "));
    wprintz(w_info, dam_color[0], "# ");
    wprintz(w_info, dam_color[1], "# ");
    wprintz(w_info, dam_color[2], "# ");
    wprintz(w_info, dam_color[3], "# ");
    wprintz(w_info, dam_color[4], "# ");
    wprintz(w_info, dam_color[5], "# ");
    wprintz(w_info, c_ltgray, _("(Thoroughly Damaged)"));

    wrefresh(w_info);

    bool done = false;
    bool redraw = true;
    int cursor_y = 0;
    int selected = -1;

    int torso_item_count;
    int eyes_item_count;
    int mouth_item_count;
    int arms_item_count;
    int hands_item_count;
    int legs_item_count;

    item tmp_item;
    std::stringstream temp1;

    do
    {
        if (redraw)
        {
            // detailed armor info window
            it_armor* cur_armor = dynamic_cast<it_armor*>(worn[cursor_y].type);

            temp1.str("");
            temp1 << _("Covers: ");
            if (cur_armor->covers & mfb(bp_head))
                temp1 << _("Head. ");
            if (cur_armor->covers & mfb(bp_eyes))
                temp1 << _("Eyes. ");
            if (cur_armor->covers & mfb(bp_mouth))
                temp1 << _("Mouth. ");
            if (cur_armor->covers & mfb(bp_torso))
               temp1 << _("Torso. ");
            if (cur_armor->covers & mfb(bp_arms))
               temp1 << _("Arms. ");
            if (cur_armor->covers & mfb(bp_hands))
               temp1 << _("Hands. ");
            if (cur_armor->covers & mfb(bp_legs))
               temp1 << _("Legs. ");
            if (cur_armor->covers & mfb(bp_feet))
               temp1 << _("Feet. ");
            temp1 << _(" Coverage: ");
            temp1 << int(cur_armor->coverage);

            werase(w_arm_info);
            wborder(w_arm_info, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_XXXO, LINE_XOXX, LINE_XXXO, LINE_XOXX );
            mvwprintz(w_arm_info, 1, 1, dam_color[int(worn[cursor_y].damage + 1)], worn[cursor_y].tname(g).c_str());
            wprintz(w_arm_info, c_ltgray, " - ");
            wprintz(w_arm_info, c_ltgray, temp1.str().c_str());

            temp1.str("");
            temp1 << _("Encumbrance: ");
            if (worn[cursor_y].has_flag("FIT"))
            {
                temp1 << (int(cur_armor->encumber) - 1);
                temp1 << _(" (fits)");
            }
            else
            {
                temp1 << int(cur_armor->encumber);
            }
            temp1 << _(" Bash prot: ");
            temp1 << int(worn[cursor_y].bash_resist());
            temp1 << _(" Cut prot: ");
            temp1 << int(worn[cursor_y].cut_resist());
            temp1 << _(" Warmth: ");
            temp1 << int(cur_armor->warmth);
            temp1 << _(" Storage: ");
            temp1 << int(cur_armor->storage);

            mvwprintz(w_arm_info, 2, 1, c_ltgray, temp1.str().c_str());

            werase(w_all_worn);
            wborder(w_all_worn, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_XXXO, LINE_OXXX, LINE_XXOO, LINE_XOOX );
            mvwprintz(w_all_worn, 1, 1, c_white, _("WORN CLOTHING"));
            mvwprintz(w_all_worn, 1, iCol1WinX-9, c_ltgray ,_("Storage"));

            mvwprintz(w_all_worn, 2, 1, c_ltgray, _("(Innermost)"));
            for (int i = 0; i < worn.size(); i++)
            {
                it_armor* each_armor = dynamic_cast<it_armor*>(worn[i].type);
                if (i == cursor_y)
                    mvwprintz(w_all_worn, 3+cursor_y, 2, c_yellow, ">>");
                if (selected >= 0 && i == selected)
                    mvwprintz(w_all_worn, i+3, 5, dam_color[int(worn[i].damage + 1)], each_armor->name.c_str());
                else
                    mvwprintz(w_all_worn, i+3, 4, dam_color[int(worn[i].damage + 1)], each_armor->name.c_str());
                mvwprintz(w_all_worn, i+3, iCol1WinX-4, dam_color[int(worn[i].damage + 1)], "%2d", int(each_armor->storage));
            }
            mvwprintz(w_all_worn, 3 + worn.size(), 1, c_ltgray, _("(Outermost)"));

            werase(w_torso_worn);
            werase(w_eyes_worn);
            werase(w_mouth_worn);
            werase(w_hands_worn);
            werase(w_arms_worn);
            werase(w_legs_worn);

            wborder(w_torso_worn, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXX, LINE_OXXX, LINE_XXXO, LINE_XOXX);
            wborder(w_eyes_worn, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_XXXO, LINE_XOXX, LINE_XXXO, LINE_XOXX);
            wborder(w_mouth_worn, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_XXXO, LINE_XOXX, LINE_XXOO, LINE_XOOX);
            wborder(w_arms_worn, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_XXXO, LINE_XOXX, LINE_XXXO, LINE_XOXX);
            wborder(w_hands_worn, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_XXXO, LINE_XOXX, LINE_XXOO, LINE_XOOX);
            wborder(w_legs_worn, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXX, LINE_XOXX, LINE_XXXO, LINE_XOXX);

            mvwprintz(w_torso_worn, 1, 1, c_white, _("TORSO CLOTHING"));
            mvwprintz(w_eyes_worn, 1, 1, c_white, _("EYES CLOTHING"));
            mvwprintz(w_mouth_worn, 1, 1, c_white, _("MOUTH CLOTHING"));
            mvwprintz(w_arms_worn, 1, 1, c_white, _("ARMS CLOTHING"));
            mvwprintz(w_hands_worn, 1, 1, c_white, _("HANDS CLOTHING"));
            mvwprintz(w_legs_worn, 1, 1, c_white, _("LEGS CLOTHING"));

            mvwprintz(w_torso_worn, 1, iCol2WinX-8, c_ltgray ,_("Encumb"));
            mvwprintz(w_eyes_worn, 1, iCol2WinX-8, c_ltgray ,_("Encumb"));
            mvwprintz(w_mouth_worn, 1, iCol2WinX-8, c_ltgray ,_("Encumb"));
            mvwprintz(w_arms_worn, 1, iCol3WinX-8, c_ltgray ,_("Encumb"));
            mvwprintz(w_hands_worn, 1, iCol3WinX-8, c_ltgray ,_("Encumb"));
            mvwprintz(w_legs_worn, 1, iCol3WinX-8, c_ltgray ,_("Encumb"));

            torso_item_count = 0;
            eyes_item_count = 0;
            mouth_item_count = 0;
            arms_item_count = 0;
            hands_item_count = 0;
            legs_item_count = 0;

            for (int i = 0; i < worn.size(); i++)
            {
                it_armor* each_armor = dynamic_cast<it_armor*>(worn[i].type);

                nc_color fitc=(worn[i].has_flag("FIT") ? c_green : c_ltgray );

                if (each_armor->covers & mfb(bp_torso))
                {
                    mvwprintz(w_torso_worn, torso_item_count + 2, 3, dam_color[int(worn[i].damage + 1)], each_armor->name.c_str());
                    mvwprintz(w_torso_worn, torso_item_count + 2, iCol2WinX-4, fitc, "%2d", int(each_armor->encumber) - (worn[i].has_flag("FIT") ? 1 : 0 ) );
                    torso_item_count++;
                }
                if (each_armor->covers & mfb(bp_eyes))
                {
                    mvwprintz(w_eyes_worn, eyes_item_count + 2, 3, dam_color[int(worn[i].damage + 1)], each_armor->name.c_str());
                    mvwprintz(w_eyes_worn, eyes_item_count + 2, iCol3WinX-4, fitc, "%2d", int(each_armor->encumber) - (worn[i].has_flag("FIT") ? 1 : 0 ) );
                    eyes_item_count++;
                }
                if (each_armor->covers & mfb(bp_mouth))
                {
                    mvwprintz(w_mouth_worn, mouth_item_count + 2, 3, dam_color[int(worn[i].damage + 1)], each_armor->name.c_str());
                    mvwprintz(w_mouth_worn, mouth_item_count + 2, iCol3WinX-4, fitc, "%2d", int(each_armor->encumber) - (worn[i].has_flag("FIT") ? 1 : 0 ) );
                    mouth_item_count++;
                }
                if (each_armor->covers & mfb(bp_arms))
                {
                    mvwprintz(w_arms_worn, arms_item_count + 2, 3, dam_color[int(worn[i].damage + 1)], each_armor->name.c_str());
                    mvwprintz(w_arms_worn, arms_item_count + 2, iCol2WinX-4, fitc, "%2d", int(each_armor->encumber) - (worn[i].has_flag("FIT") ? 1 : 0 ) );
                    arms_item_count++;
                }
                if (each_armor->covers & mfb(bp_hands))
                {
                    mvwprintz(w_hands_worn, hands_item_count + 2, 3, dam_color[int(worn[i].damage + 1)], each_armor->name.c_str());
                    mvwprintz(w_hands_worn, hands_item_count + 2, iCol2WinX-4, fitc, "%2d", int(each_armor->encumber) - (worn[i].has_flag("FIT") ? 1 : 0 ) );
                    hands_item_count++;
                }
                if (each_armor->covers & mfb(bp_legs))
                {
                    mvwprintz(w_legs_worn, legs_item_count + 2, 3, dam_color[int(worn[i].damage + 1)], each_armor->name.c_str());
                    mvwprintz(w_legs_worn, legs_item_count + 2, iCol3WinX-4, fitc, "%2d", int(each_armor->encumber) - (worn[i].has_flag("FIT") ? 1 : 0 ) );
                    legs_item_count++;
                }
            }
        }

        wrefresh(w_info);
        wrefresh(w_arm_info);
        wrefresh(w_all_worn);
        wrefresh(w_torso_worn);
        wrefresh(w_eyes_worn);
        wrefresh(w_mouth_worn);
        wrefresh(w_hands_worn);
        wrefresh(w_arms_worn);
        wrefresh(w_legs_worn);

        redraw = false;

        switch (input())
        {
            case 'j':
            case KEY_DOWN:
                if (selected >= 0)
                {
                    if (selected < (worn.size() - 1))
                    {
                        tmp_item = worn[cursor_y + 1];
                        worn[cursor_y + 1] = worn[cursor_y];
                        worn[cursor_y] = tmp_item;
                        selected++;
                        cursor_y++;
                    }
                }
                else
                {
                    cursor_y++;
                    cursor_y = (cursor_y >= worn.size() ? 0 : cursor_y);
                }
                redraw = true;
                break;
            case 'k':
            case KEY_UP:
                if (selected >= 0)
                {
                    if (selected > 0)
                    {
                        tmp_item = worn[cursor_y - 1];
                        worn[cursor_y - 1] = worn[cursor_y];
                        worn[cursor_y] = tmp_item;
                        selected--;
                        cursor_y--;
                    }
                }
                else
                {
                    cursor_y--;
                    cursor_y = (cursor_y < 0 ? worn.size() - 1 : cursor_y);
                }
                redraw = true;
                break;
            case 's':
                if (((dynamic_cast<it_armor*>(worn[cursor_y].type))->covers & mfb(bp_head)) ||
                    ((dynamic_cast<it_armor*>(worn[cursor_y].type))->covers & mfb(bp_feet)))
                {
                    popup(_("This piece of clothing cannot be layered."));
                }
                else
                {
                    if (selected >= 0)
                        selected = -1;
                    else
                        selected = cursor_y;
                }
                redraw = true;
                break;
            case 'r':   // uses special characters for worn inventory
                {
                    int invlet = 0;
                    for (int i = 0; i < worn.size(); i++)
                    {
                        if (invlet < 76)
                        {
                            if (has_item(inv_chars[52 + invlet]))
                            {
                                item change_to = i_at(inv_chars[52 + invlet]);
                                change_to.invlet = worn[i].invlet;
                                worn[i].invlet = inv_chars[52 + invlet];
                            }
                            else
                            {
                                worn[i].invlet = inv_chars[52 + invlet];
                            }
                            invlet++;
                        };
                    };
                };
                break;
            case 'q':
            case KEY_ESCAPE:
                done = true;
                break;
        }
    } while (!done);

    werase(w_info);
    werase(w_all_worn);
    werase(w_arm_info);
    werase(w_torso_worn);
    werase(w_eyes_worn);
    werase(w_mouth_worn);
    werase(w_hands_worn);
    werase(w_arms_worn);
    werase(w_legs_worn);

    delwin(w_info);
    delwin(w_arm_info);
    delwin(w_all_worn);
    delwin(w_torso_worn);
    delwin(w_eyes_worn);
    delwin(w_mouth_worn);
    delwin(w_hands_worn);
    delwin(w_arms_worn);
    delwin(w_legs_worn);
}

void player::use_wielded(game *g) {
  use(g, weapon.invlet);
}

hint_rating player::rate_action_reload(item *it) {
 if (it->is_gun()) {
  if (it->has_flag("RELOAD_AND_SHOOT") || it->ammo_type() == "NULL") {
   return HINT_CANT;
  }
  if (it->charges == it->clip_size()) {
   int alternate_magazine = -1;
   for (int i = 0; i < it->contents.size(); i++)
   {
       if ((it->contents[i].is_gunmod() &&
            (it->contents[i].typeId() == "spare_mag" &&
             it->contents[i].charges < (dynamic_cast<it_gun*>(it->type))->clip)) ||
           (it->contents[i].has_flag("MODE_AUX") &&
            it->contents[i].charges < it->contents[i].clip_size()))
       {
           alternate_magazine = i;
       }
   }
   if(alternate_magazine == -1) {
    return HINT_IFFY;
   }
  }
  return HINT_GOOD;
 } else if (it->is_tool()) {
  it_tool* tool = dynamic_cast<it_tool*>(it->type);
  if (tool->ammo == "NULL") {
   return HINT_CANT;
  }
  return HINT_GOOD;
 }
 return HINT_CANT;
}

hint_rating player::rate_action_unload(item *it) {
 if (!it->is_gun() && !it->is_container() &&
     (!it->is_tool() || it->ammo_type() == "NULL")) {
  return HINT_CANT;
 }
 int spare_mag = -1;
 int has_m203 = -1;
 int has_shotgun = -1;
 if (it->is_gun()) {
  spare_mag = it->has_gunmod ("spare_mag");
  has_m203 = it->has_gunmod ("m203");
  has_shotgun = it->has_gunmod ("u_shotgun");
 }
 if (it->is_container() ||
     (it->charges == 0 &&
      (spare_mag == -1 || it->contents[spare_mag].charges <= 0) &&
      (has_m203 == -1 || it->contents[has_m203].charges <= 0) &&
      (has_shotgun == -1 || it->contents[has_shotgun].charges <= 0))) {
  if (it->contents.size() == 0) {
   return HINT_IFFY;
  }
 }

 return HINT_GOOD;
}

//TODO refactor stuff so we don't need to have this code mirroring game::disassemble
hint_rating player::rate_action_disassemble(item *it, game *g) {
 for (recipe_map::iterator cat_iter = g->recipes.begin(); cat_iter != g->recipes.end(); ++cat_iter)
    {
        for (recipe_list::iterator list_iter = cat_iter->second.begin();
             list_iter != cat_iter->second.end();
             ++list_iter)
        {
            recipe* cur_recipe = *list_iter;
            if (it->type == g->itypes[cur_recipe->result] && cur_recipe->reversible)
            // ok, a valid recipe exists for the item, and it is reversible
            // assign the activity
            {
                // check tools are available
                // loop over the tools and see what's required...again
                inventory crafting_inv = g->crafting_inventory(this);
                for (int j = 0; j < cur_recipe->tools.size(); j++)
                {
                    bool have_tool = false;
                    if (cur_recipe->tools[j].size() == 0) // no tools required, may change this
                    {
                        have_tool = true;
                    }
                    else
                    {
                        for (int k = 0; k < cur_recipe->tools[j].size(); k++)
                        {
                            itype_id type = cur_recipe->tools[j][k].type;
                            int req = cur_recipe->tools[j][k].count;	// -1 => 1

                            if ((req <= 0 && crafting_inv.has_amount (type, 1)) ||
                                (req >  0 && crafting_inv.has_charges(type, req)))
                            {
                                have_tool = true;
                                k = cur_recipe->tools[j].size();
                            }
                            // if crafting recipe required a welder, disassembly requires a hacksaw or super toolkit
                            if (type == "welder")
                            {
                                if (crafting_inv.has_amount("hacksaw", 1) ||
                                    crafting_inv.has_amount("toolset", 1))
                                {
                                    have_tool = true;
                                }
                                else
                                {
                                    have_tool = false;
                                }
                            }
                        }

                        if (!have_tool)
                        {
                           return HINT_IFFY;
                        }
                    }
                }
                // all tools present
                return HINT_GOOD;
            }
        }
    }
    if(it->is_book())
        return HINT_GOOD;
    // no recipe exists, or the item cannot be disassembled
    return HINT_CANT;
}

hint_rating player::rate_action_use(item *it)
{
 if (it->is_tool()) {
  it_tool *tool = dynamic_cast<it_tool*>(it->type);
  if (tool->charges_per_use != 0 && it->charges < tool->charges_per_use) {
   return HINT_IFFY;
  } else {
   return HINT_GOOD;
  }
 } else if (it->is_gunmod()) {
  if (skillLevel("gun") == 0) {
   return HINT_IFFY;
  } else {
   return HINT_GOOD;
  }
 } else if (it->is_bionic()) {
  return HINT_GOOD;
 } else if (it->is_food() || it->is_food_container() || it->is_book() || it->is_armor()) {
  return HINT_IFFY; //the rating is subjective, could be argued as HINT_CANT or HINT_GOOD as well
 }

 return HINT_CANT;
}

void player::use(game *g, char let)
{
 item* used = &i_at(let);
 item copy;
 bool replace_item = false;
 if (!inv.item_by_letter(let).is_null()) {
  copy = inv.remove_item_by_letter(let);
  copy.invlet = let;
  used = &copy;
  replace_item = true;
 }

 if (used->is_null()) {
  g->add_msg(_("You do not have that item."));
  return;
 }

 last_item = itype_id(used->type->id);

 if (used->is_tool()) {

  it_tool *tool = dynamic_cast<it_tool*>(used->type);
  if (tool->charges_per_use == 0 || used->charges >= tool->charges_per_use) {
   iuse use;
   (use.*tool->use)(g, this, used, false);
   used->charges -= tool->charges_per_use;
  } else
   g->add_msg(_("Your %s has %d charges but needs %d."), used->tname(g).c_str(),
              used->charges, tool->charges_per_use);

  if (tool->use == &iuse::dogfood) replace_item = false;

  if (replace_item && used->invlet != 0)
   inv.add_item_keep_invlet(copy);
  else if (used->invlet == 0 && used == &weapon)
   remove_weapon();
  return;

 } else if (used->type->use == &iuse::boots) {

   iuse use;
   (use.*used->type->use)(g, this, used, false);
   if (replace_item)
    inv.add_item_keep_invlet(copy);
   return;
 } else if (used->is_gunmod()) {

   if (skillLevel("gun") == 0) {
   g->add_msg(_("You need to be at least level 1 in the firearms skill before you\
 can modify guns."));
   if (replace_item)
    inv.add_item(copy);
   return;
  }
  char gunlet = g->inv(_("Select gun to modify:"));
  it_gunmod *mod = static_cast<it_gunmod*>(used->type);
  item* gun = &(i_at(gunlet));
  if (gun->is_null()) {
   g->add_msg(_("You do not have that item."));
   if (replace_item)
    inv.add_item(copy);
   return;
  } else if (!gun->is_gun()) {
   g->add_msg(_("That %s is not a gun."), gun->tname(g).c_str());
   if (replace_item)
    inv.add_item(copy);
   return;
  }
  it_gun* guntype = dynamic_cast<it_gun*>(gun->type);
  if (guntype->skill_used == Skill::skill("archery") || guntype->skill_used == Skill::skill("launcher")) {
   g->add_msg(_("You cannot mod your %s."), gun->tname(g).c_str());
   if (replace_item)
    inv.add_item(copy);
   return;
  }
  if (guntype->skill_used == Skill::skill("pistol") && !mod->used_on_pistol) {
   g->add_msg(_("That %s cannot be attached to a handgun."),
              used->tname(g).c_str());
   if (replace_item)
    inv.add_item(copy);
   return;
  } else if (guntype->skill_used == Skill::skill("shotgun") && !mod->used_on_shotgun) {
   g->add_msg(_("That %s cannot be attached to a shotgun."),
              used->tname(g).c_str());
   if (replace_item)
    inv.add_item(copy);
   return;
  } else if (guntype->skill_used == Skill::skill("smg") && !mod->used_on_smg) {
   g->add_msg(_("That %s cannot be attached to a submachine gun."),
              used->tname(g).c_str());
   if (replace_item)
    inv.add_item(copy);
   return;
  } else if (guntype->skill_used == Skill::skill("rifle") && !mod->used_on_rifle) {
   g->add_msg(_("That %s cannot be attached to a rifle."),
              used->tname(g).c_str());
   if (replace_item)
    inv.add_item(copy);
   return;
  } else if ( mod->acceptible_ammo_types.size() && mod->acceptible_ammo_types.count(guntype->ammo) == 0 ) {
   g->add_msg(_("That %s cannot be used on a %s gun."), used->tname(g).c_str(),
              ammo_name(guntype->ammo).c_str());
   if (replace_item)
    inv.add_item(copy);
   return;
  } else if (gun->contents.size() >= 4) {
   g->add_msg(_("Your %s already has 4 mods installed!  To remove the mods,\
press 'U' while wielding the unloaded gun."), gun->tname(g).c_str());
   if (replace_item)
    inv.add_item(copy);
   return;
  }
  if ((mod->id == "clip" || mod->id == "clip2" || mod->id == "spare_mag") &&
      gun->clip_size() <= 2) {
   g->add_msg(_("You can not extend the ammo capacity of your %s."),
              gun->tname(g).c_str());
   if (replace_item)
    inv.add_item(copy);
   return;
  }
  if (mod->id == "spare_mag" && gun->has_flag("RELOAD_ONE")) {
   g->add_msg(_("You can not use a spare magazine with your %s."),
              gun->tname(g).c_str());
   if (replace_item)
    inv.add_item(copy);
   return;
  }
  for (int i = 0; i < gun->contents.size(); i++) {
   if (gun->contents[i].type->id == used->type->id) {
    g->add_msg(_("Your %s already has a %s."), gun->tname(g).c_str(),
               used->tname(g).c_str());
    if (replace_item)
     inv.add_item(copy);
    return;
   } else if (!(mod->item_tags.count("MODE_AUX")) && mod->newtype != "NULL" &&
	      !gun->contents[i].has_flag("MODE_AUX") &&
	      (dynamic_cast<it_gunmod*>(gun->contents[i].type))->newtype != "NULL") {
    g->add_msg(_("Your %s's caliber has already been modified."),
               gun->tname(g).c_str());
    if (replace_item)
     inv.add_item(copy);
    return;
   } else if ((mod->id == "barrel_big" || mod->id == "barrel_small") &&
              (gun->contents[i].type->id == "barrel_big" ||
               gun->contents[i].type->id == "barrel_small")) {
    g->add_msg(_("Your %s already has a barrel replacement."),
               gun->tname(g).c_str());
    if (replace_item)
     inv.add_item(copy);
    return;
   } else if ((mod->id == "clip" || mod->id == "clip2") &&
              (gun->contents[i].type->id == "clip" ||
               gun->contents[i].type->id == "clip2")) {
    g->add_msg(_("Your %s already has its magazine size extended."),
               gun->tname(g).c_str());
    if (replace_item)
     inv.add_item(copy);
    return;
   }
  }
  g->add_msg(_("You attach the %s to your %s."), used->tname(g).c_str(),
             gun->tname(g).c_str());
  if (replace_item)
   gun->contents.push_back(copy);
  else
   gun->contents.push_back(i_rem(let));
  return;

 } else if (used->is_bionic()) {

  it_bionic* tmp = dynamic_cast<it_bionic*>(used->type);
  if (install_bionics(g, tmp)) {
   if (!replace_item)
    i_rem(let);
  } else if (replace_item)
   inv.add_item(copy);
  return;

 } else if (used->is_food() || used->is_food_container()) {
  if (replace_item)
   inv.add_item(copy);
  eat(g, lookup_item(let));
  return;
 } else if (used->is_book()) {
  if (replace_item)
   inv.add_item(copy);
  read(g, let);
  return;
 } else if (used->is_armor()) {
  if (replace_item)
   inv.add_item(copy);
  wear(g, let);
  return;
 } else
  g->add_msg(_("You can't do anything interesting with your %s."),
             used->tname(g).c_str());

 if (replace_item)
  inv.add_item(copy);
}

hint_rating player::rate_action_read(item *it, game *g)
{
 //note: there's a cryptic note about macguffins in player::read(). Do we have to account for those?
 if (!it->is_book()) {
  return HINT_CANT;
 }

 it_book *book = dynamic_cast<it_book*>(it->type);

 if (g && g->light_level() < 8 && LL_LIT > g->m.light_at(posx, posy)) {
  return HINT_IFFY;
 } else if (morale_level() < MIN_MORALE_READ &&  book->fun <= 0) {
  return HINT_IFFY; //won't read non-fun books when sad
 } else if (book->intel > 0 && has_trait(PF_ILLITERATE)) {
  return HINT_IFFY;
 } else if (has_trait(PF_HYPEROPIC) && !is_wearing("glasses_reading")
            && !is_wearing("glasses_bifocal")) {
  return HINT_IFFY;
 }

 return HINT_GOOD;
}

void player::read(game *g, char ch)
{
    vehicle *veh = g->m.veh_at (posx, posy);
    if (veh && veh->player_in_control (this))
    {
        g->add_msg(_("It's bad idea to read while driving."));
        return;
    }

    // Check if reading is okay
    // check for light level
    if (fine_detail_vision_mod(g) > 2.5)
    {
        g->add_msg(_("You can't see to read!"));
        return;
    }

    // check for traits
    if (has_trait(PF_HYPEROPIC) && !is_wearing("glasses_reading")
        && !is_wearing("glasses_bifocal"))
    {
        g->add_msg(_("Your eyes won't focus without reading glasses."));
        return;
    }

    // Find the object
    int index = -1;
    item* it = NULL;
    if (weapon.invlet == ch)
    {
        index = -2;
        it = &weapon;
    }
    else
    {
        it = &inv.item_by_letter(ch);
    }

    if (it == NULL || it->is_null())
    {
        g->add_msg(_("You do not have that item."));
        return;
    }

// Some macguffins can be read, but they aren't treated like books.
    it_macguffin* mac = NULL;
    if (it->is_macguffin())
    {
        mac = dynamic_cast<it_macguffin*>(it->type);
    }
    if (mac != NULL)
    {
        iuse use;
        (use.*mac->use)(g, this, it, false);
        return;
    }

    if (!it->is_book())
    {
        g->add_msg(_("Your %s is not good reading material."),
        it->tname(g).c_str());
    return;
    }

    it_book* tmp = dynamic_cast<it_book*>(it->type);
    int time; //Declare this here so that we can change the time depending on whats needed
    if (tmp->intel > 0 && has_trait(PF_ILLITERATE))
    {
        g->add_msg(_("You're illiterate!"));
        return;
    }
    else if (tmp->type == NULL)
    {
        /* No-op, there's no associated skill. */
    }
    else if (skillLevel(tmp->type) < (int)tmp->req)
    {
        g->add_msg(_("The %s-related jargon flies over your head!"),
         tmp->type->name().c_str());
        if (tmp->recipes.size() == 0)
        {
            return;
        }
        else
        {
            g->add_msg(_("But you might be able to learn a recipe or two."));
        }
    }
    else if (morale_level() < MIN_MORALE_READ &&  tmp->fun <= 0) // See morale.h
    {
        g->add_msg(_("What's the point of reading?  (Your morale is too low!)"));
        return;
    }
    else if (skillLevel(tmp->type) >= (int)tmp->level && tmp->fun <= 0 && !can_study_recipe(tmp) &&
            !query_yn(_("Your %s skill won't be improved.  Read anyway?"),
                      tmp->type->name().c_str()))
    {
        return;
    }

    if (tmp->recipes.size() > 0 && !(activity.continuous))
    {
        if (can_study_recipe(tmp))
        {
            g->add_msg(_("This book has more recipes for you to learn."));
        }
        else if (studied_all_recipes(tmp))
        {
            g->add_msg(_("You know all the recipes this book has to offer."));
        }
        else
        {
            g->add_msg(_("This book has more recipes, but you don't have the skill to learn them yet."));
        }
    }

	// Base read_speed() is 1000 move points (1 minute per tmp->time)
    time = tmp->time * read_speed() * fine_detail_vision_mod(g);
    if (tmp->intel > int_cur)
    {
        g->add_msg(_("This book is too complex for you to easily understand. It will take longer to read."));
        time += (tmp->time * (tmp->intel - int_cur) * 100); // Lower int characters can read, at a speed penalty
        activity = player_activity(ACT_READ, time, index, ch, "");
        moves = 0;
        return;
    }

    activity = player_activity(ACT_READ, time, index, ch, "");
    moves = 0;

    // Reinforce any existing morale bonus/penalty, so it doesn't decay
    // away while you read more.
    int minutes = time / 1000;
    add_morale(MORALE_BOOK, 0, tmp->fun * 15, minutes + 30, minutes, false,
               tmp);
}

bool player::can_study_recipe(it_book* book)
{
    for (std::map<recipe*, int>::iterator iter = book->recipes.begin(); iter != book->recipes.end(); ++iter)
    {
        if (!knows_recipe(iter->first) &&
            (iter->first->sk_primary == NULL || skillLevel(iter->first->sk_primary) >= iter->second))
        {
            return true;
        }
    }
    return false;
}

bool player::studied_all_recipes(it_book* book)
{
    for (std::map<recipe*, int>::iterator iter = book->recipes.begin(); iter != book->recipes.end(); ++iter)
    {
        if (!knows_recipe(iter->first))
        {
            return false;
        }
    }
    return true;
}

bool player::try_study_recipe(game *g, it_book *book)
{
    for (std::map<recipe*, int>::iterator iter = book->recipes.begin(); iter != book->recipes.end(); ++iter)
    {
        if (!knows_recipe(iter->first) &&
            (iter->first->sk_primary == NULL || skillLevel(iter->first->sk_primary) >= iter->second))
        {
            if (iter->first->sk_primary == NULL || rng(0, 4) <= skillLevel(iter->first->sk_primary) - iter->second)
            {
                learn_recipe(iter->first);
                g->add_msg(_("Learned a recipe for %s from the %s."),
                           g->itypes[iter->first->result]->name.c_str(), book->name.c_str());
                return true;
            }
            else
            {
                g->add_msg(_("Failed to learn a recipe from the %s."), book->name.c_str());
                return false;
            }
        }
    }
    return true; // _("false") seems to mean _("attempted and failed")
}

void player::try_to_sleep(game *g)
{
 int vpart = -1;
 vehicle *veh = g->m.veh_at (posx, posy, vpart);
 const trap_id trap_at_pos = g->m.tr_at(posx, posy);
 const ter_id ter_at_pos = g->m.ter(posx, posy);
 const furn_id furn_at_pos = g->m.furn(posx, posy);
 if (furn_at_pos == f_bed || furn_at_pos == f_makeshift_bed ||
     trap_at_pos == tr_cot || trap_at_pos == tr_rollmat ||
     furn_at_pos == f_armchair || furn_at_pos == f_sofa ||
     (veh && veh->part_with_feature (vpart, vpf_seat) >= 0) ||
      (veh && veh->part_with_feature (vpart, vpf_bed) >= 0))
  g->add_msg(_("This is a comfortable place to sleep."));
 else if (ter_at_pos != t_floor)
  g->add_msg(
             terlist[ter_at_pos].movecost <= 2 ? 
             _("It's a little hard to get to sleep on this %s.") : 
             _("It's hard to get to sleep on this %s."),
             terlist[ter_at_pos].name.c_str());
 add_disease("lying_down", 300);
}

bool player::can_sleep(game *g)
{
 int sleepy = 0;
 if (has_addiction(ADD_SLEEP))
  sleepy -= 3;
 if (has_trait(PF_INSOMNIA))
  sleepy -= 8;

 int vpart = -1;
 vehicle *veh = g->m.veh_at (posx, posy, vpart);
 const trap_id trap_at_pos = g->m.tr_at(posx, posy);
 const ter_id ter_at_pos = g->m.ter(posx, posy);
 const furn_id furn_at_pos = g->m.furn(posx, posy);
 if ((veh && veh->part_with_feature (vpart, vpf_bed) >= 0) ||
     furn_at_pos == f_makeshift_bed || trap_at_pos == tr_cot ||
     furn_at_pos == f_sofa)
  sleepy += 4;
 else if ((veh && veh->part_with_feature (vpart, vpf_seat) >= 0) ||
      trap_at_pos == tr_rollmat || furn_at_pos == f_armchair)
  sleepy += 3;
 else if (furn_at_pos == f_bed)
  sleepy += 5;
 else if (ter_at_pos == t_floor)
  sleepy += 1;
 else
  sleepy -= g->m.move_cost(posx, posy);
 if (fatigue < 192)
  sleepy -= int( (192 - fatigue) / 4);
 else
  sleepy += int((fatigue - 192) / 16);
 sleepy += rng(-8, 8);
 sleepy -= 2 * stim;
 if (sleepy > 0)
  return true;
 return false;
}

// Returned values range from 1.0 (unimpeded vision) to 5.0 (totally blind).
// 2.5 is enough light for detail work.
float player::fine_detail_vision_mod(game *g)
{
    if (has_disease("blind") || has_disease("boomered"))
    {
        return 5;
    }
    if ( has_nv() )
    {
        return 1.5;
    }
    // flashlight is handled by the light level check below
    if (g->u.has_active_item("lightstrip"))
    {
        return 1;
    }
    if (LL_LIT <= g->m.light_at(posx, posy))
    {
        return 1;
    }

    float vision_ii = 0;
    if (g->m.light_at(posx, posy) == LL_LOW) { vision_ii = 4; }
    else if (g->m.light_at(posx, posy) == LL_DARK) { vision_ii = 5; }

    if (g->u.has_active_item("glowstick_lit"))
    {
        vision_ii -= 3.5;
    }

    if (has_trait(PF_NIGHTVISION)) { vision_ii -= .5; }
    else if (has_trait(PF_NIGHTVISION2)) { vision_ii -= 1.5; }
    else if (has_trait(PF_NIGHTVISION3))	{ vision_ii -= 2.5; }

    if (vision_ii < 1)	{ vision_ii = 1; }
    return vision_ii;
}

int player::warmth(body_part bp)
{
    int bodywetness = 0;
    int ret = 0, warmth = 0;
    it_armor* armor = NULL;

    // Fetch the morale value of wetness for bodywetness
    for (int i = 0; bodywetness == 0 && i < morale.size(); i++)
    {
        if( morale[i].type == MORALE_WET )
        {
            bodywetness = abs(morale[i].bonus); // Make it positive, less confusing
            break;
        }
    }

    // If the player is not wielding anything, check if hands can be put in pockets
    if(bp == bp_hands && !is_armed() && worn_with_flag("POCKETS"))
    {
        ret += 10;
    }

    for (int i = 0; i < worn.size(); i++)
    {
        armor = dynamic_cast<it_armor*>(worn[i].type);

        if (armor->covers & mfb(bp))
        {
            warmth = armor->warmth;
            // Wool items do not lose their warmth in the rain
            if (!worn[i].made_of("wool"))
            {
                warmth *= 1.0 - (float)bodywetness / 100.0;
            }
            ret += warmth;
        }
    }
    return ret;
}

int player::encumb(body_part bp) {
 int iLayers = 0, iArmorEnc = 0;
 return encumb(bp, iLayers, iArmorEnc);
}

int player::encumb(body_part bp, int &layers, int &armorenc)
{
    int ret = 0;
    it_armor* armor;
    for (int i = 0; i < worn.size(); i++)
    {
        if (!worn[i].is_armor())
            debugmsg("%s::encumb hit a non-armor item at worn[%d] (%s)", name.c_str(),
            i, worn[i].tname().c_str());
        armor = dynamic_cast<it_armor*>(worn[i].type);

        if (armor->covers & mfb(bp))
        {
           if (armor->is_power_armor() && (has_active_item("UPS_on") || has_active_item("adv_UPS_on") || has_active_bionic("bio_power_armor_interface") || has_active_bionic("bio_power_armor_interface_mkII")))
            {
                armorenc += armor->encumber - 4;
            }
            else
            {
                armorenc += armor->encumber;
                if (worn[i].has_flag("FIT"))
                {
                    armorenc--;
                }
            }
            if (armor->encumber >= 0 || bp != bp_torso)
            {
                layers++;
            }
        }
    }

    ret += armorenc;

    // Following items undo their layering. Once. Bodypart has to be taken into account, hence the switch.
    switch (bp)
    {
        case bp_feet  : if (is_wearing("socks") || is_wearing("socks_wool")) layers--; break;
        case bp_legs  : if (is_wearing("long_underpants")) layers--; break;
        case bp_hands : if (is_wearing("gloves_liner")) layers--; break;
        case bp_torso : if (is_wearing("under_armor")) layers--; break;
    }
    if (layers > 1)
    {
        ret += (layers - 1) * (bp == bp_torso ? .5 : 2);// Easier to layer on torso
    }
    if (volume_carried() > volume_capacity() - 2 && bp != bp_head)
    {
        ret += 3;
    }

    // Fix for negative hand encumbrance
    if ((bp == bp_hands) && (ret < 0))
     ret =0;

    // Bionics and mutation
    if (has_bionic("bio_stiff") && bp != bp_head && bp != bp_mouth)
    {
        ret += 1;
    }
    if (has_trait(PF_CHITIN3) && bp != bp_eyes && bp != bp_mouth)
    {
        ret += 1;
    }
    if (has_trait(PF_SLIT_NOSTRILS) && bp == bp_mouth)
    {
        ret += 1;
    }
    if (bp == bp_hands &&
     (has_trait(PF_ARM_TENTACLES) || has_trait(PF_ARM_TENTACLES_4) ||
     has_trait(PF_ARM_TENTACLES_8)))
    {
        ret += 3;
    }
    return ret;
}

int player::armor_bash(body_part bp)
{
 int ret = 0;
 it_armor* armor;
 for (int i = 0; i < worn.size(); i++) {
  armor = dynamic_cast<it_armor*>(worn[i].type);
  if (armor->covers & mfb(bp))
   ret += worn[i].bash_resist();
 }
 if (has_bionic("bio_carbon"))
  ret += 2;
 if (bp == bp_head && has_bionic("bio_armor_head"))
  ret += 3;
 else if (bp == bp_arms && has_bionic("bio_armor_arms"))
  ret += 3;
 else if (bp == bp_torso && has_bionic("bio_armor_torso"))
  ret += 3;
 else if (bp == bp_legs && has_bionic("bio_armor_legs"))
  ret += 3;
 if (has_trait(PF_FUR))
  ret++;
 if (has_trait(PF_CHITIN))
  ret += 2;
 if (has_trait(PF_SHELL) && bp == bp_torso)
  ret += 6;
 ret += rng(0, disease_intensity("armor_boost"));
 return ret;
}

int player::armor_cut(body_part bp)
{
 int ret = 0;
 it_armor* armor;
 for (int i = 0; i < worn.size(); i++) {
  armor = dynamic_cast<it_armor*>(worn[i].type);
  if (armor->covers & mfb(bp))
   ret += worn[i].cut_resist();
 }
 if (has_bionic("bio_carbon"))
  ret += 4;
 if (bp == bp_head && has_bionic("bio_armor_head"))
  ret += 3;
 else if (bp == bp_arms && has_bionic("bio_armor_arms"))
  ret += 3;
 else if (bp == bp_torso && has_bionic("bio_armor_torso"))
  ret += 3;
 else if (bp == bp_legs && has_bionic("bio_armor_legs"))
  ret += 3;
 if (has_trait(PF_THICKSKIN))
  ret++;
 if (has_trait(PF_SCALES))
  ret += 2;
 if (has_trait(PF_THICK_SCALES))
  ret += 4;
 if (has_trait(PF_SLEEK_SCALES))
  ret += 1;
 if (has_trait(PF_CHITIN))
  ret += 2;
 if (has_trait(PF_CHITIN2))
  ret += 4;
 if (has_trait(PF_CHITIN3))
  ret += 8;
 if (has_trait(PF_SHELL) && bp == bp_torso)
  ret += 14;
 ret += rng(0, disease_intensity("armor_boost"));
 return ret;
}

void player::absorb(game *g, body_part bp, int &dam, int &cut)
{
    it_armor* tmp;
    int arm_bash = 0, arm_cut = 0;
    bool cut_through = true;      // to determine if cutting damage penetrates multiple layers of armour
    int bash_absorb = 0;      // to determine if lower layers of armour get damaged

    // CBMS absorb damage first before hitting armour
    if (has_active_bionic("bio_ads"))
    {
        if (dam > 0 && power_level > 1)
        {
            dam -= rng(1, 8);
            power_level--;
        }
        if (cut > 0 && power_level > 1)
        {
            cut -= rng(0, 4);
            power_level--;
        }
        if (dam < 0)
            dam = 0;
        if (cut < 0)
            cut = 0;
    }

    // determines how much damage is absorbed by armour
    // zero if damage misses a covered part
    int bash_reduction = 0;
    int cut_reduction = 0;

    // See, we do it backwards, which assumes the player put on their jacket after
    //  their T shirt, for example.  TODO: don't assume! ASS out of U & ME, etc.
    for (int i = worn.size() - 1; i >= 0; i--)
    {
        tmp = dynamic_cast<it_armor*>(worn[i].type);
        if ((tmp->covers & mfb(bp)) && tmp->storage <= 24)
        {
            // first determine if damage is at a covered part of the body
            // probability given by coverage
            if (rng(0, 100) < tmp->coverage)
            {
                // hit a covered part of the body, so now determine if armour is damaged
                arm_bash = worn[i].bash_resist();
                arm_cut  = worn[i].cut_resist();
                // also determine how much damage is absorbed by armour
                // factor of 6 to normalise for material hardness values
                bash_reduction = arm_bash / 6;
                cut_reduction = arm_cut / 6;

                // power armour first  - to depreciate eventually
                if (((it_armor *)worn[i].type)->is_power_armor())
                {
                    if (cut > arm_cut * 2 || dam > arm_bash * 2)
                    {
                        g->add_msg_if_player(this,_("Your %s is damaged!"), worn[i].tname(g).c_str());
                        worn[i].damage++;
                    }
                }
                else // normal armour
                {
                    // determine how much the damage exceeds the armour absorption
                    // bash damage takes into account preceding layers
                    int diff_bash = (dam - arm_bash - bash_absorb < 0) ? -1 : (dam - arm_bash);
                    int diff_cut  = (cut - arm_cut  < 0) ? -1 : (dam - arm_cut);
                    bool armor_damaged = false;
                    std::string pre_damage_name = worn[i].tname(g);

                    // armour damage occurs only if damage exceeds armour absorption
                    // plus a luck factor, even if damage is below armour absorption (2% chance)
                    if ((diff_bash > arm_bash && !one_in(diff_bash)) ||
                        (diff_bash == -1 && one_in(50)))
                    {
                        armor_damaged = true;
                        worn[i].damage++;
                    }
                    bash_absorb += arm_bash;

                    // cut damage falls through to inner layers only if preceding layer was damaged
                    if (cut_through)
                    {
                        if ((diff_cut > arm_cut && !one_in(diff_cut)) ||
                            (diff_cut == -1 && one_in(50)))
                        {
                            armor_damaged = true;
                            worn[i].damage++;
                        }
                        else // layer of clothing was not damaged, so stop cutting damage from penetrating
                        {
                            cut_through = false;
                        }
                    }

                    // now check if armour was completely destroyed and display relevant messages
                    if (worn[i].damage >= 5)
                    {
                        g->add_msg_player_or_npc( this, _("Your %s is completely destroyed!"),
                                                  _("<npcname>'s %s is completely destroyed!"),
                                                  worn[i].tname(g).c_str() );
                        worn.erase(worn.begin() + i);
                    } else if (armor_damaged) {
                        std::string damage_verb = diff_bash > diff_cut ? tmp->bash_dmg_verb() :
                                                                         tmp->cut_dmg_verb();
                        g->add_msg_if_player(this, _("Your %s is %s!"), pre_damage_name.c_str(),
                                             damage_verb.c_str());
                    }
                } // end of armour damage code
            }
        }
        // reduce damage accordingly
        dam -= bash_reduction;
        cut -= cut_reduction;
    }
    // now account for CBMs and mutations
    if (has_bionic("bio_carbon"))
    {
        dam -= 2;
        cut -= 4;
    }
    if (bp == bp_head && has_bionic("bio_armor_head"))
    {
        dam -= 3;
        cut -= 3;
    }
    else if (bp == bp_arms && has_bionic("bio_armor_arms"))
    {
        dam -= 3;
        cut -= 3;
    }
    else if (bp == bp_torso && has_bionic("bio_armor_torso"))
    {
        dam -= 3;
        cut -= 3;
    }
    else if (bp == bp_legs && has_bionic("bio_armor_legs"))
    {
        dam -= 3;
        cut -= 3;
    }
    if (has_trait(PF_THICKSKIN))
        cut--;
    if (has_trait(PF_SCALES))
        cut -= 2;
    if (has_trait(PF_THICK_SCALES))
        cut -= 4;
    if (has_trait(PF_SLEEK_SCALES))
        cut -= 1;
    if (has_trait(PF_FEATHERS))
        dam--;
    if (has_trait(PF_FUR))
        dam--;
    if (has_trait(PF_CHITIN))
        cut -= 2;
    if (has_trait(PF_CHITIN2))
    {
        dam--;
        cut -= 4;
    }
    if (has_trait(PF_CHITIN3))
    {
        dam -= 2;
        cut -= 8;
    }
    if (has_trait(PF_PLANTSKIN))
        dam--;
    if (has_trait(PF_BARK))
        dam -= 2;
    if (bp == bp_feet && has_trait(PF_HOOVES))
        cut--;
    if (has_trait(PF_LIGHT_BONES))
        dam *= 1.4;
    if (has_trait(PF_HOLLOW_BONES))
        dam *= 1.8;
    if (dam < 0)
        dam = 0;
    if (cut < 0)
        cut = 0;
}

int player::resist(body_part bp)
{
 int ret = 0;
 for (int i = 0; i < worn.size(); i++) {
  if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp) ||
      (bp == bp_eyes && // Head protection works on eyes too (e.g. baseball cap)
           (dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp_head)))
   ret += (dynamic_cast<it_armor*>(worn[i].type))->env_resist;
 }
 if (bp == bp_mouth && has_bionic("bio_purifier") && ret < 5) {
  ret += 2;
  if (ret == 6)
   ret = 5;
 }
 return ret;
}

bool player::wearing_something_on(body_part bp)
{
 for (int i = 0; i < worn.size(); i++) {
  if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp))
    return true;
 }
 return false;
}

bool player::is_wearing_power_armor(bool *hasHelmet) const {
  if (worn.size() && ((it_armor *)worn[0].type)->is_power_armor()) {
    if (hasHelmet) {
      *hasHelmet = false;

      if (worn.size() > 1) {
        for (size_t i = 1; i < worn.size(); i++) {
          it_armor *candidate = dynamic_cast<it_armor*>(worn[i].type);

          if (candidate->is_power_armor() && candidate->covers & mfb(bp_head)) {
            *hasHelmet = true;
            break;
          }
        }
      }
    }

    return true;
  } else {
    return false;
  }
}

int player::adjust_for_focus(int amount)
{
    int effective_focus = focus_pool;
    if (has_trait(PF_FASTLEARNER))
    {
        effective_focus += 15;
    }
    double tmp = amount * (effective_focus / 100.0);
    int ret = int(tmp);
    if (rng(0, 100) < 100 * (tmp - ret))
    {
        ret++;
    }
    return ret;
}

void player::practice (const calendar& turn, Skill *s, int amount)
{
    SkillLevel& level = skillLevel(s);
    // Double amount, but only if level.exercise isn't a amall negative number?
    if (level.exercise() < 0)
    {
        if (amount >= -level.exercise())
        {
            amount -= level.exercise();
        } else {
            amount += amount;
        }
    }

    bool isSavant = has_trait(PF_SAVANT);

    Skill *savantSkill = NULL;
    SkillLevel savantSkillLevel = SkillLevel();

    if (isSavant)
    {
        for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin();
             aSkill != Skill::skills.end(); ++aSkill)
        {
            if (skillLevel(*aSkill) > savantSkillLevel)
            {
                savantSkill = *aSkill;
                savantSkillLevel = skillLevel(*aSkill);
            }
        }
    }

    amount = adjust_for_focus(amount);
    if (isSavant && s != savantSkill)
    {
        amount /= 2;
    }

    if (amount > 0 && level.isTraining())
    {
        skillLevel(s).train(amount);

        int chance_to_drop = focus_pool;
        focus_pool -= chance_to_drop / 100;
        if (rng(1, 100) <= (chance_to_drop % 100))
        {
            focus_pool--;
        }
    }

    skillLevel(s).practice(turn);
}

void player::practice (const calendar& turn, std::string s, int amount)
{
    Skill *aSkill = Skill::skill(s);
    practice(turn, aSkill, amount);
}

bool player::knows_recipe(recipe *rec)
{
    // do we know the recipe by virtue of it being autolearned?
    if (rec->autolearn)
    {
        if( (rec->sk_primary == NULL ||
             skillLevel(rec->sk_primary) >= rec->difficulty) &&
            (rec->sk_secondary == NULL || skillLevel(rec->sk_secondary) > 0) )
        {
            return true;
        }
    }

    if (learned_recipes.find(rec->ident) != learned_recipes.end())
    {
        return true;
    }

    return false;
}

void player::learn_recipe(recipe *rec)
{
    learned_recipes[rec->ident] = rec;
}

void player::assign_activity(game* g, activity_type type, int moves, int index, char invlet, std::string name)
{
 if (backlog.type == type && backlog.index == index && backlog.invlet == invlet &&
     backlog.name == name && query_yn(_("Resume task?"))) {
  activity = backlog;
  backlog = player_activity();
 } else
  activity = player_activity(type, moves, index, invlet, name);
}

void player::cancel_activity()
{
 if (activity_is_suspendable(activity.type))
  backlog = activity;
 activity.type = ACT_NULL;
}

std::vector<item*> player::has_ammo(ammotype at)
{
    return inv.all_ammo(at);
}

std::string player::weapname(bool charges)
{
 if (!(weapon.is_tool() &&
       dynamic_cast<it_tool*>(weapon.type)->max_charges <= 0) &&
     weapon.charges >= 0 && charges) {
  std::stringstream dump;
  int spare_mag = weapon.has_gunmod("spare_mag");
  dump << weapon.tname().c_str() << " (" << weapon.charges;
  if( -1 != spare_mag )
   dump << "+" << weapon.contents[spare_mag].charges;
  for (int i = 0; i < weapon.contents.size(); i++)
   if (weapon.contents[i].is_gunmod() &&
       weapon.contents[i].has_flag("MODE_AUX"))
    dump << "+" << weapon.contents[i].charges;
  dump << ")";
  return dump.str();
 } else if (weapon.is_null())
  return _("fists");

 else if (weapon.is_style()) { // Styles get bonus-bars!
  std::stringstream dump;
  dump << weapon.tname();

  if(weapon.typeId() == "style_capoeira"){
   if (has_disease("dodge_boost"))
    dump << _(" +Dodge");
   if (has_disease("attack_boost"))
    dump << _(" +Attack");
  } else if(weapon.typeId() == "style_ninjutsu"){
  } else if(weapon.typeId() == "style_leopard"){
   if (has_disease("attack_boost"))
    dump << _(" +Attack");
  } else if(weapon.typeId() == "style_crane"){
   if (has_disease("dodge_boost"))
    dump << _(" +Dodge");
  } else if(weapon.typeId() == "style_dragon"){
   if (has_disease("damage_boost"))
    dump << _(" +Damage");
  } else if(weapon.typeId() == "style_tiger"){
   dump << " [";
   int intensity = disease_intensity("damage_boost");
   for (int i = 1; i <= 5; i++) {
    if (intensity >= i * 2)
     dump << "*";
    else
     dump << ".";
   }
   dump << "]";
  } else if(weapon.typeId() == "style_centipede"){
   dump << " [";
   int intensity = disease_intensity("speed_boost");
   for (int i = 1; i <= 8; i++) {
    if (intensity >= i * 4)
     dump << "*";
    else
     dump << ".";
   }
   dump << "]";
  } else if(weapon.typeId() == "style_venom_snake"){
   dump << " [";
   int intensity = disease_intensity("viper_combo");
   for (int i = 1; i <= 2; i++) {
    if (intensity >= i)
     dump << "C";
    else
     dump << ".";
   }
   dump << "]";
  } else if(weapon.typeId() == "style_lizard"){
   dump << " [";
   int intensity = disease_intensity("attack_boost");
   for (int i = 1; i <= 4; i++) {
    if (intensity >= i)
     dump << "*";
    else
     dump << ".";
   }
   dump << "]";
  } else if(weapon.typeId() == "style_toad"){
   dump << " [";
   int intensity = disease_intensity("armor_boost");
   for (int i = 1; i <= 5; i++) {
    if (intensity >= 5 + i)
     dump << "!";
    else if (intensity >= i)
     dump << "*";
    else
     dump << ".";
    }
    dump << "]";
  }

  return dump.str();
 } else
  return weapon.tname();
}

nc_color encumb_color(int level)
{
 if (level < 0)
  return c_green;
 if (level == 0)
  return c_ltgray;
 if (level < 4)
  return c_yellow;
 if (level < 7)
  return c_ltred;
 return c_red;
}

bool activity_is_suspendable(activity_type type)
{
 if (type == ACT_NULL || type == ACT_RELOAD || type == ACT_DISASSEMBLE)
  return false;
 return true;
}

SkillLevel& player::skillLevel(std::string ident) {
  return _skills[Skill::skill(ident)];
}

SkillLevel& player::skillLevel(Skill *_skill) {
  return _skills[_skill];
}

void player::copy_skill_levels(const player *rhs)
{
    _skills = rhs->_skills;
}

void player::set_skill_level(Skill* _skill, int level)
{
    skillLevel(_skill).level(level);
}
void player::set_skill_level(std::string ident, int level)
{
    skillLevel(ident).level(level);
}

void player::boost_skill_level(Skill* _skill, int level)
{
    skillLevel(_skill).level(level+skillLevel(_skill));
}
void player::boost_skill_level(std::string ident, int level)
{
    skillLevel(ident).level(level+skillLevel(ident));
}

void player::setID (int i)
{
    this->id = i;
}

int player::getID ()
{
    return this->id;
}

bool player::uncanny_dodge(bool is_u)
{
    if( this->power_level < 3 || !this->has_active_bionic("bio_uncanny_dodge") ) { return false; }
    point adjacent = adjacent_tile();
    power_level -= 3;
    if (adjacent.x != posx || adjacent.y != posy)
    {
        posx = adjacent.x;
        posy = adjacent.y;
        if (is_u)
            g->add_msg(_("Time seems to slow down and you instinctively dodge!"));
        else
            g->add_msg(_("Your target dodges... so fast!"));
        return true;
    }
    if (is_u)
        g->add_msg(_("You try to dodge but there's no room!"));
    return false;
}
// adjacent_tile() returns a safe, unoccupied adjacent tile. If there are no such tiles, returns player position instead.
point player::adjacent_tile()
{
    std::vector<point> ret;
    field_entry *cur = NULL;
    field tmpfld;
    trap_id curtrap;
    int dangerous_fields;
    for (int i=posx-1; i <= posx+1; i++)
    {
        for (int j=posy-1; j <= posy+1; j++)
        {
            if (i == posx && j == posy) continue;       // don't consider player position
            curtrap=g->m.tr_at(i, j);
            if (g->mon_at(i, j) == -1 && g->npc_at(i, j) == -1 && g->m.move_cost(i, j) > 0 && (curtrap == tr_null || g->traps[curtrap]->is_benign()))        // only consider tile if unoccupied, passable and has no traps
            {
                dangerous_fields = 0;
                tmpfld = g->m.field_at(i, j);
                for(std::map<field_id, field_entry*>::iterator field_list_it = tmpfld.getFieldStart(); field_list_it != tmpfld.getFieldEnd(); ++field_list_it)
                {
                    cur = field_list_it->second;
                    if (cur != NULL && cur->is_dangerous())
                        dangerous_fields++;
                }
                if (dangerous_fields == 0)
                {
                    ret.push_back(point(i, j));
                }
            }
        }
    }
    if (ret.size())
        return ret[rng(0, ret.size()-1)];   // return a random valid adjacent tile
    else
        return point(posx, posy);           // or return player position if no valid adjacent tiles
}

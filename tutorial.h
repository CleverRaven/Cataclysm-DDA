#include "translations.h"
#include <string>

const std::string tut_text[] = {_("\
Welcome to the Cataclysm tutorial!  As you play, pop-ups like this one will\n\
appear to guide you through the basic game actions.  Pressing spacebar will\n\
close the pop-up."),
_("\
The '@' character in the center of the screen represents you.  To move, you\n\
can use the numpad, the vikeys (hjklyubn), or the arrow keys.  To have your\n\
character wait in place, press '.'"),
_("\
To see what the symbols around you mean, press ';'.  You'll be able to scroll\n\
around and get information on the terrain, monsters, and items in the world\n\
around you."),
_("\
That brown '+' next to you is a closed door.  To open it, either simply walk\n\
into it, or press 'o' and then a movement key."),
_("\
You can close an opened door by pressing 'c' and then a movement key. Closing\n\
doors behind you can often slow down throngs of monsters greatly."),
_("\
Most monsters will have to smash their way through a closed door.  If a door\n\
is locked or stuck, you can smash it by pressing 's' and then a movement key."),
_("\
You can smash through windows by pressing 's' and then a movement key.  A\n\
smashed window takes a long time to climb through, and it's possible to hurt\n\
yourself on the broken glass, but it's a good last-ditch escape route.  You\n\
can smash through other things, too; use the ';' command and look for things\n\
that are \"Smashable.\""),
_("\
You just stepped on a space with one or more items.  If you wish to pick it\n\
up, press ',' or 'g'."),
_("\
The nearby { is a display rack, and the blue background indicates that there\n\
are items there.  If you examine the square by pressing 'e', you can pick up\n\
items without moving to the square."),
_("\
The Examine command is useful for grabbing items off window frames, around\n\
corners, etc.  It's also used for interacting with a few terrain types.\n\
Interactive terrain is usually designated with a '6' symbol."),
_("\
You don't have space in your inventory to store the item you just tried to\n\
pick up.  You need to wear something with storage space, like a backpace or\n\
cargo pants, to gain more storage space."),
_("\
The item you just picked up has been wielded as a weapon automatically.  This\n\
happened because you do not have space in your inventory to stash the item,\n\
and so you must carry it in your hand.  To expand your inventory space, try\n\
wearing clothing with a lot of storage space, like a backpack, or cargo pants."),
_("\
The item you just picked up has been wielded as a weapon automatically.  This\n\
happened because it is a good melee weapon, and you were empty-handed.  This\n\
usually is faster than than picking it up, then wielding it."),
_("\
The item you just picked up went into your inventory, the shared storage\n\
space of all the clothing you're wearing.  To view your inventory, press 'i'.\n\
You can then press the letter of any item to get more information about it."),
_("\
The item you just picked up is a type of clothing!  To wear clothing, press\n\
W and then select an item.  To take off clothing, press T, or simply take it\n\
off and drop it in one action by pressing d."),
_("\
The item you just picked up is a good weapon!  To wield a weapon, press w,\n\
then pick what to wield.  To wield nothing, either drop your weapon with d,\n\
or press 'w-' to put it away.  A zombie has spawned nearby.  To attack it,\n\
simply move into it."),
_("\
The item you just picked up is a comestible!  To eat a comestible, press 'E'.\n\
Comestibles are items you can use up, like food, drink, pills, etc.  Most\n\
food expires eventually, so be careful.  Some comestibles, especially drugs,\n\
can cause subtle, long-term effects."),
_("\
The item you just picked up is a tool!  To activate a tool, press 'a'.  Some\n\
tools have a set charge, which can be reloaded once depleted.  Other are\n\
single-use items, and still others are reusable."),
_("\
The item you just picked up is a firearm!  Guns are very powerful weapons but\n\
they require ammunition.  Firearms have many special attributes.  Most\n\
modify the damage done by their ammunition.  They also have dispersion, which\n\
affects their chance to hit.  Some guns are semi-automatic, while others can\n\
fire a burst. Some firearms (mainly bows and other muscle-powered weapons)\n\
have a range, which is added on top of the ammo's range when firing."),
_("\
The item you just picked up is ammunition, used with a gun.  It has many\n\
special attributes.  The damage value is the maximum done on a standard hit;\n\
a critical hit or headshot will do much more damage.  Some monsters or NPCs\n\
will wear armor which reduces the damage from gunfire; a high Armor-pierce\n\
value will reduce this effect.  The Range is the maximum range the ammo can\n\
achieve, and the dispersion affects its chance to hit."),
_("\
You just put on an article of clothing that provides physical protection.\n\
There are two types of damage that clothing defends against, bashing, and\n\
cutting.  Most monsters deal bashing damage, but cutting is often the more\n\
deadly of the two.  Bullets are considered cutting damage."),
_("\
You just put on an article of clothing that provides ample storage space.\n\
This will allow you to carry much more stuff, but be aware that there is also\n\
a limit on the weight you can carry which depends on strength.  The item you\n\
put on also encumbered your torso.  This will make combat a little more\n\
difficult.  To check encumbrance, press @."),
_("\
You just put on an article of clothing that protects against the environment.\n\
The most common and imporant are respiratory devices, which will protect you\n\
against smoke, air-born toxins or organisms, and other common hazards.\n\
However, they also make it a little harder to breath when running, so you'll\n\
move more slowly.  To check encumbrance, press @."),
_("\
If you press 'i' and then the letter of your weapon, you'll see its combat\n\
stats.  Bashing damage ignores most armor, but varies greatly and requires\n\
strength.  Cutting damage is a fixed amount, but is blocked by armor.  The\n\
To-hit bonus affects your chances of hitting. The amount of time it takes to\n\
swing your melee weapon depends on both its size and weight; small, compact\n\
weapons are the fastest."),
_("\
Hitting a monster will stun it briefly, allowing you to escape or get in\n\
another attack.  Your dexterity and melee skills are used to determine\n\
whether an attack hits, while strength affects damage."),
_("\
Taking damage often causes pain.  Small amounts of pain are tolerable, but as\n\
it gets worse your stats will drop and you will move much slower.  To reduce\n\
pain, take painkillers like codeine, or simply wait it out."),
_("\
When you kill a monster it will often leave behind a corpse.  Corpses can be\n\
important sources of food, but you must Butcher them by standing on the\n\
corpse and pressing 'B'.  You'll need a bladed weapon in your inventory,\n\
preferably a small, very sharp one.  An unskilled butcher may only get a few\n\
pieces of meat, or none at all.  Note that many monsters, such as zombies,\n\
leave tainted meat, unsuitable for consumption."),
_("\
That drug you just took is a painkiller.  Painkillers are very important to\n\
keep on hand, as pain both penalizes your stats and makes you slower.  Be\n\
careful, as the stronger painkillers can make you woozy, and some are even\n\
addictive."),
_("\
That drug you just took placed an effect on you.  To check your effects,\n\
press '@'.  Most only last a short while, while others, like chronic disease,\n\
last until cured by some other means."),
_("\
You just drank a bottle of water, reducing your thirst level and reducing the\n\
water level in the bottle.  You'll probably want to drop empty bottles,\n\
or you might want to save them to refill from a water source later."),
_("\
You just activated a grenade!  You probably want to throw it.  To throw an\n\
item, press 't' and then select the item to throw.  The maximum range depends\n\
on your strength and the object's weight and size.  Throwing isn't very good\n\
in most combat situations, but if you've got some fire power..."),
_("\
You just placed a trap.  Traps are permanent until set off, and can be an\n\
important defensive tactic, particularly when sleeping in unsafe territory.\n\
Try stepping on that _ -- don't worry, it's a harmless bubblewrap trap."),
_("\
You're carrying more volume than you have storage for, which means you are\n\
carrying some stuff in the crook of your arm or in some other awkward manner.\n\
While overloaded like this, you will suffer SEVERE encumberment penalties,\n\
making combat dangerous.  Either drop an item, or possibly wield something--\n\
the object you are wielding does not take up inventory space."),
_("\
To use a gun, first wield it with the 'w' key.  Next you need to reload using\n\
the 'r' key, assuming you have the proper ammo.  If you wish to unload your\n\
gun, possibly to change ammunition types, press 'U'."),
_("\
Once you have a loaded gun wielded, you can fire it.  Press the 'f' key to\n\
fire a single shot.  With many guns, you can fire a burst by pressing 'F'.\n\
You'll automatically target the last monster you shot at, or the closest\n\
monster to you.  You can change your target with the movement keys, or cycle\n\
through monsters with '<' and '>'.  To fire, press 'f' or '.'; to cancel, hit\n\
the escape key."),
_("\
After firing your gun, you will probably notice a Recoil alert in the lower\n\
right.  Generally speaking, the more damaging the ammo, the worse the recoil.\n\
Recoil severely reduces your chance to hit, but you can eliminate recoil by\n\
pausing ('.') for a turn or moving normally.  High strength reduces recoil,\n\
so weak characters might want to stick to .22 or 9mm guns."),
_("\
That yellow > next to you is a staircase leading down.  To go downtairs, step\n\
onto the staircase and press the '>' key.  Similarly, a yellow < is stairs\n\
leading up, and can be followed with the '<' key."),
_("\
It's dark down here!  Without a source of light, you'll be limited to seeing\n\
only one space in any direction.  You'll encounter darkness while underground\n\
or at night.  Press '<' to go back upstairs, and look around for a flashlight."),
_("\
It's dark down here!  Fortunately, you have a flashlight.  Press 'a' and then\n\
select your flashlight to turn it on."),
_("\
~ is a terrain symbol that indicates water.  From a water source like this,\n\
you can fill any containers you might have.  Step onto the water, then press\n\
the pickup key (',' or 'g'), then select a watertight container to put the\n\
water into."),

};

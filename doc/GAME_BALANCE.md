# Stat system scaling:
Ranges below will be using strength as the example since it's the easiest to itemize, but what the numbers mean is equivalent between all stats.

Minimal stat: 0 (Should only happen due to penalties, instant failure in most scenarios.  Basically actively dying, almost unable to even carry themselves.)

Very low stat: 4 (Lowest in chargen.  Either an average person who's injured or someone who's naturally very dainty and sedentary.)

Moderately low stat: 6 (Below average.  Sedentary job, not very active.)

Nominal stat: 8 ("Average" human.  Mostly sedentary job, somewhat active but not particularly a gym-goer, etc.)

Moderately high stat: 10 (Has a decent bit of strength.  Active job and/or works out consistently.)

Very high stat: 14 (Very strong.  Job is either based around fitness or they're dedicated to building a lot of muscle.  Should require a lot of effort to maintain.)

Maximal human stat: 20 (Extremely strong.  People who spend their whole lives dedicated to being as strong as possible, and with good genetics too.  Olympic powerlifters, record-setting strongmen, etc.)

Superhuman: >20 (Anything beyond this should require bionics or mutations.)


# Skill system scaling:
Minimum skill: 0 (You have no idea where to even start.  Even following clearly written directions is a challenge because you don't know basic terms and procedures.)

Beginning hobbyist level: 1-2 (You've done this enough that you understand how to do basic things, know the tools that are used, and follow the language.)

Proficient hobbyist level: 3-4 (You've used the tools for a while and they don't feel unfamiliar to you.  Given enough directions you can do most things.)

Early professional level: 5-6 (You've been using these tools a while and know not just how to use them, but how to do some impressive stuff, and some of the more efficient shortcuts and common pitfalls.)

Expert professional level: 7-8 (There's not much you don't know about this stuff and you'd be genuinely impressed if someone knew tricks you don't.)

Master level: 9-10 (While there may be very specific tricks you don't know yet, overall you pretty much have no room for major improvement in this skill.  Sure, you'll continue to get better, but only in subtle ways noticeable to you, not anything major.)


# Monster melee skill scaling:
Minimum skill: 0 (no melee potential; turret, fungal wall)

Nominal skill: 4 (average critter; most zeds & giant insects)

Notable skill: 6 (competent/carnivore; bear, wolf, police/survivor zeds)

Very high skill: 8 (dangerous opponent; dark wyrm, vinebeast)

Maximal skill: 10 (highest for balance purposes; jabberwock, tribot, shoggoth, gracken)


# Monster maximum damage scaling:
Minimum damage: 0 (no damage potential; spore cloud, hallucination)

Nominal damage: 4 (minimal threat; decayed zombie, blank body, cat)

Average damage: 6 (normal day one threat; zombie, wasp)

High damage: 10 (dangerous day one threat; tough zombie, wolf, zombie scientist)

Very high damage: 20+ (zombie predator, super soldier ant, mi-go) 

Maximum damage: 50 (highest for balance purposes; melded task force)


# Monster HP scaling:
Minimum HP: 1 (no ability to absorb damage; yellow chick, mosquito)

Average HP: ~100 (average critter; most basic zeds, slimes, soldier ants, fungaloids & triffids)

Notable HP: ~200 (unusually resilient; tiger, zombie master, mi-go)

Very high HP: ~500 (supernaturally resilient; zombie hulk, shoggoth, triffid heart, jabberwock)

Maximal HP: 800 (highest for balance purposes; wraith)

In general larger creatures should have higher HP as should more evolved blob creatures and alien and nether creatures.


# Monster Speed Scaling:
Minimum speed: 20 (very little ability to move, 1 km/h; crawling zombie)

Average speed: 70 (average critter, 3.5 km/h, can be walked away from; most basic zeds, slimes, triffids)

Notable speed: 100+ (unusually fast, over 5 km/h, can be run away from; feral human, bee, mi-go, zombie brute, zombie predator, jabberwock, manhack)

Very high speed: ~200 (extremely fast, about 10 km/h; moose, zombie hulk, hulking horror, gracken, jabberwock)

Maximal speed: 300 (highest for balance purposes; flying polyp)

Zombies are a bit faster than "shambling". Zombified versions of fast critters will remain fast, but in general the process slows the undead version. Further, under no circumstances should a day one zed be more than 100% faster than base character speed. Currently, this means "capped at 200". attack_cost flag should be used for monsters with speeds faster or slower than 100 unless faster or slower attacks are intended


# Monster Difficulty scaling:
Minimum: 0 (no danger; yellow chick, fly, goose)

Average Day 1 Difficulty: 4 (average critter; most basic zeds, slimes, soldier ants)

Average first evolution difficulty: ~20 (more dangerous; zombie brute, zombear, albino penguin, giant wasp, missile spider)

Notable difficulty: ~50 (unusually dangerous; zombie master, mi-go, antlered hammer, M202A1 autonomous TALON UGV, mad militia, mi-go slaver, alpha bee)

Very high difficulty: ~100 (supernaturally dangerous; zombie hulk, shoggoth, wraith, jabberwock, skeletal juggernaut, fused dragonflies, mi-go guard)

Maximum danger: 300 (highest for balance purposes; triffid heart, melded task force)


# Dodge System assumptions:
Dodge chance is based on attacker's melee skill and target's dex stat and dodge skill.

Successful dodges negate the attack and impose a cumulative penalty on dodges within the same turn.

## Dodge Use Cases:
An individual with no skill and nominal stats in ideal circumstances against a basic opponent should occasionally be able to dodge.

An individual with no skill and nominal stats in ideal circumstances against a skilled opponent should rarely if ever be able to dodge.

An individual with world-class dodging ability, in ideal circumstances against a basic opponent should have a negligible chance of failure.

An individual with world-class dodging ability, in ideal circumstances against a skilled opponent should have a moderate chance of failure.

The effect of increasing dodge skill has a growth rate with diminishing returns that accelerates sharply at the point where you move beyond the dodge a "regular" character is likely to achieve (7  and above)

The balance of melee versus dodge should favor dodge which, after all, isn't effective against a wide variety of other types of attacks.

Even a world class dodger should not be able to dodge continuously when attacked many times a turn.


# MELEE WEAPONS:

## To-Hit Value
The "to_hit" value of an object represents the base likelihood that it will solidly strike an enemy during an attack. This can then be modified by martial arts, skills, proficiences, etc... to get your final chance to-hit.

For basic objects it isn't important to get a perfect value since it's highly unlikely for players to use that item as a weapon. These items start with a base of -2 and, if you wish, you may go through the categories and manually add to the to-hit value the listed numbers based on the object's characteristics, then enter the final result number directly in the format "to_hit": X, (where X is the positive or negative integer you have decided on).

For proper weapons, such as those in the data\json\items\melee file, and common makeshift weapons like crowbars, etc... we use a slightly different system. Instead of a declared number, we instead use a group of more descriptive words so that we can easily audit and make sure these values are as close to reality as possible. Instead of adding up the numbers below, you simply select the correct word for each category and it will be automatically calculated from the base -2 to-hit number in-game instead.

The format for this is:
"to_hit": { "grip": "(parameter)", "length": "(parameter)", "surface": "(parameter)", "balance": "(parameter)" }

### Grip
Grip is a measure of how well you can control the weapon to quickly respond to situational changes.

-1 - "bad" - Particularly hard to grip items, (especially those that are innately slippery or very rounded with no obvious gripping edge) such as basketballs and barrels, or which are dangerous to hold because of very sharp edges, like scrap metal and broken glass.

+0 - "none" - Any object that doesn't fall into one of the categories below. Examples include 2x4s, computer monitors, wires, stingers and clothing. Basically, anything that has a grippable component, but which is too thick, too thin, or too flimsy to grab comfortably in a way that can reliably control the object.

+1 - "solid" - A weapon with a fairly solid grip, like a pipe, a rock, guitar neck, or pool cue.

+2 - "weapon" - A weapon with a dedicated grip shaped to the hand, like a sword, axe, knife, or police baton, or that is strapped to the body (or is a piece of the body). Fists would get a +2 bonus here, bringing them to "0" total, since none of the others would apply.

### Length
Length allows more surface area for potential contact, and reduces the need to control the positioning of the body to guarantee a hit. It also allows the player to strike from a safer distance, allowing them to worry more about trying to hit without being hit in return, and allows for swings with larger arcs, making dodging such a strike more difficult.

+0 - "hand" - Any object without a length bonus.

+1 - "short" - Objects that, when held, extend over a foot (1/3 of a meter) in length from the hand, but less than about 3 feet. A normal American 12-inch ruler is the handy boundary guide for when an item should switch over to a +1 bonus (the ruler, losing several inches when held, does not get one - unless you added a handle to it!).

+2 - "long" An object that is over 3 feet in length from the point where it is held. Includes swords, spears, quarterstaffs, poles, and a lot of other stuff.

### Striking Surface
Some weapons need to strike in a certain way to be effective. Others are more difficult to use "incorrectly".

-2 - "point" - Single-Point weapons - Picks, spears, syringes. Any weapon that has a single point that must contact the enemy in a specific way in order to deal a decent amount of damage. Also, weapons with difficult attack angles, like scythes, where the damaging part of the weapon is faced away from the enemy.

-1 - "line" - Line of damage weapons - Swords, knives, and other weapons that require a solid strike along a particular piece of the weapon, where the weapon can be said to have an attack angle, fall here. Weapons that have point attacks but are still effective without any solid hit, such as a nailboard, would also fall here.

+0 - "any" - attack-anywhere weapons - Clubs, pipes, maces, etc, where the weapon will be dealing good damage with a solid blow no matter how it is angled, because every surface is effectively a striking surface.

+1 - "every" - Weapons that can still do significant damage even with glancing blows would fall here. Jagged tearing weapons and electric weapons like a stun baton would fall here.

### Balance
A measure of how well-suited the item is for being swung/thrust/etc. This factors in overall balance of the weapon, weight is accounted for separately.

-2 - "clumsy" - Very clumsy or lopsided items ill-suited for swinging or thrusting. Characterized by requiring effort just to hold steady. frying pan or pot, chainsaw, chair, vacuum cleaner.

-1 - "uneven" - Balance of the object is uneven, but in a way that at least doesn't interfere with swinging. axes, sledgehammer, rifle, scythe, most polearms.

+0 - "neutral" - Neutral balance, neither well nor poorly weighted for the typical use. Heavy stick, rock, pool stick, kitchen knives, claw hammer, metal pipe, crowbar, handguns.

+1 - "good" - Well-balanced for swinging or stabbing. Baseball bat, golf club, swords, quarterstaff, knives.

## Damage
Weapon's relative strength is based on an approximate formula involving its damage, to-hit, techniques and few other factors.

### Damage per second
A melee's weapon damage per second (dps) is calculated past armor against a sample group of monsters with a range of dodge and armor values: a soldier zombie (low dodge, high bash and cut armor), a survivor zombie (medium dodge, some bash and cut armor), and a smoker zombie (high dodge, no armor).  This should correctly weigh accuracy, criticals, and damage without over valuing any of them.

In code, this is calculated using the item::effective_dps() function, which takes a character and a monster.  It calculates the relative accuracy of the character and weapon against the monster's defenses and determines the hit rate from a table lookup.  It also determines the number of critical hits.  Number of hits is hit rate * 10,000, and number of misses is 10,000 - number of hits.

For both critical and non-critical hits, average damage is calculated based on the weapon's stats and the user's skill.  Monster armor absorbs the damage, and then the damage is multiplied by the number of hits: either critical hits for the critical hit case, or total hits - critical hits for the non critical hit case.  If the weapon has the rapid strike technique, the total damage is halved, and then the average damage is recalculated, multiplied by 0.66, and absorbed by monster armor again to account for rapid strikes.

Number of moves is calculated as attack speed * ( number of misses + number of non-critical hits + number of critical hits ) for weapons without rapid strike, or attack speed * ( number of misses + number of non-critical hits / 2  + number of critical hits / 2  ) + attack speed / 2 * ( number of non-critical hits / 2  + number of critical hits / 2 ) for weapons without rapid strikes.

Damage per second against a particular monster is total damage * 100 / number of moves (100 for the 100 moves/second).  Overall dps is the average of the dps against the three reference monsters.

### Critical hits
A double critical can occur when a second hit roll is made against 1.5 * the monster's dodge.  Double critical hits have a higher chance of occurring than normal critical hits.  For each hit, the chance of achieving either a double critical hit or a normal critical hit is calculated, and then if a random number is less than the critical chance, the critical occurs.  Both double and normal critical hits have the same effect, but the chance of them occurring is different.

**Note** The critical hit system is stupid and complicated and produces weird results.  Double critical hits should have a chance of occurring when the original hit roll is more than 1 standard deviation above the mean, which is simple and faster to calculate than the current system.

### Other factors
Reach is worth +20% at reach 2, +35% at reach 3.

A weapon that is usable by a known martial art is worth +50%.

### Weapon tiers
Relative value should put the weapon into one of those categories:

<2 - Not weapons. Those items may be pressed into service, but are unlikely to be better than fists. Plastic bottles, rocks, boots.

2-5 - Tools not meant to strike and improvised weapons. Planks, pointy sticks, pipes, hammers.

6-11 - Dangerous tools or crude dedicated weapons. Golf clubs, two-by-swords, wooden spears, knife spears, hatchets, switchblades, tonfas, quarterstaves.

12-15 - Good dedicated weapons or the most dangerous of tools. Wood and fire axes, steel spears, electric carvers, kukris, bokken, machetes, barbed wire bats.

20-35 - Weapons of war, well designed to kill humans. Wakizashis, katanas, broadswords, zweihanders, combat knives, battle axes, war hammers, maces, morningstars.

35+ - Sci-fi stuff. Diamond katanas, monomolecular blades, lightsabers and chainswords.

Specific weapon balancing points:
20 - combat knives
22 - short blades
24 - long blades, short axes, and short flails
26 - two handed blades, long axes, most spears
28 - two handed axes and polearms
30 - combat spears

Improvised weapons generally have about 75% of the value of a real weapon.

## Other melee balancing factors
### Attack speed
Out of two weapons with same dpt, the faster one is generally better.
Faster weapons allow more damage granularity (less overkill), make it less likely to miss a turn (and thus dodge/block recharges) and make positioning easier.
Slower weapons will pierce armor better, but currently most enemies are very lightly armored.

### Damage type
At low skill, piercing damage suffers from scaling and bashing damage from damage limit due to low strength and skill. Cutting damage is not affected.
At high skill, bashing damage is generally the strongest, but still suffers from the damage limit.
Exotic damage types (currently only fire) do not scale with skills or crits.

# RANGE WEAPONS
## Automatic Fire
Guns with automatic fire are balanced around 1-second of cyclic fire, unless the cyclic or practical fire rate is less than 1 every second.  Rates of fire less than 1 shot every second are increased to 2.

## Magazines
### Reload times
The overall balance is that magazines themselves are slow to reload whereas changing a magazine should be fast. For standard box magazines a default `reload_time` of 100 (per round) is appropriate with this value increasing for poor quality or extended magazines. Guns themselves should also specify `reload` of 100 (per magazine) unless their magazines are particularly awkward to reload (eg. ammo belts). The game logic intrinsically handles higher volume magazines consuming more time to attach to a gun so you need not consider this.

### Weight
Increases proportional to capacity and should have a comparable ratio to similar magazines. Consider the base item to be a 10-round .223 factory specification box magazine which has a capacity:weight of 1:10. Increase the ratio markedly for poor quality magazines or more slightly for extended magazines. Smaller calibers should use a lower ratio. The `material` should have some effect, with plastic magazines weighing less.

### Volume
Scaled based upon the capacity relative to the `stack_size` of the ammo. For example 223 has a `stack size` of 20 so for 10 and 30 round magazines the volume would be 1 and 2. Extended magazine should always have larger volume than the standard type and for very large drum magazines consider applying an extra penalty. By default most handgun magazines should be volume 1 and most rifle magazines volume 2. Ammo belts should not specify volume as this will be determined from their length.

### Rarity
Overall balance is that pistol magazines are twice as common as rifle magazines and that for guns that spawn with magazines these are always the standard capacity versions. Consider 9x19mm and .223 to be the defaults with everything else more rare. Some locations have more specific balance requirements:

Location          | Description                                               | With guns | Damaged   | Example
------------------|-----------------------------------------------------------|-----------|-----------|--------------------------
Military site     | Only source of milspec magazines and ammo belts           | Never     | Never     | LW-56, .223 ammo belt
Gun store         | Standard and extended capacity magazines                  | Never     | Never     | STANAG-30, Glock extended
Police armory     | Mostly pistol magazines, especially 9x19mm, never extended| Sometimes | Never     | Glock, MP5 magazine
SWAT truck        | Police or military magazines, occasionally extended       | Sometimes | Rarely    | MP5 extended
Survivor basement | Anything except milspec weighted towards common types     | Often     | Sometimes | Saiga mag, M1911 extended
Military surplus  | Older military magazines that are not current issue       | Never     | Rarely    | M9 mag, STEN magazine
Pawn shop         | Anything except milspec weighted towards unusual calibers | Never     | Rarely    | Makarov mag, AK-74 mag
Everywhere else   | Predominately 9mm and 223. Always with standard magazine  | Often     | Sometimes | Ruger 223 mag, M1911 mag

## Archery damage
Bow damage is based on the momentum achieved in the projectile.  Since arrows and bolts have sharp cutting surfaces, the penetration and therefore damage achieved is based on the projectile's capacity for slicing through tissues.  The arrow has a modifier based on construction, material and design, most critically centered around the effectiveness of the head.  Base damage is calculated from momentum by taking momentum in Slug-foot-seconds, multiplying by 150 and subtracting 32. This was arrived at by taking well-regarded bowhunting guidelines and determining the damage numbers necessary for a kill of various game on a critical hit, see tests/archery_damage_test.cpp for details.

## Ammo stats
The default damage, (**Dmg**) of a given **Cartridge** shot through a normal firearm is the square root of a round's muzzle energy in joules, (**M.E.**), rounded to the nearest integer with an arbitrary increase or decrease to account for terminal ballistics of different projectiles. Normal in this case is full/total metal jacketed, lead core projectiles, including slugs out of shotguns. Damage of handloaded ammo is set to 92% (rounded down) of their factory counterparts. Damage of smokeless cartridges loaded with black powder is set to 76% (rounded down) of their factory counterparts, and damage of smokeless cartridges with bullet diameter less than .30 inches loaded with black powder is set to 57% (rounded down) of their factory counterparts. A table calculating a given round's damage has been prepared and is provided below.


Each cartridge has had a curve plotted for barrel length vs damage for standard loads, sourced from reloading manuals, manufacturers' load data, and/or wikipedia, and modelled with interior ballistics software. Each curve had a logarithmic regression fit to it, and the generic formula to reproduce it is **Dmg** = ( **A** x Ln( **Brl** ) )+ **B**. For each cartridge, the default damage, **Dmg** has been calculated using its **A** coefficient and **B** offset and **Brl**. For firearms whose barrel lengths differ from **Brl**, a corresponding damage modifier should be calculated using the formula and the provided default damage.

Each cartridge also has a default barrel length (**Brl**) listed determined based loosely on cartridge length (with some exceptions). Friction losses were not modelled. Plugging in optimistically long barrel lengths will not yield accurate data. Real world barrels should provide useful estimates for determining modifiers. Any barrel featuring a separate chamber (e.g. revolvers, the HK G11, etc) should have the length of this chamber added to the barrel length as part of these calculations.  What is given here as **OAL** is the overall length of the cartridge. Barrel lengths, (**Brl**), less than the overall length of the cartridge, (**OAL**), should default to 0 ballistic damage.


Any cartridge more energetic than 20mmx102, or otherwise dissimilar to these cartridges (a baseball, anything faster than 6190 FPS, etc) is not appropriate to consider with just the square root of its muzzle energy.

For reference, each cartridge's bullet diameter, **Dia**  and weight, **Proj. wt**, have been provided.


| **Cartridge**            | **Brl** | **Dam** | **M.E.**  | **A**   | **B**   | **OAL** | **Dia**  | **Proj. Wt** |
|--------------------------|---------|---------|-----------|---------|---------|---------|----------|--------------|
| .25 ACP                  | 6.0 in  | 11.5    | 132.3 J   | 1.8941  | 8.096   | 0.9 in  | 0.251 in | 50.0 gr      |
| .22 LR                   | 6.0 in  | 12.3    | 151.3 J   | 2.5203  | 7.748   | 1.0 in  | 0.223 in | 32.0 gr      |
| 8 mm LebelRevolver       | 6.0 in  | 12.4    | 153.8 J   | 2.6618  | 7.671   | 1.5 in  | 0.327 in | 120.0 gr     |
| .32 SWLong               | 6.0 in  | 12.5    | 156.3 J   | 3.5661  | 6.149   | 1.3 in  | 0.312 in | 71.0 gr      |
| .32 ACP                  | 6.0 in  | 12.7    | 161.3 J   | 2.6885  | 7.868   | 1.0 in  | 0.312 in | 71.0 gr      |
| .17 HMR                  | 6.0 in  | 13.1    | 171.6 J   | 4.6416  | 4.768   | 1.4 in  | 0.172 in | 17.0 gr      |
| .22 WMR                  | 6.0 in  | 15.2    | 231.0 J   | 4.29    | 7.541   | 1.4 in  | 0.223 in | 40.0 gr      |
| 7.62 NagantRuss.         | 6.0 in  | 15.5    | 240.3 J   | 3.3784  | 9.492   | 1.5 in  | 0.308 in | 95.0 gr      |
| .455 Webley              | 6.0 in  | 15.7    | 246.5 J   | 3.4003  | 9.569   | 1.3 in  | 0.454 in | 200.0 gr     |
| .44 SWSpecial            | 6.0 in  | 17.3    | 299.3 J   | 4.6001  | 9.068   | 1.6 in  | 0.429 in | 225.0 gr     |
| .380 Auto                | 6.0 in  | 17.4    | 302.8 J   | 3.7784  | 10.62   | 1.0 in  | 0.355 in | 95.0 gr      |
| .32 HRMagnum             | 6.0 in  | 17.5    | 306.3 J   | 4.9173  | 8.652   | 1.4 in  | 0.312 in | 71.0 gr      |
| .25-20 Win. CF           | 6.0 in  | 17.5    | 306.3 J   | 5.5441  | 7.53    | 1.6 in  | 0.257 in | 87.0 gr      |
| .32-20 Win.              | 6.0 in  | 17.5    | 306.3 J   | 6.2949  | 6.23    | 1.6 in  | 0.312 in | 100.0 gr     |
| .38 Special              | 6.0 in  | 17.7    | 313.3 J   | 5.3041  | 8.225   | 1.6 in  | 0.357 in | 140.0 gr     |
| .30 Luger                | 6.0 in  | 17.9    | 320.4 J   | 3.665   | 11.36   | 1.2 in  | 0.308 in | 90.0 gr      |
| 9 mm Makarov             | 6.0 in  | 18.0    | 324.0 J   | 3.8329  | 11.11   | 1.0 in  | 0.364 in | 95.0 gr      |
| .44 SWRussian            | 6.0 in  | 18.6    | 346.0 J   | 4.5498  | 10.47   | 1.4 in  | 0.429 in | 200.0 gr     |
| 8 mm Nambu               | 6.0 in  | 18.7    | 349.7 J   | 5.3701  | 9.071   | 1.3 in  | 0.320 in | 100.0 gr     |
| .38 SW                   | 6.0 in  | 19.1    | 364.8 J   | 3.767   | 12.32   | 1.2 in  | 0.357 in | 121.0 gr     |
| 4.6 x30 HK               | 6.0 in  | 19.7    | 388.1 J   | 6.7641  | 7.597   | 1.5 in  | 0.183 in | 42.0 gr      |
| .218 Bee                 | 6.0 in  | 21.0    | 441.0 J   | 8.5183  | 5.782   | 1.7 in  | 0.223 in | 40.0 gr      |
| 7.62 x25 Tokarev         | 6.0 in  | 21.1    | 445.2 J   | 4.753   | 12.56   | 1.4 in  | 0.308 in | 90.0 gr      |
| .45 SWSchofield          | 6.0 in  | 21.4    | 458.0 J   | 4.8278  | 12.72   | 1.4 in  | 0.451 in | 230.0 gr     |
| .45 ACP                  | 6.0 in  | 21.7    | 470.9 J   | 4.6379  | 13.42   | 1.3 in  | 0.451 in | 230.0 gr     |
| .41 Act. Exp.            | 6.0 in  | 22.0    | 484.0 J   | 3.6826  | 15.4    | 1.2 in  | 0.410 in | 180.0 gr     |
| .38 Super                | 6.0 in  | 22.1    | 488.4 J   | 4.8588  | 13.42   | 1.3 in  | 0.355 in | 124.0 gr     |
| .38-40 Win. CF           | 6.0 in  | 22.8    | 519.8 J   | 6.7429  | 10.69   | 1.6 in  | 0.400 in | 175.0 gr     |
| 9 mm Luger               | 6.0 in  | 23.5    | 552.3 J   | 4.4284  | 15.58   | 1.2 in  | 0.355 in | 115.0 gr     |
| 5.7 x28 FN               | 6.0 in  | 23.5    | 552.3 J   | 6.1092  | 12.59   | 1.6 in  | 0.224 in | 40.0 gr      |
| 9 x21                    | 6.0 in  | 24.0    | 576.0 J   | 4.3384  | 16.21   | 1.2 in  | 0.355 in | 124.0 gr     |
| .45 Colt                 | 6.0 in  | 24.9    | 620.0 J   | 6.9275  | 12.44   | 1.6 in  | 0.451 in | 225.0 gr     |
| .50 GI                   | 6.0 in  | 25.2    | 635.0 J   | 3.8964  | 18.17   | 1.3 in  | 0.500 in | 275.0 gr     |
| .44-40 Win. CF           | 6.0 in  | 25.6    | 655.4 J   | 7.5595  | 12.07   | 1.6 in  | 0.429 in | 200.0 gr     |
| .40 SW                   | 6.0 in  | 26.3    | 691.7 J   | 5.0626  | 17.2    | 1.1 in  | 0.400 in | 165.0 gr     |
| .357 SIG                 | 6.0 in  | 27.0    | 729.0 J   | 4.8888  | 18.2    | 1.1 in  | 0.355 in | 124.0 gr     |
| .357 Magnum              | 6.0 in  | 27.4    | 750.8 J   | 6.3938  | 15.91   | 1.6 in  | 0.357 in | 140.0 gr     |
| .327 FederalMagnum       | 6.0 in  | 27.6    | 761.8 J   | 7.6295  | 13.93   | 1.5 in  | 0.312 in | 100.0 gr     |
| .30 Carbine              | 6.0 in  | 27.9    | 778.4 J   | 7.6922  | 14.1    | 1.7 in  | 0.308 in | 110.0 gr     |
| 10 mm Auto               | 6.0 in  | 28.4    | 806.6 J   | 5.7868  | 18.05   | 1.3 in  | 0.400 in | 165.0 gr     |
| .410 Bore 3in            | 18.0 in | 31.0    | 961.0 J   | 9.4303  | 3.699   | 2.9 in  | 0.408 in | 109.3 gr     |
| 28 Gauge 2.75in          | 18.0 in | 33.5    | 1122.3 J  | 10.195  | 4.048   | 2.6 in  | 0.540 in | 194.0 gr     |
| .41 Rem. Mag.            | 6.0 in  | 33.7    | 1135.7 J  | 8.8293  | 17.93   | 1.6 in  | 0.410 in | 210.0 gr     |
| .44 Rem. Mag.            | 6.0 in  | 34.1    | 1162.8 J  | 8.1882  | 19.38   | 1.6 in  | 0.429 in | 240.0 gr     |
| .222 Rem. Mag.           | 16.0 in | 34.2    | 1169.6 J  | 11.962  | 1.06    | 2.3 in  | 0.224 in | 55.0 gr      |
| 4.73 x33 Caseless        | 16.0 in | 34.6    | 1197.2 J  | 10.816  | 4.645   | 1.3 in  | 0.185 in | 51.0 gr      |
| .222 Rem.                | 16.0 in | 36.3    | 1317.7 J  | 12.302  | 2.167   | 2.1 in  | 0.224 in | 55.0 gr      |
| .22 PPC USA              | 16.0 in | 36.5    | 1332.3 J  | 12.269  | 2.478   | 2.1 in  | 0.224 in | 55.0 gr      |
| 5.45 x39 mm              | 16.0 in | 36.9    | 1361.6 J  | 12.849  | 1.2319  | 2.3 in  | 0.222 in | 52.8 gr      |
| .480 Ruger               | 6.0 in  | 37.7    | 1421.3 J  | 8.9024  | 21.76   | 1.7 in  | 0.475 in | 325.0 gr     |
| .224 Weath. Mag.         | 16.0 in | 37.9    | 1436.4 J  | 12.85   | 2.318   | 2.3 in  | 0.224 in | 55.0 gr      |
| .220 Swift               | 16.0 in | 37.9    | 1436.4 J  | 15.58   | -5.247  | 2.7 in  | 0.224 in | 55.0 gr      |
| .45 Win. Mag.            | 6.0 in  | 38.2    | 1459.2 J  | 10.582  | 19.27   | 1.6 in  | 0.451 in | 230.0 gr     |
| .243 Win.                | 16.0 in | 38.5    | 1482.3 J  | 15.444  | -4.294  | 2.7 in  | 0.243 in | 80.0 gr      |
| .460 Rowland             | 6.0 in  | 38.6    | 1490.0 J  | 9.5775  | 21.44   | 1.2 in  | 0.451 in | 200.0 gr     |
| 6 mm PPC                 | 16.0 in | 39.3    | 1544.5 J  | 12.783  | 3.859   | 2.2 in  | 0.243 in | 70.0 gr      |
| .224 Valkyrie            | 16.0 in | 39.4    | 1552.4 J  | 12.1    | 5.827   | 2.2 in  | 0.224 in | 77.0 gr      |
| .223 Rem.                | 16.0 in | 39.4    | 1552.4 J  | 13.454  | 2.11    | 2.3 in  | 0.224 in | 55.0 gr      |
| .225 Win.                | 16.0 in | 39.7    | 1576.1 J  | 16.008  | -4.682  | 2.5 in  | 0.224 in | 55.0 gr      |
| .375 Win.                | 16.0 in | 40.1    | 1608.0 J  | 11.074  | 9.359   | 2.6 in  | 0.377 in | 255.0 gr     |
| 6.5 x52 Carcano          | 16.0 in | 40.2    | 1616.0 J  | 15.01   | -1.385  | 3.0 in  | 0.267 in | 160.0 gr     |
| .454 Casull              | 6.0 in  | 40.3    | 1624.1 J  | 7.8628  | 18.75   | 1.7 in  | 0.452 in | 300.0 gr     |
| .300 Blackout            | 16.0 in | 40.3    | 1624.1 J  | 9.6958  | 13.42   | 2.2 in  | 0.308 in | 135.0 gr     |
| .22-250 Rem.             | 16.0 in | 40.8    | 1664.6 J  | 14.664  | 0.16    | 2.4 in  | 0.224 in | 55.0 gr      |
| 6 mm Rem.                | 16.0 in | 41.3    | 1705.7 J  | 16.29   | -3.82   | 2.8 in  | 0.243 in | 80.0 gr      |
| .257 Roberts             | 16.0 in | 41.5    | 1722.3 J  | 16.218  | -3.5    | 2.8 in  | 0.257 in | 87.0 gr      |
| 6.5 x50 Arisaka          | 16.0 in | 41.5    | 1722.3 J  | 15.189  | -0.603  | 2.9 in  | 0.264 in | 139.0 gr     |
| 6.5 Grendel              | 16.0 in | 42.1    | 1772.4 J  | 12.435  | 7.647   | 2.3 in  | 0.264 in | 107.0 gr     |
| .250 Savage              | 16.0 in | 43.0    | 1849.0 J  | 16.213  | -1.976  | 2.5 in  | 0.257 in | 87.0 gr      |
| 7.62 x39 M43             | 16.0 in | 43.5    | 1892.3 J  | 13.431  | 6.272   | 2.2 in  | 0.310 in | 123.0 gr     |
| 7.35 x52 Carcano         | 16.0 in | 43.5    | 1892.3 J  | 16.04   | -0.936  | 2.8 in  | 0.298 in | 128.0 gr     |
| .50 A. E.                | 6.0 in  | 43.8    | 1918.4 J  | 15.8    | 15.47   | 1.6 in  | 0.500 in | 300.0 gr     |
| .240 Weath. Mag.         | 16.0 in | 44.5    | 1980.3 J  | 18.671  | -7.226  | 3.1 in  | 0.243 in | 85.0 gr      |
| 16 Gauge 2in             | 18.0 in | 44.7    | 1998.1 J  | 10.763  | 13.58   | 2.5 in  | 0.660 in | 448.0 gr     |
| .30-30 Win.              | 16.0 in | 44.7    | 1998.1 J  | 14.388  | 4.842   | 2.6 in  | 0.308 in | 150.0 gr     |
| .38-55 Win.              | 16.0 in | 44.8    | 2007.0 J  | 11.419  | 13.13   | 2.5 in  | 0.377 in | 255.0 gr     |
| .40-65 Win.              | 16.0 in | 44.9    | 2016.0 J  | 11.202  | 13.85   | 2.5 in  | 0.410 in | 400.0 gr     |
| 6.5 x55 Swedish          | 16.0 in | 45.3    | 2052.1 J  | 17.872  | -4.209  | 3.2 in  | 0.264 in | 129.0 gr     |
| 8 x40 (CDDA)             | 16.0 in | 45.8    | 2097.6 J  | 15.976  | 1.529   | 1.6 in  | 0.323 in | 127.0 gr     |
| .35 Rem.                 | 16.0 in | 46.0    | 2116.0 J  | 14.667  | 5.334   | 2.5 in  | 0.358 in | 200.0 gr     |
| .350 Legend              | 16.0 in | 46.3    | 2143.7 J  | 13.378  | 9.245   | 2.3 in  | 0.355 in | 147.0 gr     |
| .270 WSM                 | 16.0 in | 46.3    | 2143.7 J  | 18.229  | -4.268  | 2.9 in  | 0.277 in | 130.0 gr     |
| .40-70 Sharps Straight   | 16.0 in | 46.5    | 2162.3 J  | 14.549  | 6.176   | 3.3 in  | 0.408 in | 400.0 gr     |
| .32 Win. Spec.           | 16.0 in | 46.6    | 2171.6 J  | 15.378  | 3.979   | 2.6 in  | 0.321 in | 170.0 gr     |
| .25-06 Rem.              | 16.0 in | 46.6    | 2171.6 J  | 19.427  | -7.293  | 3.3 in  | 0.257 in | 100.0 gr     |
| .500 WyomingExpress      | 6.0 in  | 46.7    | 2180.9 J  | 11.053  | 26.93   | 1.8 in  | 0.500 in | 400.0 gr     |
| 7-30 Waters              | 16.0 in | 46.9    | 2199.6 J  | 14.605  | 6.362   | 2.6 in  | 0.284 in | 139.0 gr     |
| 20 Gauge 2.75in          | 18.0 in | 47.2    | 2227.8 J  | 11.228  | 14.72   | 2.6 in  | 0.610 in | 398.0 gr     |
| .460 SWMagnum            |  6.0 in | 47.2    | 2227.8 J  | 15.949  | 18.6    | 2.2 in  | 0.452 in | 300.0 gr     |
| .30-40 Krag              | 16.0 in | 47.8    | 2284.8 J  | 16.672  | 1.604   | 3.1 in  | 0.308 in | 180.0 gr     |
| .264 Win. Mag.           | 16.0 in | 48.1    | 2313.6 J  | 20.447  | -8.58   | 3.3 in  | 0.264 in | 140.0 gr     |
| .500 SWMagnum            |  6.0 in | 48.6    | 2362.0 J  | 10.823  | 29.2    | 2.3 in  | 0.500 in | 375.0 gr     |
| .257 Weath. Mag          | 16.0 in | 48.9    | 2391.2 J  | 21.697  | -11.27  | 3.2 in  | 0.257 in | 87.0 gr      |
| .300 Savage              | 16.0 in | 49.0    | 2401.0 J  | 15.767  | 5.24    | 2.6 in  | 0.308 in | 150.0 gr     |
| .30-06 Spring.           | 16.0 in | 49.1    | 2410.8 J  | 19.926  | -6.152  | 3.3 in  | 0.308 in | 150.0 gr     |
| .260 Rem                 | 16.0 in | 49.3    | 2430.5 J  | 16.959  | 2.232   | 2.8 in  | 0.264 in | 140.0 gr     |
| 6.5 Creedmoor            | 16.0 in | 49.4    | 2440.4 J  | 18.697  | -2.428  | 2.8 in  | 0.264 in | 120.0 gr     |
| 12 Gauge 2.75 in         | 18.0 in | 49.5    | 2450.3 J  | 11.733  | 15.54   | 2.5 in  | 0.730 in | 437.5 gr     |
| 7 x57 mm                 | 16.0 in | 49.5    | 2450.3 J  | 16.293  | 4.341   | 3.1 in  | 0.284 in | 175.0 gr     |
| .280 Rem.                | 16.0 in | 49.5    | 2450.3 J  | 19.654  | -4.98   | 3.3 in  | 0.284 in | 162.0 gr     |
| .270 Win.                | 16.0 in | 50.0    | 2500.0 J  | 20.266  | -6.237  | 3.3 in  | 0.277 in | 100.0 gr     |
| 8 mm Lebel (Rifle)       | 16.0 in | 50.7    | 2570.5 J  | 17.863  | 1.137   | 2.9 in  | 0.327 in | 200.0 gr     |
| .303 British             | 16.0 in | 50.9    | 2590.8 J  | 20.002  | -4.521  | 3.1 in  | 0.312 in | 174.0 gr     |
| 7 mm -08 Rem.            | 16.0 in | 51.0    | 2601.0 J  | 17.061  | 3.66    | 2.8 in  | 0.284 in | 154.0 gr     |
| 7.62 x54 R               | 16.0 in | 51.0    | 2601.0 J  | 20.641  | -6.211  | 3.0 in  | 0.312 in | 150.0 gr     |
| .458 Socom               | 16.0 in | 51.2    | 2621.4 J  | 12.394  | 16.87   | 2.0 in  | 0.458 in | 350.0 gr     |
| 10 Gauge 3.75in          | 18.0 in | 51.7    | 2672.9 J  | 17.226  | 1.931   | 3.3 in  | 0.775 in | 765.0 gr     |
| 7.7 mm x58 Arisaka       | 16.0 in | 52.0    | 2704.0 J  | 18.587  | 0.456   | 3.1 in  | 0.312 in | 165.0 gr     |
| 12 Gauge 3in             | 18.0 in | 52.3    | 2735.3 J  | 14.669  | 9.939   | 2.8 in  | 0.730 in | 601.0 gr     |
| 7.65 x53 Arg. Belg.      | 16.0 in | 52.6    | 2766.8 J  | 19.906  | -2.602  | 3.0 in  | 0.312 in | 174.0 gr     |
| 8 x57 IS                 | 16.0 in | 52.8    | 2787.8 J  | 18.978  | 0.207   | 2.9 in  | 0.323 in | 170.0 gr     |
| .284 Win.                | 16.0 in | 53.1    | 2819.6 J  | 18.773  | 1.053   | 2.8 in  | 0.284 in | 139.0 gr     |
| 7 mm Rem. Mag.           | 16.0 in | 53.6    | 2873.0 J  | 21.967  | -7.284  | 3.3 in  | 0.284 in | 154.0 gr     |
| .50 Beowulf              | 16.0 in | 54.1    | 2926.8 J  | 17.126  | 6.626   | 2.3 in  | 0.500 in | 335.0 gr     |
| .348 Win.                | 16.0 in | 54.1    | 2926.8 J  | 17.61   | 5.283   | 2.8 in  | 0.348 in | 200.0 gr     |
| .308 Win.                | 16.0 in | 54.1    | 2926.8 J  | 16.982  | 7.027   | 2.8 in  | 0.308 in | 168.0 gr     |
| .444 Marlin              | 16.0 in | 54.2    | 2937.6 J  | 15.535  | 11.09   | 2.6 in  | 0.429 in | 240.0 gr     |
| .270 Weath. Mag.         | 16.0 in | 54.5    | 2970.3 J  | 21.818  | -5.993  | 3.3 in  | 0.277 in | 140.0 gr     |
| 20 x66 caseless (CDDA)   | 16.0 in | 55.3    | 3058.1 J  | 19.641  | 0.873   | 2.6 in  | 0.775 in | 765.0 gr     |
| 7 mm WSM                 | 16.0 in | 55.5    | 3080.3 J  | 21.035  | -2.818  | 2.9 in  | 0.284 in | 139.0 gr     |
| 7 mm RemSA               | 16.0 in | 55.7    | 3102.5 J  | 19.124  | 2.648   | 2.8 in  | 0.284 in | 150.0 gr     |
| .450 Bushmaster          | 16.0 in | 55.9    | 3124.8 J  | 14.666  | 15.23   | 2.3 in  | 0.452 in | 250.0 gr     |
| .358 Win.                | 16.0 in | 55.9    | 3124.8 J  | 17.741  | 6.718   | 2.8 in  | 0.358 in | 220.0 gr     |
| 1.23 Ln(8 mm -06) (CDDA) | 16.0 in | 56.8    | 3226.2 J  | 18.643  | 5.074   | 3.2 in  | 0.323 in | 213.0 gr     |
| .45-70 Govt.             | 16.0 in | 57.0    | 3249.0 J  | 17.198  | 9.301   | 2.6 in  | 0.458 in | 400.0 gr     |
| .300 RemSAUltra          | 16.0 in | 57.7    | 3329.3 J  | 20.241  | 1.541   | 2.8 in  | 0.308 in | 180.0 gr     |
| .300 WSM                 | 16.0 in | 58.0    | 3364.0 J  | 20.172  | 2.076   | 2.9 in  | 0.308 in | 190.0 gr     |
| .45-90 Win. WM           | 16.0 in | 59.2    | 3504.6 J  | 14.041  | 20.31   | 2.8 in  | 0.458 in | 400.0 gr     |
| .300 Win. Mag.           | 16.0 in | 59.2    | 3504.6 J  | 23.479  | -5.913  | 3.3 in  | 0.308 in | 180.0 gr     |
| .35 Whelen               | 16.0 in | 60.0    | 3600.0 J  | 20.823  | 2.253   | 3.3 in  | 0.358 in | 225.0 gr     |
| .338 WinMag.             | 16.0 in | 61.6    | 3794.6 J  | 22.354  | -0.416  | 3.3 in  | 0.338 in | 210.0 gr     |
| .277 Fury                | 16.0 in | 63.3    | 4006.9 J  | 23.596  | -2.095  | 2.8 in  | 0.277 in | 135.0 gr     |
| .450 Raptor              | 16.0 in | 63.5    | 4032.3 J  | 15.709  | 19.97   | 2.3 in  | 0.452 in | 300.0 gr     |
| 7 mm STW                 | 26.0 in | 65.3    | 4264.1 J  | 23.081  | -9.917  | 3.7 in  | 0.284 in | 150.0 gr     |
| 7 mm Weath. Mag.         | 26.0 in | 66.2    | 4382.4 J  | 21.83   | -4.881  | 3.4 in  | 0.284 in | 150.0 gr     |
| 7 mm RemUltra            | 26.0 in | 68.4    | 4678.6 J  | 25.875  | -15.92  | 3.6 in  | 0.284 in | 162.0 gr     |
| .300 H. H. Mag.          | 26.0 in | 68.4    | 4678.6 J  | 22.744  | -5.675  | 3.6 in  | 0.308 in | 165.0 gr     |
| 8 mm Rem. Mag.           | 26.0 in | 69.8    | 4872.0 J  | 23.501  | -6.809  | 3.6 in  | 0.323 in | 180.0 gr     |
| .300 Weath. Mag.         | 26.0 in | 70.9    | 5026.8 J  | 24.25   | -8.124  | 3.6 in  | 0.308 in | 180.0 gr     |
| .45-110 SharpsStraight   | 26.0 in | 72.2    | 5212.8 J  | 19.261  | 9.482   | 3.4 in  | 0.458 in | 500.0 gr     |
| .300 RemUltraMag         | 26.0 in | 72.9    | 5314.4 J  | 25.415  | -9.908  | 3.6 in  | 0.308 in | 200.0 gr     |
| .375 H. H. Mag.          | 26.0 in | 73.9    | 5461.2 J  | 22.934  | -0.778  | 3.6 in  | 0.375 in | 285.0 gr     |
| .338 RemUltraMag         | 26.0 in | 74.5    | 5550.3 J  | 24.681  | -5.938  | 3.6 in  | 0.338 in | 225.0 gr     |
| .30-378 Weath. Mag.      | 26.0 in | 75.1    | 5640.0 J  | 28.626  | -18.2   | 3.7 in  | 0.308 in | 180.0 gr     |
| .458 Win. Mag.           | 16.0 in | 75.7    | 5730.5 J  | 24.043  | 9.01    | 3.3 in  | 0.458 in | 350.0 gr     |
| .338-378 Weath. Mag.     | 26.0 in | 75.8    | 5745.6 J  | 26.717  | -11.2   | 3.8 in  | 0.338 in | 210.0 gr     |
| .378 Weath. Mag.         | 26.0 in | 76.5    | 5852.3 J  | 27.778  | -13.98  | 3.7 in  | 0.375 in | 235.0 gr     |
| .416 Rigby               | 26.0 in | 80.1    | 6416.0 J  | 26.908  | -7.584  | 3.8 in  | 0.416 in | 350.0 gr     |
| .375 RemUltraMag         | 26.0 in | 81.0    | 6561.0 J  | 25.988  | -3.658  | 3.6 in  | 0.375 in | 285.0 gr     |
| .470 Nitro Exp.          | 26.0 in | 82.6    | 6822.8 J  | 26.399  | -3.37   | 3.9 in  | 0.474 in | 500.0 gr     |
| .416 Rem. Mag.           | 26.0 in | 83.2    | 6922.2 J  | 24.774  | 2.448   | 3.6 in  | 0.416 in | 350.0 gr     |
| 7.92 x94 M318            | 26.0 in | 84.1    | 7072.8 J  | 38.767  | -42.24  | 4.0 in  | 0.323 in | 220.0 gr     |
| .500 Nitro Exp. 3in      | 26.0 in | 91.4    | 8354.0 J  | 25.554  | 8.19    | 3.7 in  | 0.510 in | 570.0 gr     |
| .577 Nitro Exp. 3in      | 26.0 in | 95.2    | 9063.0 J  | 27.24   | 6.438   | 3.7 in  | 0.583 in | 750.0 gr     |
| .600 Nitro Exp.          | 26.0 in | 99.0    | 9801.0 J  | 27.84   | 8.29    | 3.7 in  | 0.620 in | 900.0 gr     |
| .460 Weath. Mag.         | 26.0 in | 99.4    | 9880.4 J  | 28.8    | 5.599   | 3.8 in  | 0.458 in | 500.0 gr     |
| .700 Nitro Exp. 3in      | 26.0 in | 108.3   | 11728.9 J | 37.488  | -13.87  | 3.7 in  | 0.700 in | 1000.0 gr    |
| .50 BMG                  | 46.0 in | 141.5   | 20022.3 J | 45.425  | -32.41  | 5.4 in  | 0.510 in | 650.0 gr     |
| 14.5 x114 Russ.          | 46.0 in | 179.5   | 32220.3 J | 64.296  | -66.67  | 6.1 in  | 0.588 in | 978.0 gr     |
| 20 mm x138               | 46.0 in | 198.3   | 39322.9 J | 78.524  | -102.3  | 8.0 in  | 0.787 in | 1852.0 gr    |
| 20 mm x102               | 46.0 in | 209.3   | 43806.5 J |  63.381 | - 33.41 | 6.6 in  | 0.820 in | 1521.0 gr    |



# LIQUIDS:
Multi-charge items are weighed by the charge/use.  If you have an item that contains 40 uses, it'll weigh 40x as much (when found in-game) as you entered in the JSON. Liquids are priced by the 250mL unit, but handled in containers.  This can cause problems if you create something that comes in (say) a gallon jug (15 charges) and price it at the cost of a jug's worth: it'll be 15x as expensive as intended.

To that end, here's a list of containers with non-one volume.  If you have something spawn in 'em, divide the "shelf" price by this value to arrive at the correct price to list in the JSON.

- plastic bottle: 2

- glass jar: 2

- glass bottle: 3

- plastic canteen: 6

- 3L glass jar: 12, as expected

- gallon jug: 15

# Diamond weapons
Diamond weapons should be uniform in their CVD machine requirements.
Coal requirements are `floor((weapon_volume+1)/2)*25`.
Hydrogen requirements are `coal_requirements/2.5`.

# MUTATIONS
Mutations are given completely subjective point values.  The most important factor is that mutations that adversely affect a character are given a negative point value, or positive for beneficial mutations.  The chance of obtaining a positive or negative mutation varies based on Instability (a counter that increases by a default of 100 when a mutation is gained or lost and decays by 1 per in-game day by default).  0 point mutations will always have a 10% chance of appearing.  There is a 90% chance to obtain a good mutation until approximately 800 Instability.  There is an equal chance (45% each) of obtaining a good or bad mutation at approximately 2800 Instability.  There is an approximately 70% chance of obtaining a bad mutation at 10000 Instability, which will be the cap after a current test phase where it is capped at 8000.

# Preparing Food and Water:

"surface_heat" uses a base cost of 20 kJ. Its various options are then given charge costs equal to their efficiency - 80% for induction or a microwave (25u battery, 1:1 kJ:battery), 60% for basic electric (35u battery 1:1 kJ:battery), 35% for most combustible fuels - gasoline (2u fuel 34:1 kJ:fuel), kerosene (2u fuel 34:1 kJ:fuel), propane (3u fuel 25:1 kJ:fuel), acetylene (1u fuel 50:1 kJ:fuel) and ethanol (3u fuel 25:1 kJ:fuel). 25% for hexamine (2u fuel 40:1 kJ:fuel) and 16% for coal/charcoal (4u fuel 32:1 kJ:fuel).

Food recipes that use surface_heat are made up of a combination of individual costs per type of ingredient in most cases. 
- 1u of surface_heat: 1u of flour, up to 2u of butter/oil, 1u of sugar_standard, cook one corn tortilla, process 100 grams of fruit
- 2u of surface_heat: one scrap of meat (30 grams), 1u of non-raw milk, 1u of batter (breading/frying), 100 grams of non-startchy veggy (for things like potatoes that are highly starchy, assume they are equal to their weight in flour - 1u of flour weighs 13 grams)
- 3u of surface_heat: cook one unit of oatmeal.
- 4u of surface_heat: cook one unit of beans, rice or lentils, use cooked meat in a recipe, cook 1u of raw organs.
- 5u of surface_heat: cook one unit of cornmeal or bread flour.
- 6u of surface_heat: roast 1u of nuts, cook 1u of tofu
- 15u of surface_heat: process a chunk of fat in a recipe or cook a unit of blood
- 20u of surface_heat: cook a chunk of meat (300 grams)

The bread recipe for example uses 20 units of flour, so it should be roughly 20 units of surface_heat. However it is special-cased because you must also use warm water to activate the yeast used in the recipe so it ends up being 22 units. These values serve as a rough guideline to give a decent estimate of what the total recipe should cost. 

Also, because many foods are not cooked to boiling temperature, the recipe should always require clean water and not any water. For recipes that use water_boiling_heat, it is OK to use regular water.

The easiest way to estimate the power needed to cook something is to see how long it takes to microwave. 1000w is a good ballpark estimate for a microwave power usage which gives an easy solution of 3u of this per minute in the microwave.


"water_boiling_heat" uses a base cost of 100kJ. This is the hypothetical maximum efficiency to boil 0.25 liters of water for about a minute assuming the water was previously at basement temperature (~45 F). These costs are based on that figure, and the total efficiencies are slightly different then that of surface_heat. It's still about 80% for induction (125u battery, 1:1 kJ:battery), and for regular electric 50% (200u battery, 1:1 kJ:battery), for a microwave, 40% (250u battery, 1:1 kJ:battery), 25% for most combustible fuels - gasoline (12u fuel 34:1 kJ:fuel), kerosene (12u fuel 34:1 kJ:fuel), propane (16u fuel 25:1 kJ:fuel), acetlyne (8u fuel 50:1 kJ:fuel), ethanol (20u fuel, 25:1 kJ:fuel), 20% for hexamine (13u fuel 40:1 kJ:fuel), 10% for charcoal (30u fuel 32:1 kJ:fuel)

To apply "water_boiling_heat" apply 1u of water_boiling_heat for each 0.25 liter of water being boiled, then additionally add an extra water_boiling_heat for each 15 minutes it boils on top of the heat to bring it up to boil. The clean water recipe is 1 unit of this. To boil an one liter of water for an hour you would expect about 20u total (4x0.25L + 4 extra charges per each 0.25L). Add one more water_boiling_heat for each 0.25L of non-water foodstuff added to the recipe regardless of how long the boil time will be - to boil 0.5 liters of eggs for an hour using the above 4 hours of water it would be 22u water_boiling_heat. (In practice it doesn't actually take that long to boil an egg and it doesn't take that much water so it will be much lower, this is just an example!)

Canning recipes use "water_boiling_heat" in multiples. It is split into water bath and pressure canning methods - water bath uses 10u of water_boiling_heat for a 0.5 liter jar or 60u for a 3 liter jar. Pressure canning is 50% higher and requires appropriate equipment.


"dehydrating_heat" is measured in 10 grams of material each, and takes 66 kJ of battery or 12u of coal/charcoal. Smoking food is similar to this, but uses half as much charcoal (about 4:5 charcoal:food by weight) 



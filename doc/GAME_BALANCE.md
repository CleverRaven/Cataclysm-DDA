# Stat system scaling:
Minimum stat: 0 (should only happen due to penalties, instant failure in most scenarios)

Nominal stat: 8 ("average" person)

Very high stat: 14 (realistic world class human, maximum cost-effective in chargen)

Maximal stat: 20 (higher may be achievable, but we're not worried about balancing at that point.)


# Skill system scaling:
Minimum skill: 0 (no training)

Maximum skill: 10 (requires regular training to maintain, "professional" level)


# Monster melee skill scaling:
Minimum skill: 0 (no melee potential; turret, fungal wall)

Nominal skill: 4 (average critter; most zeds & giant insects)

Notable skill: 6 (competent/carnivore; bear, wolf, police/survivor zeds)

Very high skill: 8 (dangerous opponent; dark wyrm, vinebeast)

Maximal skill: 10 (highest for balance purposes; jabberwock, tribot, shoggoth, gracken)


# Speeds:
Zombies are a bit faster than "shambling". Zombified versions of fast critters will remain fast, but in general the process slows the undead version. Further, under no circumstances should a zed be more than 50% faster than base character speed. Currently, this means "capped at 150".


# Dodge System assumptions:
Dodge chance is based on attacker's melee skill and target's dex stat and dodge skill.

Successful dodges negate the attack and impose a cumulative penalty on dodges within the same turn.

## Dodge Use Cases:
An individual with no skill and nominal stats in ideal circumstances against a basic opponent should occasionally be able to dodge.

An individual with no skill and nominal stats in ideal circumstances against a skilled opponent should rarely if ever be able to dodge.

An individual with world-class dodging ability, in ideal circumstances against a basic opponent should have a negligible chance of failure.

An individual with world-class dodging ability, in ideal circumstances against a skilled opponent should have a moderate chance of failure.

The effect of increasing dodge skill has a growth rate with diminishing returns that accelerates sharply at the point where you move beyond the dodge a "regular" character is likely to achieve (7  and above)

The balance of melee versus dodge should favour dodge which, after all, isn't effective against a wide variety of other types of attacks.

Even a world class dodger should not be able to dodge continuously when attacked many times a turn.


# MELEE WEAPONS:
## To-Hit Bonuses
To-hit bonuses start at '-2' and are modified as follows for weapons that have the following properties:

### Grip
Grip is a measure of how well you can control the weapon to quickly respond to situational changes.

-1 - Particularly hard to grip items, (especially those that are innately slipper or very rounded with no obvious gripping edge) such as basketballs and barrels, or which are dangerous to hold because of very sharp edges, like scrap metal and broken glass.

+0 - Any object that doesn't fall into one of the categories below. Examples include 2x4s, computer monitors, wires, stingers and clothing. Basically, anything that has a grippable component, but which is too thick, too thin, or too flimsy to grab comfortably in a way that can reliably control the object.

+1 - A weapon with a fairly solid grip, like a pipe, a rock, guitar neck, pool cue or a heavy stick.

+2 - A weapon with a dedicated grip shaped to the hand, like a sword, axe, knife, or police baton, or that is strapped to the body (or is a piece of the body). Fists would get a +2 bonus here, bringing them to "0" total, since none of the others would apply.

### Length
Length allows more surface area for potential contact, and reduces the need to control the positioning of the body to guarantee a hit. It also allows the player to strike from a safer distance, allowing them to worry more about trying to hit without being hit in return, and allows for swings with larger arcs, making dodging such a strike more difficult.

+0 - Any object without a length bonus.

+1 - Objects that, when held, extend over a foot (1/3 of a meter) in length from the hand. A normal american 12inch ruler is the handy boundary guide for when an item should switch over to a +1 bonus (the ruler, losing several inches when held, does not get one - unless you added a handle to it!).

+2 - An object that is over 3 feet in length from the point where it is held. Includes swords, spears, quarterstaffs, poles, and a lot of other stuff.

### Striking Surface
Some weapons need to strike in a certain way to be effective. Others are more difficult to use "incorrectly".

-2 - Single-Point weapons - Picks, spears, syringes. Any weapon that has a single point that must contact the enemy in a specific way in order to deal a decent amount of damage. Also, weapons with difficult attack angles, like scythes, where the damaging part of the weapon is faced away from the enemy.

-1 - Line of damage weapons - Swords, knives, and other weapons that require a solid strike along a particular piece of the weapon, where the weapon can be said to have an attack angle, fall here. Weapons that have point attacks but are still effective without any solid hit, such as a nailboard, would also fall here.

+0 - attack-anywhere weapons - Clubs, pipes, maces, etc, where the weapon will be dealing full damage with a solid blow no matter how it is angled, because every surface is a striking surface.

+1 - Weapons that can still do significant damage even with glancing blows would fall here. Jagged tearing weapons and electric weapons like a stun baton would fall here.

### Balance
A measure of how well-suited the item is for being swung/thrust/etc. This factors in overall balance of the weapon, weight is accounted for separately.

-2 - Very clumsy or lopsided items ill-suited for swinging or thrusting.  Characterized by requiring effort just to hold steady.  frying pan or pot, chainsaw, chair, vacuum cleaner.

-1 - Balance of the object is uneven, but in a way that at least doesn't interfere with swinging. axes, sledgehammer, rifle, scythe, most polearms.

+0 - Neutral balance, neither well nor poorly weighted for the typical use. Heavy stick, rock, pool stick, kitchen knives, claw hammer, metal pipe, crowbar, handguns.

+1 - Well-balanced for swinging or stabbing.  Baseball bat, golf club, swords, quarterstaff, knives.

##Damage
Weapon's relative strength is based on an approximate formula involving its damage, to-hit, techniques and few other factors.
This strength will be expressed in "points" or dpt (damage per turn) as damage per turn is the most significant balancing factor.

### Damage per turn
Melee weapon's relative strength is measured by damage per turn (more precisely: damage per 100 moves). The damage is sum of all damage types (average of min/max where random) from the weapon alone, without including strength in the calculation.
For bashing weapons, it is assumed the character can achieve the maximum damage, but this strength isn't added to damage (or critical damage).

### To-hit
While not a direct measure of damage, to-hit bonus has to be included in the calculations, as it has a significant effect on actual damage output (through hit and crit rate).

Each point of to-hit is "worth" roughly a 10% increase/decrease in damage per turn or 2.5 points of damage per turn (whichever is higher).

### Techniques
Rapid strike technique increases damage per turn by 0.66 / 0.5 = 132%
Other techniques are generally too situational to be worth any points here.

### Weapon tiers
Damage per turn should put the weapon into one of those categories:

<10 - Not weapons. Those items may be pressed into service, but are unlikely to be better than fists. Plastic bottles, rocks, boots.

11-15 - Tools not meant to strike and improvised weapons. Two-by-fours, pointy sticks, pipes, hammers.

16-25 - Dangerous tools or crude dedicated weapons. Golf clubs, two-by-swords, wooden spears, knife spears, hatchets, switchblades, tonfas, quarterstaves.

26-35 - Good dedicated weapons or the most dangerous of tools. Wood and fire axes, steel spears, electric carvers, kukris, bokken, machetes, barbed wire bats.

36-45 - Weapons of war, well designed to kill humans. Wakizashis, katanas, broadswords, zweihanders, combat knifes, battle axes, war hammers, maces, morningstars.

46+ - Sci-fi stuff. Diamond katanas, monomolecular blades, lightsabers and chainswords.

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
## Magazines
### Reload times
The overall balance is that magazines themselves are slow to reload whereas changing a magazine should be fast. For standard box magazines a default `reload_time` of 100 (per round) is appropriate with this value increasing for poor quality or extended magazines. Guns themselves should also specify `reload` of 100 (per magazine) unless their magazines are particularly awkward to reload (eg. ammo belts). The game logic intrinsically handles higher volume magazines consuming more time to attach to a gun so you need not consider this.

### Weight
Increases proportional to capacity and should have a comparable ratio to similar magazines. Consider the base item to be a 10-round .223 factory specification box magazine which has a capacity:weight of 1:10. Increase the ratio markedly for poor quality magazines or more slightly for extended magazines. Smaller calibers should use a lower ratio. The `material` should have some effect with plastic magazines weighing less. Nothing should have a lower ratio than the LeadWorks magazines.

### Volume
Scaled based upon the capacity relative to the `stack_size` of the ammo. For example 223 has a `stack size` of 20 so for 10 and 30 round magazines the volume would be 1 and 2. Extended magazine should always have larger volume than the standard type and for very large drum magazines consider applying an extra penalty. By default most handgun magazines should be volume 1 and most rifle magazines volume 2. Ammo belts should not specify volume as this will be determined from their length.

### Reliability
Should be specified first considering the below and then scaled against any equivalent magazines. For example if an extended version of a magazine exists place it one rank below the standard capacity version. Damaged guns or magazines will further adversely affect reliability.

10 - **Perfectly reliable**. Factory specification or milspec only. Never extended magazines. Very rare.

9 - **Reliable**. Failures only in burst fire. Factory or milspec magazines only. Never extended magazines. Uncommon.

8 - **Dependable**. Failures infrequently in any fire mode. Highest reliability possible for extended magazines and those crafted using gunsmithing tools. Most common.

7 - **Serviceable**. Fail infrequently in semi-automatic, more frequently in burst. Includes many extended and aftermarket gunsmithing tools. Common.

6 - **Acceptable**. Failures can be problematic. Highest reliability possible for magazines crafted **without** gunsmithing tools. Includes most ammo belts.

5 - **Usable**. Failures can be problematic and more serious. Mostly poor quality hand-crafted magazines.

<4 - **Poor**. Significant risk of catastrophic failure. Not applied by default to any item but can be acquired by damage or other factors.

### Rarity
Overall balance is that pistol magazines are twice as common as rifle magazines and that for guns that spawn with magazines these are always the standard capacity versions. Consider 9mm and .223 to be the defaults with everything else more rare. Some locations have more specific balance requirements:

Location          | Description                                               | With guns | Damaged   | Example
------------------|-----------------------------------------------------------|-----------|-----------|--------------------------
Military site     | Only source of milspec magazines and ammo belts           | Never     | Never     | LW-56, .223 ammo belt
Gun store         | Standard and extended capacity magazines                  | Never     | Never     | STANAG-30, Glock extended
Police armory     | Mostly pistol magazines, especially 9mm, never extended   | Sometimes | Never     | Glock, MP5 magazine
SWAT truck        | Police or military magazines, occasionally extended       | Sometimes | Rarely    | MP5 extended
Survivor basement | Anything except milspec weighted towards common types     | Often     | Sometimes | Saiga mag, M1911 extended
Military surplus  | Older military magazines that are not current issue       | Never     | Rarely    | M9 mag, STEN magazine
Pawn showp        | Anything except milspec weighted towards unusual calibers | Never     | Rarely    | Makarov mag, AK-74 mag
Everywhere else   | Predominately 9mm and 223. Always with standard magazine  | Often     | Sometimes | Ruger 223 mag, M1911 mag

## Ammo stats
The damage of ammunition is the square root of a round's muzzle energy in joules rounded to the nearest integer with an arbitrary increase or decrease to account for terminal ballistics. A similar system for calculating recoil is planned but not currently being worked on. The figures used to calculate stats and any other relevant information are presented in table below. Damage of handloaded ammo is set to 92% (rounded down) of their factory counterparts.

Ammo ID            | Description                 | Muzzle Energy, J | Damage     | Applied Modifiers / Comments |
-------------------|-----------------------------|------------------|------------|---------|
.22LR              | 40gr unjacketed bullet      | 141              | √141 = 12  |         |
.22LR FMJ          | 30gr FMJ bullet             | 277              | √277 = 17  |         |
9x19mm FMJ         | 115gr FMJ bullet            | 420              | √420 = 24  |         |
9x19mm +P          | 115gr JHP bullet            | 632              | √632 = 25  |         |
9x19mm +P+         | 115gr JHP bullet            | 678              | √678 = 26  |         |
9x19mm JHP         | 116gr JHP bullet            | 533              | √533 = 23  | damage increased by 3 |
9x18mm 57-N-181S   | 93gr FMJ bullet             | 251              | √251 = 16  |         |
9x18mm SP-7        | 93gr bullet                 | 417              | √417 = 20  |         |
9x18mm RG028       | 93gr hardened steel core bullet | 317          | √317 = 18  | damage reduced by 4 |
.38 Special        | 130gr FMJ bullet            | 256              | √256 = 16  |         |
.38 Super          | 90gr JHP bullet             | 658              | √658 = 26  |         |
.38 FMJ            | 130gr FMJ bullet            | 256              | √256 = 16  |         |
10mm auto          | 180gr FMJ bullet            | 575              | √575 = 24  |         |
.40 S&W            | 135gr JHP bullet            | 575              | √575 = 24  |         |
.40 FMJ            | 180gr FMJ bullet            | 598              | √598 = 24  |         |
.44 magnum         | 240gr JHP bullet            | 1570             | √1570 = 40 |         |
.45 ACP JHP        | 185gr JHP bullet            | 614              | √614 = 25  |         |
.45 ACP FMJ        | 230gr FMJ bullet            | 447              | √447 = 21  |         |
.45 ACP +P         | 200gr JHP bullet            | 702              | √702 = 26  |         |
.454 Casull        | 300gr JSP bullet            | 2459             | √2459 = 50 |         |
.500 S&W Magnum    | 500gr bullet                | 3056             | √3056 = 55 |         |
5.7x28mm SS190     | 31gr AP FMJ bullet          | 534              | √534 = 23  | damage reduced by 3   |
4.6x30mm           | 31gr copper plated steel bullet | 505          | √505 = 22  | damage reduced by 4   |
7.62x39mm M43      | 123gr steel core FMJ bullet | 1607             | √1607 = 47 | damage lowered by 15  |
7.62x39mm M67      | 123gr steel core FMJ bullet | 1607             | √1607 = 47 | damage lowered by 9   |
5.45x39mm 7N10     | 56gr FMJ bullet             | 1402             | √1402 = 37 | damage increased by 3 |
5.45x39mm 7N22     | 57gr steel core FMJ bullet  | 1461             | √1461 = 38 |         |
.223 Remington     | 36gr JHP bullet             | 1524             | √1524 = 39 | damage increased by 5 |
.308 Winchester    | 168gr hollow point bullet   | 3570             | √3570 = 60 |         |
7.62 NATO M80      | 147gr FMJ bullet            | 3304             | √3304 = 57 |         |
7.62 NATO M62      | 147gr FMJ bullet            | 3304             | √3304 = 57 | Belt with 1/5 tracer rounds |
.270 Winchester    | 130gr soft point bullet     | 3663             | √3663 = 61 |         |
.30-06 Springfield | 165gr soft point bullet     | 3894             | √3894 = 62 | damage increased by 4 |
.30-06 M2          | 165gr FMJ bullet            | 3894             | √3894 = 62 | damage reduced by 10 |
.30-06 M14A1       | Incendiary ammunition       | 3894             | √3894 = 62 | damage reduced by 10 |
.300 Winchester Magnum | 220gr JHP bullet        | 5299             | √5299 = 73 | damage increased by 5 |
7.62x54mmR         | 150gr FMJ bullet            | 2677             | √2677 = 52 |         |

# LIQUIDS:
Multi-charge items are weighed by the charge/use.  If you have an item that contains 40 uses, it'll weigh 40x as much (when found in-game) as you entered in the JSON. Liquids are priced by the 250mL unit, but handled in containers.  This can cause problems if you create something that comes in (say) a gallon jug (15 charges) and price it at the cost of a jug's worth: it'll be 15x as expensive as intended.

To that end, here's a list of containers with non-one volume.  If you have something spawn in 'em, divide the "shelf" price by this value to arrive at the correct price to list in the JSON.

- plastic bottle: 2

- glass jar: 2

- glass bottle: 3

- plastic canteen: 6

- 3L glass jar: 12, as expected

- gallon jug: 15
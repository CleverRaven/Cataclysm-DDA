# Gun Naming and Inclusion Guidelines

Cataclysm: Dark Days Ahead includes much of the complexity embodied by guns in
the real world. This means that, as of writing, there are approximately 80
different types of ammo in the game and an astonishing 330 or so different guns
using these ammo types.	Many people (the author included) appreciate this large
variety and what it brings to the game. However, as this variety is composed of
almost entirely of real guns, it suffers from two related problems.

The first problem is that if you are not already familiar with (and have
memorized) their names, gun names aren't meaningful in the form they're added
to the game. What is a `Manhurin MR 73`? What is a `SIG P226`? What is an
`FS2000`? When this, and a string of numbers representing their ammotype
is all you have, you can't easily tell that those are a revolver, a pistol, and
a rifle. Even if the ammotype may give some clues, that's another piece of
information that must be memorized and isn't obvious at a glance. Then, once
you obtain a gun, the mapping between that gun and what magazines it takes is
not always clear. If I have a Luty submachine gun, how do I know that it takes
magazines with STEN in their name? I can read the description, the game will
helpfully highlight them if I have the gun on my person, but the mapping is not
intuitive. 

If you are interested in learning about guns and enjoy doing that, the learning
that is required to understand guns is an interesting experience. If you're
just here to try your hand at surviving the cataclysm, and don't care to learn
much about guns, it's just an obstacle to understanding how to use guns.
Because of the significant tactical role of guns in the game, this obstacle
matters in how approachable the game is. If guns are hard to understand, it's
hard to use them, which makes the game artificially harder, and not in an fun
or engaging way.

The second problem is problem of mapping the extreme variety of real world guns
to the guns in game. In the real world, many guns can be quite similar. While
they are welcomed in game, having them all of them as separate items creates
some problems. Each item has its own stat block, and this invites all sorts of
differences that aren't meaningfully different. Having non-meaningful
differences like this only exacerbates the first problem.

These problems have resulted in two separate solutions:

## Generic Guns

Generic Guns is a mod that entirely strips out the gun system used in the base
game, and replaces it with a much simplified system. It does this by replacing
all of the ammunition types for firearms with 12 different archetypes. For each
ammunition type, it then adds appropriate guns for each type.

This is fairly extreme, but by largely ignoring the real complexity of guns, is
a very accessible experience for people who are not familiar with or interested
in guns. And by completely replacing all the guns in the game, the second issue
is no longer relevant.

However, it also has fairly drastic balance changes. Collapsing 80 ammo types
and 330 guns into 12 and 48 significantly changes how available and compatible
guns and ammo are.

## Gun Brand Names (`gun` type variants)

Because the significant balance implications of Generic Guns, another way to
make firearms more accessible was created. Through variants, one item can be
shown as several items, with unique cosmetic aspects.

By converting the very similar guns to multiple variants of a single item, the
second problem is resolved. By allowing variants for guns to be toggled on and
off, different names can be given to guns for those who want all the complexity
and inaccessibility of real life, and descriptive names can be given as an
alternative, solving the first problem.

# Inclusion Criteria

There are two criteria for adding a new gun to the game. The first is a "hard"
threshold of rarity that your gun must pass. The second is a script-enforced
"difference" threshold. The second is softer, as guns that fail it should be
added as variants instead of distinct items.

## Rarity Threshold

Pre-cataclysm firearms from the same dimension in which the game takes place in
can be broadly divided into 5 categories: civilian, civilian
NFA(non-machinegun), civilian machineguns, police, and military

### Civilian Firearms

Non-machinegun civilian firearms: these are your normal shotguns, handguns,
rifles, pcc's, etc.

Per family of firearm, if there have been historically 100 listings or more
combined on a secondary market's website (e.g. GunBroker, GunsAmerica, Rock
Island Auctionhouse, etc) then the gun is common enough for inclusion into
C:DDA. A family of firearms would include things like, all S&W K-frame models,
all types of Mosin-Nagant longarms, the entire MDR rifle family, etc. This is
distinct from "100 models produced, ever", as it is instead a measure of how
common the guns are on the broader market.

Civilian NFA firearms: Short Barreled Rifles, Short Barreled Shotguns, Any
Other Weapons, or Silencers.

- Silencers: Should be genercized into types of silencers. Shotgun silencer, rimfire silencer, pistol silencer, hybrid rifle/pistol silencer, rifle silencer, rifle (flow through) silencer, big bore silencer. Specific models should be avoided.

- SBRs, SBSs: If they're major models like AK's, AR's, 870's, or 500's, maybe, but they need to be very uncommon. Anything else, too rare to include. At a ratio of 532000 SBR's (2021, ATF) : 393347000 (total guns in the US, est), an SBR/SBS should be 0.00135 times as common as its longarm counterpart, or otherwise just be described as its non-NFA pistol braced variant.

- AOW's(pen guns, cane guns, not technically-an-AOW's like a pistol with a VFG): no.

Civilian Machineguns: If it's not a MAC 10, MAC 11, Full size UZI, STEN gun, or
a Reising, then no. No post samples. All civilian machineguns have a collective
rarity of 175,977 machineguns: 393,347,00 total guns in the US.

### Military and Police Firearms

These are harder to find stats on, but guns are not allowed simply by merit of
being used by the military or police. Whether or not a gun will be allowed will
be determined in large part by mergers.

## Difference Threshold

Guns are eligible to be combined into variants based on the criteria in the
[verification script](../tools/json_tools/gun_variant_validator.py). Check that
for the most up-to-date info on these criteria.

As of writing, the checks are:

 - The guns take the same ammo types.
 - The guns have the same fire modes.
 - The volumes of the guns are within 500 mL
 - The weight of the guns are within 500 g
 - If both guns have a longest side defined, and they are within 10 cm
 - If both guns have dispersion defined and they are within 50 (0.5 moa)
 - The guns take the same speedloaders
 - The guns take the same magazines
 - The guns have reload speed modifiers that are within 250 moves
 - the barrel lengths of the guns are within 2 cm
 - the ranged damage of the guns are within 4 pts

If all of these are true, the guns should simply be variants of one gun.

# Gun Naming

There are two rules for the default names of guns. That is, the names of guns
when the 'Gun Brand Names' option is not enabled. These rules are enforced by
script.

## Rule 1: What is this?

This rule requires that the name of the gun clarifies what it is. This is meant
to prevent gun names that are obscure strings of letters and numbers.

As of writing, names are considered to be acceptable if the gun name contains
any of these strings. As with the other rules, the most up-to-date info on what
is acceptable is contained in the
[verification script](../tools/json_tools/gun_variant_validator.py).

|        String       |Reason|
|---------------------|------|
|automagnum           |describes a class of gun|
|blunderbuss          |describes a class of gun|
|carbine              |describes a class of gun|
|coilgun              |describes a gun|
|combination gun      |describes a (bizarre) gun|
|flamethrower         |describes a class of gun|
|flintlock            |describes a class of gun|
|FSP                  |Hub-01 special faction weapon. Gets a bit of a pass|
|gatling gun          |describes a class of gun|
|hand cannon          |describes a class of gun|
|handgun              |describes a class of gun|
|HWP                  |Hub-01 special faction weapon. Gets a bit of a pass|
|launcher             |describes a class of gun|
|lever gun            |describes a class of gun|
|LMG                  |short name for light machine gun|
|HMG                  |short name for heavy machine gun|
|SMG                  |short name for sub machine gun|
|machine gun          |describes a class of gun|
|operational briefcase|Used for the bizarre H&K concealed gun|
|pistol               |describes a class of gun|
|railgun              |describes a class of gun|
|revolver             |describes a class of gun|
|rifle                |describes a class of gun|
|shotgun              |describes a class of gun|
|six-shooter          |describes a class of gun|
|submachine gun       |describes a class of gun|
|trenchgun            |describes a class of gun|

## Rule 2: What goes with this?

This rule requires that there is a common "identifier" string shared between
the gun and all the magazines or speedloaders it takes.

As the STANAG magazines are very generic, they are not considered in evaluation
of whether or not a gun has this "identifier".

As of writing, the following strings are not considered when checking.
As with the other rules, the most up-to-date info on what is acceptable is
contained in the [verification script](../tools/json_tools/gun_variant_validator.py).

|   String  |Reason|
|-----------|------|
|10mm       |ammo type name|
|.22        |ammo type name|
|.338       |ammo type name|
|.38        |ammo type name|
|.40        |ammo type name|
|.44        |ammo type name|
|.45        |ammo type name|
|.500       |ammo type name|
|5x50mm     |ammo type name|
|5.7mm      |ammo type name|
|7.7mm      |ammo type name|
|8x40mm     |ammo type name|
|9x19mm     |ammo type name|
|-round     |common term in magazine names|
|magazine   |common term in magazine names|
|stripper   |common term in magazine names|
|speedloader|common term in magazine names|
|11         |common false-positive term|
|to         |common false-positive term|
|if         |common false-positive term|
|ip         |common false-positive term|
|rifle      |common false-positive term|
|carbine    |common false-positive term|


Here are some selected identifiers that are used by existing guns

| Identifier |Commentary|
|------------|----------|
|Exodii      |Describes the faction associated with all the items that go with this gun|
|P230        |This is the name of a handgun. This is a valid common identifier, as long as the handgun follows the other rules|
|varmint     |This is a descriptor of the role of the gun. This is a valid identifier|
|Glock       |This is a brand. This is a valid common identifier, matching the magazines with this ammo type to the gun|
|Saiga-410   |This is the name of a shotgun. This is a valid common identifier|

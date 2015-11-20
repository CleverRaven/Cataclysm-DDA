# JSON Flags

## Notes
- Many of the flags intended for one category or item type, can be used in other categories or item types. Experiment to see where else flags can be used.
- Offensive and defensive flags can be used on any item type that can be wielded.

## Material Phases

- ```NULL```
- ```SOLID```
- ```LIQUID```
- ```GAS```
- ```PLASMA```

## Recipes

### Categories

- ```CC_WEAPON```
- ```CC_AMMO```
- ```CC_FOOD```
- ```CC_DRINK```
- ```CC_CHEM```
- ```CC_ELECTRONIC```
- ```CC_ARMOR```
- ```CC_MISC```

## Furniture & Terrain
List of known flags, used in both terrain.json and furniture.json

### Flags

- ```TRANSPARENT``` Players and monsters can see through/past it. Also sets ter_t.transparent.
- ```FLAT``` Player can build and move furniture on.
- ```BASHABLE``` Players + Monsters can bash this.
- ```CONTAINER``` Items on this square are hidden until looted by the player.
- ```PLACE_ITEM``` Valid terrain for place_item() to put items on.
- ```DOOR``` Can be opened (used for NPC path-finding).
- ```FLAMMABLE``` Can be lit on fire.
- ```FLAMMABLE_HARD``` Harder to light on fire, but still possible.
- ```EXPLODES``` Explodes when on fire.
- ```DIGGABLE``` Digging monsters, seeding monster, digging with shovel, etc.
- ```LIQUID``` Blocks movement, but isn't a wall (lava, water, etc.)
- ```SWIMMABLE``` Player and monsters can swim through it.
- ```SHARP``` May do minor damage to players/monsters passing through it.
- ```PAINFUL``` May cause a small amount of pain.
- ```ROUGH``` May hurt the player's feet.
- ```SEALED``` Can't use `e` to retrieve items; must smash them open first.
- ```NOITEM``` Items 'fall off' this space.
- ```DESTROY_ITEM``` Items that land here are destroyed.
- ```GOES_DOWN``` Can use `>` to go down a level.
- ```GOES_UP``` Can use '<' to go up a level.
- ```CONSOLE``` Used as a computer.
- ```ALARMED``` Sets off an alarm if smashed.
- ```SUPPORTS_ROOF``` Used as a boundary for roof construction.
- ```MINEABLE``` Can be mined with a pickaxe/jackhammer
- ```INDOORS``` Has a roof over it; blocks rain, sunlight, etc.
- ```THIN_OBSTACLE``` Passable by players and monsters; vehicles destroy it.
- ```COLLAPSES``` Has a roof that can collapse.
- ```FLAMMABLE_ASH``` Burns to ash rather than rubble.
- ```DECONSTRUCT``` Can be deconstructed.
- ```REDUCE_SCENT``` Reduces scent even more; only works if also bashable.
- ```FIRE_CONTAINER``` Stops fire from spreading (brazier, wood stove, etc.)
- ```SUPPRESS_SMOKE``` Prevents smoke from fires; used by ventilated wood stoves, etc.
- ```PLANT``` A 'furniture' that grows and fruits.
- ```OPENCLOSE_INSIDE``` If it's a door (with an 'open' or 'close' field), it can only be opened or closed if you're inside.
- ```BARRICADABLE_WINDOW``` Window that can be barricaded.
- ```BARRICADABLE_DOOR``` Door that can be barricaded.
- ```SHORT``` Feature too short to collide with vehicle protrusions. (mirrors, blades)
- ```TINY``` Feature too short to collide with vehicle undercarriage. Vehicles drive over them with no damage, unless a wheel hits them.
- ```NOCOLLIDE``` Feature that simply doesn't collide with vehicles at all.
- ```PERMEABLE``` Permeable for gases.
- ```MOUNTABLE``` Suitable for guns with the "MOUNTED_GUN" flag.
- ```UNSTABLE``` Walking here cause the bouldering effect on the character.
- ```HARVESTED``` Marks the harvested version of a terrain type (e.g. harvesting an apple tree turns it into a harvested tree, which later becomes an apple tree again).
- ```ROAD``` Flat and hard enough to drive or skate (with rollerblades) on.
- ```AUTO_WALL_SYMBOL``` (only for terrain) The symbol of this terrain will be one of the line drawings (corner, T-intersection, straight line etc.) depending on the adjacent terrains.
- ```ALLOW_FIELD_EFFECT``` Apply field effects to items inside ```SEALED``` terrain/furniture
- ```CHIP``` Used in construction menu to determine if wall can have paint chipped off

Example: `-` and `|` is terrain with the `CONNECT_TO_WALL` flag, `O` does not have it, `X` and `Y` have the `AUTO_WALL_SYMBOL` flag, `X` terrain will be drawn as a T-intersection (connected to west, south and east), `Y` will be drawn as horizontal line (going from west to east, no connection to south).
```
-X-    -Y-
 |      O
```

### Examine actions

- ```none``` None
- ```gaspump``` Use the gas-pump.
- ```toilet``` Either drink or get water out of the toilet.
- ```elevator``` Use the elevator to change floors.
- ```controls_gate``` Controls the attached gate.
- ```cardreader``` Use the cardreader with a valid card, or attempt to hack.
- ```rubble``` Clear up the rubble if you have a shovel.
- ```chainfence``` Hop over the chain fence.
- ```bars``` Take advantage of AMORPHOUS and slip through the bars.
- ```tent``` Take down the tent.
- ```shelter``` Take down the shelter.
- ```wreckage``` Clear up the wreckage if you have a shovel.
- ```pit``` Cover the pit if you have some planks of wood.
- ```pit_covered``` Uncover the pit.
- ```fence_post``` Build a fence.
- ```remove_fence_rope``` Remove the rope from the fence.
- ```remove_fence_wire``` Remove the wire from the fence.
- ```remove_fence_barbed``` Remove the barbed wire from the fence.
- ```slot_machine``` Gamble.
- ```safe``` Attempt to crack the safe.
- ```bulletin_board``` Create a home camp; currently not implemented.
- ```fault``` Displays descriptive message, but otherwise unused.
- ```pedestal_wyrm``` Spawn wyrms.
- ```pedestal_temple``` Opens the temple if you have a petrified eye.
- ```fswitch``` Flip the switch and the rocks will shift.
- ```flower_poppy``` Pick the mutated poppy.
- ```fungus``` Release spores as the terrain crumbles away.
- ```dirtmound``` Plant seeds and plants.
- ```aggie_plant``` Harvest plants.
- ```harvest_tree_shrub``` Harvest a fruit tree or shrub.
- ```shrub_marloss``` Pick a marloss bush.
- ```shrub_wildveggies``` Pick a wild veggies shrub.
- ```recycler``` Recycle metal objects.
- ```trap``` Interact with a trap.
- ```water_source``` Drink or get water from a water source.
- ```acid_source``` Get acid from an acid source.

### Currently only used for Fungal conversions

- ```WALL``` This terrain is an upright obstacle.
- ```ORGANIC``` This furniture is partly organic.
- ```FLOWER``` This furniture is a flower.
- ```SHRUB``` This terrain is a shrub.
- ```TREE``` This terrain is a tree.
- ```YOUNG``` This terrain is a young tree.
- ```FUNGUS``` Fungal covered.

### Furniture only

- ```BLOCKSDOOR``` This will boost map terrain's resistance to bashing if `str_*_blocked` is set (see `map_bash_info`)

## Monsters
Flags used to describe monsters and define their properties and abilities.

### Sizes

- ```TINY``` Squirrel
- ```SMALL``` Dog
- ```MEDIUM``` Human
- ```LARGE``` Cow
- ```HUGE``` Tank

### Categories

- ```NULL``` No category.
- ```CLASSIC``` Only monsters we expect in a classic zombie movie.
- ```WILDLIFE``` Natural animals.

### Death functions. Multiple death functions can be used. Not all combinations make sense.

- ```NORMAL``` Drop a body, leave gibs.
- ```ACID``` Acid instead of a body. not the same as the ACID_BLOOD flag. In most cases you want both.
- ```BOOMER``` Explodes in vomit.
- ```KILL_VINES``` Kill all nearby vines.
- ```VINE_CUT``` Kill adjacent vine if it's cut.
- ```TRIFFID_HEART``` Destroys all roots.
- ```FUNGUS``` Explodes in spores.
- ```DISINTEGRATE``` Falls apart.
- ```WORM``` Spawns 2 half-worms
- ```DISAPPEAR``` Hallucination disappears.
- ```GUILT``` Moral penalty. There is also a flag with a similar effect.
- ```BLOBSPLIT``` Creates more blobs.
- ```MELT``` Normal death, but melts.
- ```AMIGARA``` Removes hypnosis if the last one.
- ```THING``` Turn into a full thing.
- ```EXPLODE``` Damaging explosion.
- ```BROKEN``` Spawns a broken robot item, its id calculated like this: the prefix "mon_" is removed from the monster id, than the prefix "broken_" is added. Example: mon_eyebot -> broken_eyebot
- ```RATKING``` Cure verminitis.
- ```KILL_BREATHERS``` All breathers die.
- ```SMOKEBURST``` Explode like a huge smoke bomb.
- ```GAMEOVER``` Game over man! Game over! Defense mode.

### Flags

- ```NULL``` Source use only.
- ```SEES``` It can see you (and will run/follow).
- ```HEARS``` It can hear you.
- ```GOODHEARING``` Pursues sounds more than most monsters.
- ```SMELLS``` It can smell you.
- ```KEENNOSE``` Keen sense of smell.
- ```STUMBLES``` Stumbles in its movement.
- ```WARM``` Warm blooded.
- ```NOHEAD``` Headshots not allowed!
- ```HARDTOSHOOT``` Some shots are actually misses.
- ```LEAKSGAS``` Leaks toxic gas.
- ```GRABS``` Its attacks may grab you!
- ```BASHES``` Bashes down doors.
- ```GROUP_BASH``` Gets help from monsters around it when bashing.
- ```DESTROYS``` Bashes down walls and more. (2.5x bash multiplier, where base is the critter's max melee bashing)
- ```BORES``` Tunnels through just about anything (15x bash multiplier: dark wyrms' bash skill 12->180)
- ```POISON``` Poisonous to eat.
- ```VENOM``` Attack may poison the player.
- ```BADVENOM``` Attack may **severely** poison the player.
- ```PARALYZE``` Attack may paralyze the player with venom.
- ```BLEED``` Causes the player to bleed.
- ```WEBWALK``` Doesn't destroy webs.
- ```DIGS``` Digs through the ground.
- ```CAN_DIG``` Can dig _and_ walk.
- ```FLIES``` Can fly (over water, etc.)
- ```AQUATIC``` Confined to water.
- ```SWIMS``` Treats water as 50 movement point terrain.
- ```ATTACKMON``` Attacks other monsters.
- ```ANIMAL``` Is an _animal_ for purposes of the `Animal Empathy` trait.
- ```PLASTIC``` Absorbs physical damage to a great degree.
- ```SUNDEATH``` Dies in full sunlight.
- ```ELECTRIC``` Shocks unarmed attackers.
- ```ACIDPROOF``` Immune to acid.
- ```ACIDTRAIL``` Leaves a trail of acid.
- ```SLUDGEPROOF``` Ignores the effect of sludge trails.
- ```SLUDGETRAIL``` Causes the monster to leave a sludge trap trail when moving.
- ```FIREY``` Burns stuff and is immune to fire.
- ```QUEEN``` When it dies, local populations start to die off too.
- ```ELECTRONIC``` e.g. A Robot; affected by emp blasts and other stuff.
- ```FUR``` May produce fur when butchered.
- ```LEATHER``` May produce leather when butchered.
- ```FEATHER``` May produce feathers when butchered.
- ```FAT``` May produce fat when butchered.
- ```CBM_CIV``` May produce a common cbm or two when butchered.
- ```BONES``` May produce bones and sinews when butchered.
- ```IMMOBILE``` Doesn't move (e.g. turrets)
- ```FRIENDLY_SPECIAL``` Use our special attack, even if friendly.
- ```HIT_AND_RUN``` Flee for several turns after a melee attack.
- ```GUILT``` You feel guilty for killing it.
- ```HUMAN``` It's a live human, as long as it's alive.
- ```NO_BREATHE``` Creature can't drown and is unharmed by gas, smoke or poison.
- ```REGENERATES_50``` Monster regenerates very quickly over time.
- ```REGENERATES_10``` Monster regenerates very quickly over time.
- ```FLAMMABLE``` Monster catches fire, burns, and spreads fire to nearby objects.
- ```REVIVES``` Monster corpse will revive after a short period of time.
- ```CHITIN``` May produce chitin when butchered.
- ```VERMIN``` Creature is too small for normal combat, butchering etc.
- ```NOGIB``` Does not leave gibs / meat chunks when killed with huge damage.
- ```HUNTS_VERMIN``` Creature uses vermin as a food source. Not implemented.
- ```SMALL_BITER``` Creature can cause a painful, non-damaging bite. Not implemented.
- ```ABSORBS``` Consumes objects it moves over.
- ```LARVA``` Creature is a larva. Currently used for gib and blood handling.
- ```ARTHROPOD_BLOOD``` Forces monster to bleed hemolymph.
- ```ACID_BLOOD``` Makes monster bleed acid. Fun stuff! Does not automatically dissolve in a pool of acid on death.
- ```BILE_BLOOD``` Makes monster bleed bile.
- ```REGEN_MORALE``` Will stop fleeing if at max hp, and regen anger and morale.
- ```CBM_POWER``` May produce a power CBM when butchered, independent of CBM.
- ```CBM_SCI``` May produce a cbm or two from bionics_sci when butchered.
- ```CBM_OP``` May produce a cbm or two from bionics_op when butchered.
- ```FISHABLE``` It is fishable.
- ```INTERIOR_AMMO``` Monster contains ammo inside itself, no need to load on launch. Prevents ammo from being dropped on disable.

### Special attacks
Some special attacks are also valid use actions for tools and weapons.

- ```NONE``` No special attack.
- ```ANTQUEEN``` Hatches/grows: `egg > ant > soldier`.
- ```SHRIEK``` "a terrible shriek!"
- ```HOWL``` "an ear-piercing howl!"
- ```RATTLE``` "a sibilant rattling sound!"
- ```ACID``` Spit acid.
- ```SHOCKSTORM``` Shoots bolts of lightning.
- ```PULL_METAL_WEAPON``` Pull weapon that's made of iron or steel from the player's hand.
- ```SMOKECLOUD``` Produces a cloud of smoke.
- ```BOOMER``` Spit bile.
- ```RESURRECT``` Revives the dead--again.
- ```SCIENCE``` Various science/technology related attacks (e.g. manhacks, radioactive beams, etc.)
- ```GROWPLANTS``` Spawns underbrush, or promotes it to `> young tree > tree`.
- ```GROW_VINE``` Grows creeper vines.
- ```VINE``` Attacks with vine.
- ```SPIT_SAP``` Spit sap.
- ```TRIFFID_HEARTBEAT``` Grows and crumbles root walls around the player, and spawns more monsters.
- ```FUNGUS``` Releases fungal spores and attempts to infect the player.
- ```FUNGUS_GROWTH``` Grows a young fungaloid into an adult.
- ```FUNGUS_SPROUT``` Grows a fungal wall.
- ```LEAP``` Monster leaps from one point to another.
- ```DERMATIK``` Attempts to lay dermatik eggs in the player.
- ```DERMATIK_GROWTH``` Dermatik larva grows into an adult.
- ```PLANT``` Fungal spores take seed and grow into a fungaloid.
- ```DISAPPEAR``` Hallucination disappears.
- ```FORMBLOB``` Spawns blobs?
- ```DOGTHING``` The dog _thing_ spawns into a tentacle dog.
- ```TENTACLE``` Lashes a tentacle at the player.
- ```VORTEX``` Forms a vortex/tornado that causes damage and throws creatures around.
- ```GENE_STING``` Shoot a dart at the player that causes a mutation if it connects.
- ```PARA_STING``` Shoot a paralyzing dart at the player.
- ```TRIFFID_GROWTH``` Young triffid grows into an adult.
- ```STARE``` Stare at the player and inflict teleglow.
- ```FEAR_PARALYZE``` Paralyze the player with fear.
- ```PHOTOGRAPH``` Photograph the player. Causes a robot attack?
- ```TAZER``` Shock the player.
- ```SMG``` SMG turret fires.
- ```LASER``` Laser turret fires.
- ```RIFLE_TUR``` Rifle turret fires.
- ```FRAG_TUR``` MGL fires frag rounds.
- ```BMG_TUR``` Barrett .50BMG rifle fires.
- ```FLAMETHROWER``` Shoots a stream of fire.
- ```COPBOT``` Cop-bot alerts and then tazes the player.
- ```CHICKENBOT``` Robot can attack with tazer, M4, or MGL depending on distance.
- ```MULTI_ROBOT``` Robot can attack with tazer, flamethrower, M4, MGL, or 120mm cannon depending on distance.
- ```RATKING``` Inflicts disease `rat`
- ```GENERATOR``` Regenerates health.
- ```UPGRADE``` Upgrades a regular zombie into a special zombie.
- ```BREATHE``` Spawns a `breather`
- ```BITE``` Bites the player.
- ```BRANDISH``` Brandish a knife at the player.
- ```FLESH_GOLEM``` Attack the player with claw, and inflict disease `downed` if the attack connects.
- ```PARROT``` Parrots the speech defined in `speech.json`, picks one of the lines randomly. "speaker" points to a monster id.

### Anger, Fear & Placation Triggers

- ```NULL``` Source use only?
- ```STALK``` Increases when following the player.
- ```MEAT``` Meat or a corpse is nearby.
- ```PLAYER_WEAK``` The player is hurt.
- ```PLAYER_CLOSE``` The player gets within a few tiles distance.
- ```HURT``` The monster is hurt.
- ```FIRE``` There's a fire nearby.
- ```FRIEND_DIED``` A monster of the same type died.
- ```FRIEND_ATTACKED``` A monster of the same type was attacked.
- ```SOUND``` Heard a sound.

## Monster Groups

### Conditions
Limit when monsters can spawn.

#### Seasons
Multiple season conditions will be combined together so that any of those conditions become valid time of year spawn times.

- ```SUMMER```
- ```WINTER```
- ```AUTUMN```
- ```SPRING```

#### Time of day
Multiple time of day conditions will be combined together so that any of those conditions become valid time of day spawn times.

- ```DAY```
- ```NIGHT```
- ```DUSK```
- ```DAWN```

## Mutations

### Categories
These branches are also the valid entries for the categories of `dreams` in `dreams.json`

- ```MUTCAT_PLANT``` "You feel much closer to nature."
- ```MUTCAT_INSECT``` "You hear buzzing, and feel your body harden."
- ```MUTCAT_SPIDER``` "You feel insidious."
- ```MUTCAT_SLIME``` "Your body loses all rigidity for a moment."
- ```MUTCAT_FISH``` "You are overcome by an overwhelming longing for the ocean."
- ```MUTCAT_RAT``` "You feel a momentary nausea."
- ```MUTCAT_BEAST``` "Your heart races and you see blood for a moment."
- ```MUTCAT_CATTLE``` "Your mind and body slow down. You feel peaceful."
- ```MUTCAT_CEPHALOPOD``` "Your mind is overcome by images of eldritch horrors...and then they pass."
- ```MUTCAT_BIRD``` "Your body lightens and you long for the sky."
- ```MUTCAT_LIZARD``` "For a heartbeat, your body cools down."
- ```MUTCAT_TROGLOBITE``` "You yearn for a cool, dark place to hide."
- ```MUTCAT_ALPHA``` "You feel...better. Somehow."
- ```MUTCAT_MEDICAL``` "Your can feel the blood rushing through your veins and a strange, medicated feeling washes over your senses."
- ```MUTCAT_CHIMERA``` "You need to roar, bask, bite, and flap. NOW."
- ```MUTCAT_ELFA``` "Nature is becoming one with you..."
- ```MUTCAT_RAPTOR``` "Mmm...sweet bloody flavor...tastes like victory."

## Vehicle Parts

### Fuel types

- ```NULL``` None
- ```gasoline``` Refined dino.
- ```diesel``` Refined dino.
- ```battery``` Electrifying.
- ```plutonium``` 1.21 Gigawatts!
- ```plasma``` Superheated.
- ```water``` Clean.

### Flags

- ```NOINSTALL``` Cannot be installed.
- ```INTERNAL``` Must be mounted inside a cargo area.
- ```ANCHOR_POINT``` Allows secure seatbelt attachment.
- ```OVER``` Can be mounted over other parts.
- ```VARIABLE_SIZE``` Has 'bigness' for power, wheel radius, etc.
- ```BOARDABLE``` The player can safely move over or stand on this part while the vehicle is moving.
- ```CARGO``` Cargo holding area.
- ```COVERED``` Prevents items in cargo parts from emitting any light.
- ```BELTABLE``` Seatbelt can be attached to this part.
- ```SEATBELT``` Helps prevent the player from being ejected from the vehicle during an accident.
- ```SEAT``` A seat where the player can sit or sleep.
- ```TRACK``` Allows the vehicle installed on, to be marked and tracked on a PDA.
- ```UNMOUNT_ON_DAMAGE``` Part breaks off the vehicle when destroyed by damage.
- ```BED``` A bed where the player can sleep.
- ```OPAQUE``` Cannot be seen through.
- ```OBSTACLE``` Cannot walk through part, unless the part is also 'OPENABLE'.
- ```AISLE``` Player can move over this part with less speed penalty than normal.
- ```ROOF``` Covers a section of the vehicle. Areas of the vehicle that have a roof and roofs on surrounding sections, are considered inside. Otherwise they're outside.
- ```OPENABLE``` Can be opened or closed.
- ```OPENCLOSE_INSIDE```  Can be opened or closed, but only from inside the vehicle.
- ```WINDOW``` Can see through this part and can install curtains over it.
- ```SHARP``` Striking a monster with this part does cutting damage instead of bashing damage, and prevents stunning the monster.
- ```PROTRUSION``` Part sticks out so no other parts can be installed over it.
- ```WHEEL``` Counts as a wheel in wheel calculations.
- ```STABLE``` Similar to `WHEEL`, but if the vehicle is only a 1x1 section, this single wheel counts as enough wheels.
- ```ENGINE``` Is an engine and contributes towards vehicle mechanical power.
- ```ALTERNATOR``` Recharges batteries installed on the vehicle.
- ```MUSCLE_LEGS``` Power of the engine with such flag depends on player's strength.
- ```MUSCLE_ARMS``` Power of the engine with such flag depends on player's strength (it's less effective than 'MUSCLE_LEGS').
- ```FUEL_TANK``` Storage device for a fuel type.
- ```FRIDGE``` Can refrigerate items.
- ```CONTROLS``` Can be used to control the vehicle.
- ```CTRL_ELECTRONIC``` Controls electrical and electronic systems of the vehicle.
- ```MUFFLER``` Muffles the noise a vehicle makes while running.
- ```CURTAIN``` Can be installed over a part flagged with `WINDOW`, and functions the same as blinds found on windows in buildings.
- ```SOLAR_PANEL``` Recharges vehicle batteries when exposed to sunlight. Has a 1 in 4 chance of being broken on car generation.
- ```KITCHEN``` Acts as a kitchen unit and heat source for crafting.
- ```WELDRIG``` Acts as a welder for crafting.
- ```CRAFTRIG``` Acts as a dehydrator, vacuum sealer and reloading press for crafting purposes. Potentially to include additional tools in the future.
- ```CHEMLAB``` Acts as a chemistry set for crafting.
- ```FORGE``` Acts as a forge for crafting.
- ```TURRET``` Is a weapon turret.
- ```ARMOR``` Protects the other vehicle parts it's installed over during collisions.
- ```CONE_LIGHT``` Projects a cone of light when turned on.
- ```HORN``` Generates noise when used.
- ```MULTISQUARE``` Causes this part and any adjacent parts with the same ID to act as a singular part.
- ```CIRCLE_LIGHT``` Projects a circular radius of light when turned on.
- ```ODDTURN``` Only on during odd turns.
- ```EVENTURN``` Only on during even turns.
- ```RECHARGE``` Recharge items with the same flag. ( Currently only the rechargeable battery mod. )
- ```UNMOUNT_ON_MOVE``` Dismount this part when the vehicle moves. Doesn't drop the part, unless you give it special handling.
- ```POWER_TRANSFER``` Transmits power to and from an attached thingy (probably a vehicle)
- ```INITIAL_PART``` When starting a new vehicle via the construction menu, this vehicle part will be the initial part of the vehicle (if the used item matches the item required for this part).
- ```SCOOP``` Pulls items from underneath the vehicle to the cargo space of the part. Also mops up liquids. 
  - Uses the ```bonus``` tag to determine the maximum size of the item picked up
- ```PLANTER``` Plants seeds into tilled dirt, spilling them when the terrain underneath is unsuitable. It is damaged by running it over non-```DIGGABLE``` surfaces.
  - ```ADVANCED_PLANTER``` This planter doesn't spill seeds and avoids damaging itself on non-diggable surfaces.
- ```REAPER``` Cuts down mature crops, depositing them on the square
  - The ```bonus``` tag defines how productive the harvest can be.
- ```PLOW``` Tills the soil underneath the part while active. Takes damage from unsuitable terrain at a level proportional to the speed of the vehicle.
- ```EXTRA_DRAG``` tells the vehicle that the part exerts engine power reduction.

## Ammo

### Ammo type
These are handled through ammo_types.json.  You can tag a weapon with these to have it chamber existing ammo,
or make your own ammo there.  The first column in this list is the tag's "id", the internal identifier DDA uses
to track the tag, and the second is a brief description of the ammo tagged.  Use the id to search for ammo
listings, as ids are constant throughout DDA's code.  Happy chambering!  :-)

- ```22``` .22LR (and relatives)
- ```223``` .223 Remington (and 5.56 NATO)
- ```300``` .300 WinMag
- ```3006``` 30.06
- ```308``` .308 Winchester (and relatives)
- ```32``` .32 ACP
- ```36paper``` .36 cap & ball
- ```38``` .38 Special
- ```40``` 10mm
- ```44``` .44 Magnum
- ```44paper``` .44 cap & ball
- ```45``` .45 ACP (and relatives)
- ```454``` .454 Casull
- ```46``` 46mm
- ```5x50``` 5x50 Dart
- ```50``` .50 BMG
- ```500``` .500 Magnum
- ```57``` 57mm
- ```700nx``` .700 Nitro Express
- ```762x25``` 7.62x25mm
- ```762``` 7.62x39mm
- ```762R``` 7.62x54mm
- ```8x40mm``` 8mm Caseless
- ```9mm``` 9mm Luger (and relatives)
- ```12mm``` 12mm
- ```20x66mm``` 20x66mm Shot (and relatives)
- ```40mm``` 40mm Grenade
- ```66mm``` 66mm HEAT
- ```84x246mm``` 84x246mm HE
- ```120mm``` 120mm HEAT
- ```ammo_flintlock``` Flintlock ammo
- ```ampoule``` Ampoule
- ```arrow``` Arrow
- ```battery``` Battery
- ```BB``` BB
- ```blunderbuss``` Blunderbuss
- ```bolt``` Bolt
- ```charcoal``` Charcoal
- ```components``` Components
- ```dart``` Dart
- ```diesel``` Diesel
- ```fish_bait``` Fish bait
- ```fishspear``` Speargun spear
- ```fusion``` Laser Pack
- ```gasoline``` Gasoline
- ```homebrew_rocket``` homebrew rocket
- ```lamp_oil``` Lamp oil
- ```laser_capacitor``` Charge
- ```m235``` M235 TPA (66mm Incendiary Rocket)
- ```metal_rail``` Rebar Rail
- ```mininuke_mod``` Mininuke
- ```money``` Cents
- ```muscle``` Muscle
- ```nail``` Nail
- ```pebble``` Pebble
- ```plasma``` Plasma
- ```plutonium``` Plutonium Cell
- ```rebreather_filter``` Rebreather filter
- ```RPG-7``` RPG-7
- ```signal_flare``` Signal Flare
- ```shot``` Shotshell
- ```tape``` Duct tape
- ```thread``` Thread
- ```thrown``` Thrown
- ```unfinished_char``` Semi-charred fuel
- ```UPS``` UPS charges
- ```water``` Water

### Effects

- ```COOKOFF``` Explodes when lit on fire.
- ```SHOT``` Multiple smaller pellets instead of one singular bullet; less effective against armor but increases chance to hit.
- ```BOUNCE``` Inflicts target with `bounced` effect and rebounds to a nearby target without this effect.
- ```EXPLOSIVE``` Explodes without any shrapnel.
- ```EXPLOSIVE_BIG``` Large explosion without any shrapnel.
- ```EXPLOSIVE_HUGE``` Huge explosion without any shrapnel.
- ```FRAG``` Small explosion that spreads shrapnel.
- ```INCENDIARY``` Lights target on fire.
- ```NAPALM``` Explosion that spreads fire.
- ```TEARGAS``` Generates a cloud of teargas.
- ```SMOKE``` Generates a cloud of smoke.
- ```SMOKE_BIG``` Generates a large cloud of smoke.
- ```TRAIL``` Creates a trail of smoke.
- ```FLARE``` Lights the target on fire.
- ```FLASHBANG``` Blinds and deafens nearby targets.
- ```ACIDBOMB``` Leaves a pool of acid on detonation.
- ```FLAME``` Very small explosion that lights fires.
- ```STREAM``` Leaves a trail of fire fields.
- ```STREAM_BIG``` Leaves a trail of intense fire fields.
- ```BEANBAG``` Stuns the target.
- ```LARGE_BEANBAG``` Heavily stuns the target.
- ```MININUKE_MOD``` Small thermo-nuclear detonation that leaves behind radioactive fallout.
- ```LIGHTNING``` Creates a trail of lightning.
- ```PLASMA``` Creates a trail of superheated plasma.
- ```LASER``` Creates a trail of laser (the field type)
- ```NEVER_MISFIRES``` Firing ammo without this flag may trigger a misfiring, this is independent of the weapon flags.
- ```RECYCLED``` (For handmade ammo) causes the gun to misfire sometimes, this independent of the weapon flags.
- ```WHIP``` Special sounds for whips and has a chance of disarming the opponent.
- ```NOGIB``` Prevents overkill damage on the target (target won't explode into gibs, see also the monster flag NO_GIBS).
- ```WIDE``` Prevents `HARDTOSHOOT` monster flag from having any effect. Implied by ```SHOT``` or liquid ammo.
- ```BLINDS_EYES``` Blinds the target if it hits the head (ranged projectiles can't actually hit the eyes at the moment).
- ```ACID_DROP``` Creates a tiny field of weak acid.
- ```RECOVER_[X]``` where X is any of 3, 5, 10, 15, 25 - Has a (X-1/X) chance to create a single charge of the used ammo at the point of impact.
- ```NO_EMBED``` When an item would be spawned from the projectile, it will always be spawned on the ground rather than in monster's inventory. Implied for active thrown items. Doesn't do anything on projectiles that do not drop items.
- ```NO_ITEM_DAMAGE``` Will not damage items on the map even when it otherwise would try to.
- ```DRAW_AS_LINE``` Doesn't go through regular bullet animation, instead draws a line and the bullet on its end for one frame.

## Techniques
Techniques may be used by tools, armors, weapons and anything else that can be wielded.

- see contents of `data/json/techniques.json`
- techniques are also used with martial arts styles, see `data/json/martialarts.json`

## Armor

### Covers

- ```TORSO```
- ```HEAD```
- ```EYES```
- ```MOUTH```
- ```ARM_L```
- ```ARM_R```
- ```ARMS``` ... same ```ARM_L``` and ```ARM_R```
- ```HAND_L```
- ```HAND_R```
- ```HANDS``` ... same ```HAND_L``` and ```HAND_R```
- ```LEG_L```
- ```LEG_R```
- ```LEGS``` ... same ```LEG_L``` and ```LEG_R```
- ```FOOT_L```
- ```FOOT_R```
- ```FEET``` ... same ```FOOT_L``` and ```FOOT_R```

### Flags
Some armor flags, such as `WATCH` and `ALARMCLOCK` are compatible with other item types. Experiment to find which flags work elsewhere.

- ```ALARMCLOCK``` Has an alarm-clock feature.
- ```BELTED``` Layer for backpacks and things worn over outerwear.
- ```BLIND``` - Blinds the wearer while worn, and provides nominal protection v. flashbang flashes.
- ```BOOTS``` - You can store knives in this gear.
- ```COLLAR``` - This piece of clothing has a wide collar that can keep your mouth warm.
- ```DEAF``` Makes the player deaf.
- ```ELECTRIC_IMMUNE``` - This gear completely protects you from electric discharges.
- ```FANCY``` Wearing this clothing gives a morale bonus if the player has the `Stylish` trait.
- ```FRAGILE``` This gear is less resistant to damage than normal.
- ```FLOTATION``` Prevents the player from drowning in deep water. Also prevents diving underwater.
- ```furred``` - This piece of clothing has a fur lining sewn into it to increase its overall warmth.
- ```HOOD``` Allow this clothing to conditionally cover the head, for additional warmth or water protection., if the player's head isn't encumbered
- ```kevlar_padded``` - This gear has kevlar inserted into strategic locations to increase protection without increasing encumbrance.
- ```leather_padded``` - This gear has certain parts padded with leather to increase protection without increasing encumbrance.
- ```NO_QUICKDRAW``` - Don't offer to draw items from this holster when the fire key is pressed whilst the players hands are empty
- ```OUTER```  Outer garment layer.
- ```OVERSIZE``` Can always be worn no matter encumbrance/mutations/bionics/etc., but prevents any other clothing being worn over this.
- ```POCKETS``` Increases warmth for hands if the player's hands are cold and the player is wielding nothing.
- ```RAD_PROOF``` - This piece of clothing completely protects you from radiation.
- ```RAD_RESIST``` - This piece of clothing partially protects you from radiation.
- ```RAINPROOF``` Prevents the covered body-part(s) from getting wet in the rain.
- ```SKINTIGHT``` Undergarment layer.
- ```STURDY``` This clothing is a lot more resistant to damage than normal.
- ```SUN_GLASSES``` - Prevents glaring when in sunlight.
- ```SUPER_FANCY``` Gives an additional moral bonus over `FANCY` if the player has the `Stylish` trait.
- ```SWIM_GOGGLES``` - Allows you to see much further under water.
- ```THERMOMETER``` - This gear is equipped with an accurate thermometer.
- ```VARSIZE``` Can be made to fit via tailoring.
- ```WAIST``` Layer for belts other things worn on the waist.
- ```WATCH``` Acts as a watch and allows the player to see actual time.
- ```WATER_FRIENDLY``` Prevents the item from making the body part count as unfriendly to water and thus causing negative morale from being wet.
- ```WATERPROOF``` Prevents the covered body-part(s) from getting wet in any circumstance.
- ```wooled```, ```furred```, ```kevlar_padded```, ```leather_padded``` - This piece of clothing has a sewn into it to increase some properties (warmth/encumbrance/...).

## Comestibles

### Comestible type

- ```DRINK```
- ```FOOD```
- ```MED```

### Addiction type

- ```nicotine```
- ```caffeine```
- ```alcohol```
- ```sleeping pill```
- ```opiate```
- ```amphetamine```
- ```cocaine```
- ```crack```

### Use action

- ```NONE``` "You can't do anything of interest with your [x]."
- ```SEWAGE``` Causes vomiting and a chance to mutate.
- ```HONEYCOMB``` Spawns wax.
- ```ROYAL_JELLY``` Alleviates many negative conditions and diseases.
- ```BANDAGE``` Stop bleeding.
- ```FIRSTAID``` Heal.
- ```DISINFECTANT``` Prevents infections.
- ```CAFF``` Reduces fatigue.
- ```ATOMIC_CAFF``` Greatly reduces fatigue and increases radiation dosage.
- ```ALCOHOL``` Increases drunkenness. Adds disease `drunk`.
- ```ALCOHOL_WEAK``` Slightly increases drunkenness. Adds disease `drunk`
- ```PKILL``` Reduces pain. Adds disease `pkill[n]` where `[n]` is the level of flag `PKILL_[n]` used on this comestible.
- ```XANAX``` Alleviates anxiety. Adds disease `took_xanax`.
- ```CIG``` Alleviates nicotine cravings. Adds disease `cig`.
- ```ANTIBIOTIC``` Helps fight infections. Removes disease `infected` and adds disease `recover`.
- ```FUNGICIDE``` Kills fungus and spores. Removes diseases `fungus` and `spores`.
- ```WEED``` Makes you roll with Cheech & Chong. Adds disease `weed_high`.
- ```COKE``` Decreases hunger. Adds disease `high`.
- ```CRACK``` Decreases hunger. Adds disease `high`.
- ```GRACK``` Decreases hunger. Adds disease 'high'.
- ```METH``` Adds disease `meth`
- ```VITAMINS``` Increases healthiness (not to be confused with HP)
- ```VACCINE``` Greatly increases health.
- ```POISON``` Adds diseases `poison` and `foodpoison`.
- ```HALLU``` Adds disease `hallu`.
- ```THORAZINE``` Removes diseases `hallu`, `visuals`, `high`. Additionally removes disease `formication` if disease `dermatik` isn't also present. Has a chance of a negative reaction which increases fatigue.
- ```PROZAC``` Adds disease `took_prozac` if not currently present, otherwise acts as a minor stimulant.
- ```SLEEP``` Greatly increases fatigue.
- ```IODINE``` Adds disease `iodine`.
- ```FLUMED``` Adds disease `took_flumed`.
- ```FLUSLEEP``` Adds disease `took_flumed` and increases fatigue.
- ```INHALER``` Removes disease `asthma`.
- ```BLECH``` Causes vomiting.
- ```PLANTBLECH``` Causes vomiting if player does not contain plant mutations
- ```CHEW``` Displays message "You chew your %s", but otherwise does nothing.
- ```MUTAGEN``` Causes mutation.
- ```PURIFIER``` Removes negative mutations.
- ```MARLOSS``` "As you eat the berry, you have a near-religious experience, feeling at one with your surroundings..."
- ```DOGFOOD``` Makes a dog friendly.
- ```CATFOOD``` Makes a cat friendly.

### Flags

- ```EATEN_HOT``` Morale bonus for eating hot.
- ```EATEN_COLD``` Morale bonus for eating cold.
- ```USE_EAT_VERB``` "You drink your %s." or "You eat your %s."
- ```FERTILIZER``` Works as fertilizer for farming.
- ```LENS``` Lens items can make fires via focusing light rays.
- ```FIRE_DRILL``` Item will start fires in the primitive way.
- ```MUTAGEN_STRONG``` Chance of mutating several times.
- ```MUTAGEN_PLANT``` Causes mutation in the plant branch.
- ```MUTAGEN_INSECT``` Causes mutation in the insect branch.
- ```MUTAGEN_SPIDER``` Causes mutation in the spider branch.
- ```MUTAGEN_SLIME``` Causes mutation in the slime branch.
- ```MUTAGEN_FISH``` Causes mutation in the fish branch.
- ```MUTAGEN_RAT``` Causes mutation in the rat branch.
- ```MUTAGEN_BEAST``` Causes mutation in the beast branch.
- ```MUTAGEN_CATTLE``` Causes mutation in the cattle branch.
- ```MUTAGEN_CEPHALOPOD``` Causes mutation in the cephalopod branch.
- ```MUTAGEN_BIRD``` Causes mutation in the bird branch.
- ```MUTAGEN_LIZARD``` Causes mutation in the lizard branch.
- ```MUTAGEN_TROGLOBITE``` Causes mutation in the troglobite branch.
- ```MUTAGEN_MEDICAL``` Causes mutation in the medical branch.
- ```MUTAGEN_CHIMERA``` Causes mutation in the chimera branch.
- ```MUTAGEN_ALPHA``` Causes mutation in the alpha branch.
- ```MUTAGEN_ELFA``` Causes mutation in the elfa branch.
- ```MUTAGEN_RAPTOR``` Causes mutation in the raptor branch.
- ```PKILL_1``` Minor painkiller.
- ```PKILL_2``` Moderate painkiller.
- ```PKILL_3``` Heavy painkiller.
- ```PKILL_4``` "You shoot up."
- ```PKILL_L``` Slow-release painkiller.
- ```BREW``` ... Can be put into fermenting vat.
- ```HIDDEN_POISON``` ... Food is poisonous, visible only with a certain survival skill level.
- ```HIDDEN_HALLU``` ... Food causes hallucinations, visible only with a certain survival skill level.
- ```USE_ON_NPC``` Can be used on NPCs (not necessarily by them).

## Melee

### Flags

- ```SPEAR``` Deals stabbing damage, with a moderate chance of getting stuck. The `SPEAR` flag is synonymous with the `STAB` flag.
- ```CHOP``` Does cutting damage, with a high chance of getting stuck.
- ```STAB``` Deals stabbing damage, with a moderate chance of getting stuck. The `STAB` flag is synonymous with the `SPEAR` flag.
- ```SLICE``` Deals cutting damage, with a low chance of getting stuck.
- ```MESSY``` Resistant to getting stuck in a monster. Potentially cause more gore in the future?
- ```NON_STUCK``` Resistant to getting stuck in a monster; not as large of an effect as `MESSY`.
- ```UNARMED_WEAPON``` Wielding this item still counts as unarmed combat.
- ```NO_UNWIELD``` Cannot unwield this item.
- ```NO_RELOAD``` Item can never be reloaded (even if has a valid ammo type).
- ```SHEATH_SWORD``` Item can be sheathed in a sword scabbard
- ```IAIJUTSU``` Sword can slash at an enemy as it's drawn if cutting skill is above 7 and a roll is passed
- ```SHEATH_KNIFE``` Item can be sheathed in a knife sheath, it applicable to small/medium knives (with volume not bigger than 2)
- ```QUIVER_n``` Item can hold n arrows (will parse number as integer)
- ```ALWAYS_TWOHAND``` Item is always wielded with two hands. Without this, the items volume and weight are used to calculate this.
- ```BAYONET``` If the item is attached to a gun (as gunmod), the gun will use the cutting damage from the mod instead of its own.

## Guns

- ```MODE_BURST``` Has a burst-fire mode.
- ```BURST_ONLY``` No single-fire mode. Note that this is an additional flag to the above "MODE_BURST" flag.
- ```RELOAD_AND_SHOOT``` Firing automatically reloads and then shoots.
- ```RELOAD_ONE``` Only reloads one round at a time.
- ```NO_AMMO``` Does not directly have a loaded ammo type.
- ```BIO_WEAPON``` Weapon is a CBM weapon, uses power as ammo. (CBM weapons should get both NO_AMMO and BIO_WEAPON, to work correctly).
- ```CHARGE``` Has to be charged to fire. Higher charges do more damage.
- ```NO_UNLOAD``` Cannot be unloaded.
- ```FIRE_50``` Uses 50 shots per firing.
- ```FIRE_100``` Uses 100 shots per firing.
- ```BACKBLAST``` Causes a small explosion behind the person firing the weapon. Currently not implemented?
- ```STR_RELOAD``` Reload speed is affected by strength.
- ```RELOAD_EJECT``` Ejects shell from gun on reload instead of when fired.
- ```NO_BOOM``` Cancels the ammo effect "FLAME".
- ```STR8_DRAW``` Character needs at least strength 8 to use the full range of this bow, can not be used with less than 4 strength.
- ```STR10_DRAW``` Character needs at least strength 10 to use the full range of this bow, can not be used with less than 5 strength.
- ```STR12_DRAW``` Character needs at least strength 12 to use the full range of this bow, can not be used with less than 6 strength.
- ```MOUNTED_GUN``` Gun can only be used on terrain / furniture with the "MOUNTABLE" flag.
- ```WATERPROOF_GUN``` Gun does not rust and can be used underwater.
- ```UNDERWATER_GUN``` Gun is optimized for usage underwater, does perform badly outside of water.
- ```NEVER_JAMS``` Never malfunctions.
- ```COLLAPSIBLE_STOCK``` Reduces weapon volume proportional to the base size of the gun excluding any mods (see also SLOW_WIELD)
- ```IRREMOVABLE``` Makes so that the gunmod cannot be removed.

## Tools

### Flags
Melee flags are fully compatible with tool flags, and vice versa.

- ```LIGHT_[X]``` Illuminates the area with light intensity `[X]` where `[X]` is an intensity value. (e.x. `LIGHT_4` or `LIGHT_100`).
- ```CHARGEDIM``` If illuminated, light intensity fades with charge, starting at 20% charge left.
- ```FIRE``` Counts as a fire for crafting purposes.
- ```WRAP``` Unused?
- ```RECHARGE``` Gain charges when placed in a cargo area with a recharge station.
- ```USE_UPS``` Item is charges from an UPS / it uses the charges of an UPS instead of its own.
- ```NO_UNLOAD``` Cannot be unloaded.
- ```RADIOCARITEM``` Item can be put into a remote controlled car.
- ```RADIOSIGNAL_1``` Activated per radios signal 1.
- ```RADIOSIGNAL_2``` Activated per radios signal 2.
- ```RADIOSIGNAL_3``` Activated per radios signal 3.
- ```BOMB``` It's a radio controlled bomb.
- ```RADIO_CONTAINER``` It's a container of something that is radio controlled.
- ```RADIO_ACTIVATION``` It is activated by a remote control (also requires RADIOSIGNAL_*).
- ```FISH_GOOD``` When used for fishing, it's a good tool (requires that the matching use_action has been set).
- ```FISH_POOR``` When used for fishing, it's a poor tool (requires that the matching use_action has been set).
- ```CABLE_SPOOL``` This item is a cable spool and must be processed as such. It has an internal "state" variable which may be in the states "attach_first" or "pay_out_cable" -- in the latter case, set its charges to `max_charges - dist(here, point(vars["source_x"], vars["source_y"]))`. If this results in 0 or a negative number, set its state back to "attach_first".
- ```NO_DROP``` An item with this flag should never actually be dropped. Used internally to signal that an item was created, but that it is unwanted. Needless to say, don't use this in an item definition.
- ```WET``` Item is wet and will slowly dry off (e.g. towel).
- ```MC_MOBILE```, ```MC_RANDOM_STUFF```, ```MC_SCIENCE_STUFF```, ```MC_USED```, ```MC_HAS_DATA``` Memory card related flags, see `iuse.cpp`
- ```HAS_RECIPE``` Used by the E-Ink tablet to indicates it's currently showing a recipe.
- ```RADIO_MODABLE``` Indicates the item can be made into a radio-activated item.
- ```RADIO_MOD``` The item has been made into a radio-activated item.
- ```ACT_ON_RANGED_HIT```  The item should activate when thrown or fired, then immediately get processed if it spawns on the ground.
- ```BELT_CLIP``` The item can be clipped or hooked on to a belt loop of the appropriate size (belt loops are limited by their max_volume and max_weight properties)

### Flags that apply to items, not to item types.
Those flags are added by the game code to specific items (that specific welder, not *all* welders).

- ```DOUBLE_AMMO``` The tool has the double battery mod and has its max_charges doubled.
- ```USE_UPS``` The tool has the UPS mod and is charged from an UPS.
- ```ATOMIC_AMMO``` The tool has the atomic mod and runs on plutonium instead of normal batteries.
- ```FIT``` Reduces encumbrance by one.
- ```COLD``` Item is cold (see EATEN_COLD).
- ```HOT``` Item is hot (see EATEN_HOT).
- ```LITCIG``` Marks a lit smoking item (cigarette, joint etc.).
- ```WET``` Item is wet and will slowly dry off (e.g. towel).
- ```REVIVE_SPECIAL``` ... Corpses revives when the player is nearby.

## Books

- ```INSPIRATIONAL```` Reading this book grants bonus morale to characters with the SPIRITUAL trait.

### Use actions

- ```NONE``` Do nothing.
- ```FIRESTARTER``` Light a fire with a lens, primitive tools or lighters.
- ```SEW``` Sew clothing.
- ```SCISSORS``` Cut up clothing.
- ```HAMMER``` Pry boards off of windows, doors and fences.
- ```EXTINGUISHER``` Put out fires.
- ```EXTRA_BATTERY``` Doubles the amount of charges a battery-powered item holds.
- ```GASOLINE_LANTERN_OFF``` Turns the lantern on.
- ```GASOLINE_LANTERN_ON``` Turns the lantern off.
- ```LIGHT_OFF``` Turns the light on.
- ```LIGHT_ON``` Turns the light off.
- ```LIGHTSTRIP``` Activates the lightstrip.
- ```LIGHTSTRIP_ACTIVE``` The lightstrip fades and dies.
- ```GLOWSTICK``` Turn on the glowstick.
- ```GLOWSTICK_ACTIVE``` The glowstick fades and dies.
- ```HANDFLARE``` Light the flare.
- ```HANDFLARE_LIT``` The flare dies out.
- ```HOTPLATE``` Use the hotplate.
- ```SOLDER_WELD``` Solder or weld items, or cauterize wounds.
- ```WATER_PURIFIER``` Purify water.
- ```TWO_WAY_RADIO``` Listen to, or talk to others over the radio.
- ```RADIO_OFF``` Turn the radio on.
- ```RADIO_ON``` Turn the radio off.
- ```DIRECTIONAL_ANTENNA``` Find the source of a signal with your radio.
- ```NOISE_EMITTER_OFF``` Turn the noise emitter on.
- ```NOISE_EMITTER_ON``` Turn the noise emitter off.
- ```ROADMAP``` Learn of local common points-of-interest and show roads.
- ```SURVIVORMAP``` Learn of local points-of-interest that can help you survive, and show roads.
- ```MILITARYMAP``` Learn of local military installations, and show roads.
- ```RESTAURANTMAP``` Learn of local eateries, and show roads.
- ```TOURISTMAP``` Learn of local points-of-interest that a tourist would like to visit, and show roads.
- ```CROWBAR``` Pry open doors, windows, man-hole covers and many other things that need prying.
- ```MAKEMOUND``` Make a mound of dirt.
- ```DIG``` Dig a hole in the ground.
- ```SIPHON``` Siphon liquids out of vehicle.
- ```CHAINSAW_OFF``` Turn the chainsaw on.
- ```CHAINSAW_ON``` Turn the chainsaw off.
- ```ELEC_CHAINSAW_OFF``` Turn the electric chainsaw on.
- ```ELEC_CHAINSAW_ON``` Turn the electric chainsaw off.
- ```CARVER_OFF``` Turn the carver on.
- ```CARVER_ON``` Turn the carver off.
- ```COMBATSAW_OFF``` Turn the combat-saw on.
- ```COMBATSAW_ON``` Turn the combat-saw off
- ```JACKHAMMER``` Bust down walls and other constructions.
- ```JACQUESHAMMER``` Mr. Gorbachev, tear down this wall!
- ```SET_TRAP``` Set a trap.
- ```GEIGER``` Detect local radiation levels.
- ```TELEPORT``` Teleport.
- ```CAN_GOO``` Release a little blob buddy.
- ```PIPEBOMB``` Light a pipebomb.
- ```PIPEBOMB_ACT``` Let's hope it doesn't fizzle out.
- ```GRENADE``` Pull the pin on a grenade.
- ```GRENADE_ACT``` Throw it dummy!
- ```GRANADE``` Pull the pin on Granade.
- ```GRANADE_ACT``` Assaults enemies with source code fixes?
- ```FLASHBANG``` Pull the pin on a flashbang.
- ```FLASHBANG_ACT``` Hope it doesn't light any fires.
- ```EMPBOMB``` Pull the pin on an EMP grenade.
- ```EMPBOMB_ACT``` You may not be a robot, but you probably shouldn't keep holding it.
- ```SCRAMBLER``` Pull the pin on the scrambler grenade.
- ```SCRAMBLER_ACT``` I don't even know what this does, so you better get rid of it quick.
- ```GASBOMB``` Pull the pin on a teargas canister.
- ```GASBOMB_ACT``` Don't cry or pout, just get rid of it.
- ```SMOKEBOMB``` Pull the pin on a smoke bomb.
- ```SMOKEBOMB_ACT``` This may be a good way to hide as a smoker.
- ```ARROW_FLAMABLE``` Light your arrow and let fly.
- ```MOLOTOV``` Light the Molotov cocktail.
- ```MOLOTOV_LIT``` Throw it, but don't drop it.
- ```ACIDBOMB``` Pull the pin on an acid bomb.
- ```ACIDBOMB_ACT``` Get rid of it or you'll end up like that guy in Robocop.
- ```DYNAMITE``` Light a stick of dynamite.
- ```DYNAMITE_ACT``` Sister Sara still needs her mules.
- ```FIRECRACKER_PACK``` Light an entire packet of firecrackers.
- ```FIRECRACKER_PACK_ACT``` Keep the change you filthy animal.
- ```FIRECRACKER``` Light a singular firecracker.
- ```FIRECRACKER_ACT``` The saddest Fourth of July.
- ```MININUKE``` Set the timer and run. Or hit with a hammer (not really).
- ```MININUKE_ACT``` If you move quick enough, you might survive long enough to see _The Day After_.
- ```PHEROMONE``` Makes zombies love you.
- ```PORTAL``` Create portal traps.
- ```MANHACK``` Activate a manhack.
- ```TURRET``` Activate a turret.
- ```TURRET_LASER``` Activate a laser turret.
- ```UPS_OFF``` Turn on the UPS.
- ```UPS_ON``` Turn off the UPS.
- ```adv_UPS_ON``` Turn on the advanced UPS.
- ```adv_UPS_OFF``` Turn off the advanced UPS.
- ```TAZER``` Shock someone or something.
- ```SHOCKTONFA_OFF``` Turn the shocktonfa on.
- ```SHOCKTONFA_ON``` Turn the shocktonfa off.
- ```MP3``` Turn the mp3 player on.
- ```MP3_ON``` Turn the mp3 player off.
- ```PORTABLE_GAME``` Play games.
- ```C4``` Arm the C4.
- ```C4ARMED``` Just set it and forget it (get away from it though)!
- ```DOG_WHISTLE``` Dogs hate this thing; your dog seems pretty cool with it though.
- ```VACUTAINER``` Sucks the blood out of things like a robotic vampire.
- ```KNIFE``` Cut things up.
- ```FIREMACHETE_OFF``` Turn the fire-machete on.
- ```FIREMACHETE_ON``` Turn the fire-machete off.
- ```FIREKATANA_OFF``` Turn the fire-katana on.
- ```FIREKATANA_ON``` Turn the fire-katana off.
- ```ZWEIFIRE_OFF``` Turn the zwei-fire on.
- ```ZWEIFRE_ON``` Turn the zwei-fire off.
- ```BROADFIRE_OFF``` Turn the broad-fire on.
- ```BROADFIRE_ON``` Turn the broad-fire off.
- ```SHISHKEBAB_OFF``` Turn the shishkebab on.
- ```SHISHKEBAB_ON``` Turn the shshkebab off.
- ```LUMBER``` Cut logs into planks.
- ```HACKSAW``` Cut metal into chunks.
- ```TENT``` Pitch a tent.
- ```TORCH``` Light a torch.
- ```TORCH_LIT``` Extinguish the torch.
- ```BATTLETORCH``` Light the battle torch.
- ```BATTLETORCH_LIT``` Extinguish the battle torch.
- ```CANDLE``` Light the candle.
- ```CANDLE_LIT``` Extinguish the candle.
- ```BULLET_PULLER``` Pull bullets; says it right on the package.
- ```BOLTCUTTERS``` Use your town key to gain access anywhere.
- ```MOP``` Mop up the mess.
- ```PICKLOCK``` Attempt to pick the lock on doors.
- ```PICKAXE``` Does nothing but berate you for having it (I'm serious).
- ```SPRAY_CAN``` Graffiti the town.
- ```RAG``` Stop the bleeding.
- ```PDA``` Use your pda.
- ```PDA_FLASHLIGHT``` Use your pda as a flashlight.
- ```SHELTER``` Put up a full-blown shelter.
- ```HEATPACK``` Activate the heatpack and get warm.
- ```LAW``` Unpack the LAW for firing.
- ```DEJAR```
- ```DOLLCHAT``` That creepy doll just keeps on talking.
- ```TOWEL``` Dry your character using the item as towel.
- ```UNFOLD_BICYCLE``` Unfold the folding bicycle.
- ```MATCHBOMB``` Light the matchbomb.
- ```MATCHBOMB_ACT``` This thing is so hokey that you might actually be safe just holding onto it.
- ```HORN_BICYCLE``` Honk the horn.
- ```RAD_BADGE``` Take the radiation badge out of its protective case to start measuring absorbed dosage.
- ```AIRHORN``` Sound the horn.
- ```BELL``` Ring the bell.
- ```SEED``` Asks if you are sure that you want to eat the seed. As it is better to plant seeds.
- ```OXYGEN_BOTTLE```
- ```ATOMIC_BATTERY```
- ```FISHING_BASIC``` Use a fishing rod
- ```JET_INJECTOR``` Inject some jet drugs right into your veins.
- ```CABLE_ATTACH``` This item is a cable spool. Use it to try to attach to a vehicle.
- ```CAPTURE_MONSTER_ACT``` Capture and encapsulate a monster. The associated action is also used for releasing it.
  -```PLACE_RANDOMLY``` This is very much like the flag in the manhack iuse, it prevents the item from querying the player as to where they want the monster unloaded to, and instead choses randomly.

## Generic

### Flags
- ```UNRECOVERABLE``` Cannot be recovered from a disassembly.
- ```NO_SALVAGE``` Item cannot be broken down through a salvage process. Best used when something should not be able to be broken down (i.e. base components like leather patches).
- ```FLAMING``` ... Sets the target on fire when used as melee weapon.
- ```GAS_DISCOUNT``` ... Discount cards for the automated gas stations.
- ```RADIOACTIVE``` ... Is radioactive (can be used with LEAK_*).
- ```LEAK_ALWAYS``` ... Leaks (may be combined with "RADIOACTIVE").
- ```LEAK_DAM``` ... Leaks when damaged (may be combined with "RADIOACTIVE").
- ```UNBREAKABLE_MELEE``` ... Does never get damaged when used as melee weapon.
- ```DURABLE_MELEE``` ... Item is made to hit stuff and it does it well, so it's considered to be a lot tougher than other weapons made of the same materials.
- ```RAIN_PROTECT``` ... Protects from sunlight and from rain, when wielded.
- ```NO_PICKUP``` ... Character can not pickup anything while wielding this item (e.g. bionic claws).
- ```SLOW_WIELD``` ... Has an additional time penalty upon wielding. For melee weapons and guns this is offset by the relevant skill.
- ```REDUCED_WEIGHT``` ... Gunmod flag; reduces the item's base weight by 25%.
- ```REDUCED_BASHING``` ... Gunmod flag; reduces the item's bashing damage by 50%.
- ```PSEUDO``` ... Used internally to mark items that are referred to in the crafting inventory but are not actually items. They can be used as tools, but not as components.

## Skills

### Tags

- ```"gun_types"``` Define gun related skills?

## Scenarios

### Flags

- ```SUM_START``` ... start in summer.
- ```SPR_START``` ... start in spring.
- ```AUT_START``` ... start in autumn.
- ```WIN_START``` ... start in winter.
- ```SUR_START``` ... surrounded start, zombies outside the starting shelter.

## TODO

- Descriptions for `Special attacks` under `Monsters` could stand to be more descriptive of exactly what the attack does.
- `Ammo effects` under `Ammo` need more descriptive details, and some need to be double-checked for accuracy.

## MAP SPECIALS

- ```mx_null``` ... No special at all.
- ```mx_helicopter``` ... Metal wreckage and some items.
- ```mx_military``` ... Corpses and some military items.
- ```mx_science``` ... Corpses and some scientist items.
- ```mx_collegekids``` ... Corpses and items.
- ```mx_roadblock``` ... Roadblock furniture with turrets and some cars.
- ```mx_drugdeal``` ... Corpses and some drugs.
- ```mx_supplydrop``` ... Crates with some military items in it.
- ```mx_portal``` ... Portal to neither space.
- ```mx_minefield``` ... Landmines, a field of them.
- ```mx_crater``` ... Crater with rubble (and radioactivity).
- ```mx_fumarole``` ... A lava rift.
- ```mx_portal_in``` ... Another portal to neither space.
- ```mx_anomaly``` ...  Natural anomaly (crater + artifact).

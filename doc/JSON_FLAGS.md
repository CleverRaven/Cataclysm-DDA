# JSON Flags

## Notes

- Many of the flags intended for one category or item type, can be used in other categories or item types. Experiment to see where else flags can be used.
- Offensive and defensive flags can be used on any item type that can be wielded.

## Material Phases

- ```NULL```
- ```GAS```
- ```LIQUID```
- ```PLASMA```
- ```SOLID```

## Recipes

### Categories

- ```CC_AMMO```
- ```CC_ARMOR```
- ```CC_CHEM```
- ```CC_DRINK```
- ```CC_ELECTRONIC```
- ```CC_FOOD```
- ```CC_MISC```
- ```CC_WEAPON```

### Flags

- ```BLIND_EASY``` Easy to craft with little to no light
- ```BLIND_HARD``` Possible to craft with little to no light, but difficult
- ```UNCRAFT_SINGLE_CHARGE``` Lists returned amounts for one charge of an item that is counted by charges.
- ```UNCRAFT_LIQUIDS_CONTAINED``` Spawn liquid items in its default container.

## Furniture & Terrain

List of known flags, used in both terrain.json and furniture.json

### Flags

- ```ALARMED``` Sets off an alarm if smashed.
- ```ALLOW_FIELD_EFFECT``` Apply field effects to items inside ```SEALED``` terrain/furniture.
- ```AUTO_WALL_SYMBOL``` (only for terrain) The symbol of this terrain will be one of the line drawings (corner, T-intersection, straight line etc.) depending on the adjacent terrains.

    Example: `-` and `|` are both terrain with the `CONNECT_TO_WALL` flag. `O` does not have the flag, while `X` and `Y` have the `AUTO_WALL_SYMBOL` flag.

    `X` terrain will be drawn as a T-intersection (connected to west, south and east), `Y` will be drawn as horizontal line (going from west to east, no connection to south).
    ```
    -X-    -Y-
     |      O
    ```

- ```BARRICADABLE_DOOR``` Door that can be barricaded.
- ```BARRICADABLE_DOOR_DAMAGED```
- ```BARRICADABLE_DOOR_REINFORCED```
- ```BARRICADABLE_DOOR_REINFORCED_DAMAGED```
- ```BARRICADABLE_WINDOW``` Window that can be barricaded.
- ```BARRICADABLE_WINDOW_CURTAINS```
- ```BASHABLE``` Players + Monsters can bash this.
- ```CHIP``` Used in construction menu to determine if wall can have paint chipped off.
- ```COLLAPSES``` Has a roof that can collapse.
- ```CONNECT_TO_WALL``` (only for terrain) This flag has been superseded by the JSON entry `connects_to`, but is retained for backward compatibility.
- ```CONSOLE``` Used as a computer.
- ```CONTAINER``` Items on this square are hidden until looted by the player.
- ```DECONSTRUCT``` Can be deconstructed.
- ```DEEP_WATER```
- ```DESTROY_ITEM``` Items that land here are destroyed. See also `NOITEM`
- ```DIGGABLE``` Digging monsters, seeding monster, digging with shovel, etc.
- ```DOOR``` Can be opened (used for NPC path-finding).
- ```EASY_DECONSTRUCT``` Player can deconstruct this without tools.
- ```EXPLODES``` Explodes when on fire.
- ```FIRE_CONTAINER``` Stops fire from spreading (brazier, wood stove, etc.)
- ```FLAMMABLE``` Can be lit on fire.
- ```FLAMMABLE_ASH``` Burns to ash rather than rubble.
- ```FLAMMABLE_HARD``` Harder to light on fire, but still possible.
- ```FLAT``` Player can build and move furniture on.
- ```GOES_DOWN``` Can use <kbd>></kbd> to go down a level.
- ```GOES_UP``` Can use <kbd><</kbd> to go up a level.
- ```HARVESTED``` Marks the harvested version of a terrain type (e.g. harvesting an apple tree turns it into a harvested tree, which later becomes an apple tree again).
- ```INDOORS``` Has a roof over it; blocks rain, sunlight, etc.
- ```LADDER``` This piece of furniture that makes climbing easy (works only with z-level mode).
- ```LIQUID``` Blocks movement, but isn't a wall (lava, water, etc.)
- ```LIQUIDCONT``` Furniture that contains liquid, allows for contents to be accessed in some checks even if `SEALED`.
- ```MINEABLE``` Can be mined with a pickaxe/jackhammer.
- ```MOUNTABLE``` Suitable for guns with the `MOUNTED_GUN` flag.
- ```NOCOLLIDE``` Feature that simply doesn't collide with vehicles at all.
- ```NOITEM``` Items cannot be added here but may overflow to adjacent tiles. See also `DESTROY_ITEM`
- ```NO_FLOOR``` Things should fall when placed on this tile
- ```OPENCLOSE_INSIDE``` If it's a door (with an 'open' or 'close' field), it can only be opened or closed if you're inside.
- ```PAINFUL``` May cause a small amount of pain.
- ```PERMEABLE``` Permeable for gases.
- ```PLACE_ITEM``` Valid terrain for `place_item()` to put items on.
- ```PLANT``` A 'furniture' that grows and fruits.
- ```RAMP``` Can be used to move up a z-level
- ```RAMP_END```
- ```REDUCE_SCENT``` Reduces scent even more; only works if also bashable.
- ```ROAD``` Flat and hard enough to drive or skate (with rollerblades) on.
- ```ROUGH``` May hurt the player's feet.
- ```RUG``` Enables the `Remove Carpet` Construction entry.
- ```SALT_WATER``` Source of salt water (works for terrains with examine action "water_source").
- ```SEALED``` Can't use <kbd>e</kbd> to retrieve items; must smash them open first.
- ```SEEN_FROM_ABOVE``` Visible from a higher level (provided the tile above has no floor)
- ```SHARP``` May do minor damage to players/monsters passing through it.
- ```SHORT``` Feature too short to collide with vehicle protrusions. (mirrors, blades).
- ```SUPPORTS_ROOF``` Used as a boundary for roof construction.
- ```SUPPRESS_SMOKE``` Prevents smoke from fires; used by ventilated wood stoves, etc.
- ```SWIMMABLE``` Player and monsters can swim through it.
- ```THIN_OBSTACLE``` Passable by players and monsters; vehicles destroy it.
- ```TINY``` Feature too short to collide with vehicle undercarriage. Vehicles drive over them with no damage, unless a wheel hits them.
- ```TRANSPARENT``` Players and monsters can see through/past it. Also sets ter_t.transparent.
- ```UNSTABLE``` Walking here cause the bouldering effect on the character.
- ```WALL``` This terrain is an upright obstacle. Used for fungal conversion, and also implies `CONNECT_TO_WALL`.

### Examine actions

- ```none``` None
- ```aggie_plant``` Harvest plants.
- ```bars``` Take advantage of AMORPHOUS and slip through the bars.
- ```bulletin_board``` Create a home camp; currently not implemented.
- ```cardreader``` Use the cardreader with a valid card, or attempt to hack.
- ```chainfence``` Hop over the chain fence.
- ```controls_gate``` Controls the attached gate.
- ```dirtmound``` Plant seeds and plants.
- ```elevator``` Use the elevator to change floors.
- ```fault``` Displays descriptive message, but otherwise unused.
- ```flower_poppy``` Pick the mutated poppy.
- ```fswitch``` Flip the switch and the rocks will shift.
- ```fungus``` Release spores as the terrain crumbles away.
- ```gaspump``` Use the gas-pump.
- ```pit``` Cover the pit if you have some planks of wood.
- ```pit_covered``` Uncover the pit.
- ```pedestal_temple``` Opens the temple if you have a petrified eye.
- ```pedestal_wyrm``` Spawn wyrms.
- ```recycler``` Recycle metal objects.
- ```rubble``` Clear up the rubble if you have a shovel.
- ```safe``` Attempt to crack the safe.
- ```shelter``` Take down the shelter.
- ```shrub_marloss``` Pick a marloss bush.
- ```shrub_wildveggies``` Pick a wild veggies shrub.
- ```slot_machine``` Gamble.
- ```tent``` Take down the tent.
- ```toilet``` Either drink or get water out of the toilet.
- ```trap``` Interact with a trap.
- ```water_source``` Drink or get water from a water source.

### Currently only used for Fungal conversions

- ```FLOWER``` This furniture is a flower.
- ```FUNGUS``` Fungal covered.
- ```ORGANIC``` This furniture is partly organic.
- ```SHRUB``` This terrain is a shrub.
- ```TREE``` This terrain is a tree.
- ```YOUNG``` This terrain is a young tree.

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

- ```ACID``` Acid instead of a body. not the same as the ACID_BLOOD flag. In most cases you want both.
- ```AMIGARA``` Removes hypnosis if the last one.
- ```BLOBSPLIT``` Creates more blobs.
- ```BOOMER``` Explodes in vomit.
- ```BROKEN``` Spawns a broken robot item, its id calculated like this: the prefix "mon_" is removed from the monster id, than the prefix "broken_" is added. Example: mon_eyebot -> broken_eyebot
- ```DISAPPEAR``` Hallucination disappears.
- ```DISINTEGRATE``` Falls apart.
- ```EXPLODE``` Damaging explosion.
- ```FIREBALL``` 10 percent chance to explode in a fireball.
- ```FUNGUS``` Explodes in spores.
- ```GAMEOVER``` Game over man! Game over! Defense mode.
- ```GUILT``` Moral penalty. There is also a flag with a similar effect.
- ```KILL_BREATHERS``` All breathers die.
- ```KILL_VINES``` Kill all nearby vines.
- ```MELT``` Normal death, but melts.
- ```NORMAL``` Drop a body, leave gibs.
- ```RATKING``` Cure verminitis.
- ```SMOKEBURST``` Explode like a huge smoke bomb.
- ```THING``` Turn into a full thing.
- ```TRIFFID_HEART``` Destroys all roots.
- ```VINE_CUT``` Kill adjacent vine if it's cut.
- ```WORM``` Spawns 2 half-worms

### Flags

- ```NULL``` Source use only.
- ```ABSORBS``` Consumes objects it moves over. (Modders use this).
- ```ABSORBS_SPLITS``` Consumes objects it moves over, and if it absorbs enough it will split into a copy.
- ```ACIDPROOF``` Immune to acid.
- ```ACIDTRAIL``` Leaves a trail of acid.
- ```ACID_BLOOD``` Makes monster bleed acid. Fun stuff! Does not automatically dissolve in a pool of acid on death.
- ```ANIMAL``` Is an _animal_ for purposes of the `Animal Empathy` trait.
- ```AQUATIC``` Confined to water.
- ```ARTHROPOD_BLOOD``` Forces monster to bleed hemolymph.
- ```ATTACKMON``` Attacks other monsters.
- ```BADVENOM``` Attack may **severely** poison the player.
- ```BASHES``` Bashes down doors.
- ```BIRDFOOD``` Becomes friendly / tamed with bird food.
- ```BILE_BLOOD``` Makes monster bleed bile.
- ```BLEED``` Causes the player to bleed.
- ```BONES``` May produce bones and sinews when butchered.
- ```BORES``` Tunnels through just about anything (15x bash multiplier: dark wyrms' bash skill 12->180)
- ```CAN_DIG``` Can dig _and_ walk.
- ```CATFOOD``` Becomes friendly / tamed with cat food.
- ```CATTLEFODDER``` Becomes friendly / tamed with cattle fodder.
- ```CBM_CIV``` May produce a common CBM a power CBM when butchered.
- ```CBM_OP``` May produce a CBM or two from 'bionics_op' item group when butchered.
- ```CBM_POWER``` May produce a power CBM when butchered, independent of CBM.
- ```CBM_SUBS``` May produce a CBM or two from bionics_subs and a power CBM when butchered.
- ```CBM_SCI``` May produce a CBM from 'bionics_sci' item group when butchered.
- ```CBM_TECH``` May produce a CBM or two from 'bionics_tech' item group and a power CBM when butchered.
- ```CHITIN``` May produce chitin when butchered.
- ```CLIMBS``` Can climb.
- ```DESTROYS``` Bashes down walls and more. (2.5x bash multiplier, where base is the critter's max melee bashing)
- ```DIGS``` Digs through the ground.
- ```DOGFOOD``` Becomes friendly / tamed with dog food.
- ```DRIPS_NAPALM``` Occasionally drips napalm on move.
- ```ELECTRIC``` Shocks unarmed attackers.
- ```ELECTRONIC``` e.g. A Robot; affected by emp blasts and other stuff.
- ```FAT``` May produce fat when butchered.
- ```FEATHER``` May produce feathers when butchered.
- ```FLIES``` Can fly (over water, etc.)
- ```FIREPROOF``` Immune to fire.
- ```FIREY``` Burns stuff and is immune to fire.
- ```FISHABLE``` It is fishable.
- ```FLAMMABLE``` Monster catches fire, burns, and spreads fire to nearby objects.
- ```FUR``` May produce fur when butchered.
- ```GOODHEARING``` Pursues sounds more than most monsters.
- ```GRABS``` Its attacks may grab you!
- ```GROUP_BASH``` Gets help from monsters around it when bashing.
- ```GROUP_MORALE``` More courageous when near friends.
- ```GUILT``` You feel guilty for killing it.
- ```HARDTOSHOOT``` It's one size smaller for ranged attacks, no less then MS_TINY 
- ```HEARS``` It can hear you.
- ```HIT_AND_RUN``` Flee for several turns after a melee attack.
- ```HUMAN``` It's a live human, as long as it's alive.
- ```IMMOBILE``` Doesn't move (e.g. turrets)
- ```INTERIOR_AMMO``` Monster contains ammo inside itself, no need to load on launch. Prevents ammo from being dropped on disable.
- ```KEENNOSE``` Keen sense of smell.
- ```LARVA``` Creature is a larva. Currently used for gib and blood handling.
- ```LEATHER``` May produce leather when butchered.
- ```MILKABLE``` Produces milk when milked.
- ```NIGHT_INVISIBILITY``` Monster becomes invisible if it's more than one tile away and the lighting on its tile is LL_LOW or less. Visibility is not affected by night vision.
- ```NOHEAD``` Headshots not allowed!
- ```NOGIB``` Does not leave gibs / meat chunks when killed with huge damage.
- ```NO_BREATHE``` Creature can't drown and is unharmed by gas, smoke or poison.
- ```PARALYZE``` Attack may paralyze the player with venom.
- ```PLASTIC``` Absorbs physical damage to a great degree.
- ```POISON``` Poisonous to eat.
- ```PUSH_MON``` Can push creatures out of its way.
- ```QUEEN``` When it dies, local populations start to die off too.
- ```REGENERATES_10``` Monster regenerates quickly over time.
- ```REGENERATES_50``` Monster regenerates very quickly over time.
- ```REGENERATES_IN_DARK``` Monster regenerates very quickly in poorly lit tiles.
- ```REGEN_MORALE``` Will stop fleeing if at max hp, and regen anger and morale.
- ```REVIVES``` Monster corpse will revive after a short period of time.
- ```SEES``` It can see you (and will run/follow).
- ```SLUDGEPROOF``` Ignores the effect of sludge trails.
- ```SLUDGETRAIL``` Causes the monster to leave a sludge trap trail when moving.
- ```SMELLS``` It can smell you.
- ```STUMBLES``` Stumbles in its movement.
- ```SUNDEATH``` Dies in full sunlight.
- ```SWARMS``` Groups together and form loose packs.
- ```SWIMS``` Treats water as 50 movement point terrain.
- ```VENOM``` Attack may poison the player.
- ```VERMIN``` Obsolete flag for inconsequential monsters, now prevents loading.
- ```WARM``` Warm blooded.
- ```WEBWALK``` Doesn't destroy webs.
- ```WOOL``` May produce wool when butchered.

### Monster defense attacks

- ```NONE``` No special attack-back
- ```ACIDSPLASH``` Splash acid on the attacker
- ```ZAPBACK``` Shock attacker on hit

### Special attacks

Some special attacks are also valid use actions for tools and weapons.
See monsters.json for examples on how to use these attacks.
Also see monster_attacks.json for more special attacks, for example, impale and scratch

- ```NONE``` No special attack.
- ```ACID``` Spit acid.
- ```ACID_ACCURATE``` Shoots acid that is accurate at long ranges, but less so up close.
- ```ACID_BARF``` Barfs corroding, blinding acid.
- ```ANTQUEEN``` Hatches/grows: `egg > ant > soldier`.
- ```BIO_OP_TAKEDOWN``` Attack with special martial art takedown manoeuvres.
- ```BITE``` Bite attack that can cause deep infected wounds.
- ```BMG_TUR``` Barrett .50BMG rifle fires.
- ```BOOMER``` Spit bile.
- ```BOOMER_GLOW``` Spit glowing bile.
- ```BRANDISH``` Brandish a knife at the player.
- ```BREATHE``` Spawns a `breather`
- ```CALLBLOBS``` Calls 2/3 of nearby blobs to defend this monster, and sends 1/3 of nearby blobs after the player.
- ```CHICKENBOT``` Robot can attack with tazer, M4, or MGL depending on distance.
- ```COPBOT``` Cop-bot alerts and then tazes the player.
- ```DANCE``` Monster dances.
- ```DARKMAN``` Can cause darkness and wraiths to spawn.
- ```DERMATIK``` Attempts to lay dermatik eggs in the player.
- ```DERMATIK_GROWTH``` Dermatik larva grows into an adult.
- ```DISAPPEAR``` Hallucination disappears.
- ```DOGTHING``` The dog _thing_ spawns into a tentacle dog.
- ```FEAR_PARALYZE``` Paralyze the player with fear.
- ```FLAMETHROWER``` Shoots a stream of fire.
- ```FLESH_GOLEM``` Attack the player with claw, and inflict disease `downed` if the attack connects.
- ```FORMBLOB``` Spawns blobs?
- ```FRAG_TUR``` MGL fires frag rounds.
- ```FUNGUS``` Releases fungal spores and attempts to infect the player.
- ```FUNGUS_BIG_BLOSSOM``` Spreads fire suppressing fungal haze.
- ```FUNGUS_BRISTLE``` Perform barbed tendril attack that can cause fungal infections.
- ```FUNGUS_FORTIFY``` Grows Fungal hedgerows, and advances player on the mycus threshold path.
- ```FUNGUS_GROWTH``` Grows a young fungaloid into an adult.
- ```FUNGUS_HAZE``` Spawns fungal fields.
- ```FUNGUS_INJECT``` Perform needle attack that can cause fungal infections.
- ```FUNGUS_SPROUT``` Grows a fungal wall.
- ```GENERATOR``` Regenerates health.
- ```GENE_STING``` Shoot a dart at the player that causes a mutation if it connects.
- ```GRAB``` Grabs the player, slowing on hit, making movement and dodging impossible and blocking harder.
- ```GRAB``` GRAB the target, and drag it around.
- ```GROWPLANTS``` Spawns underbrush, or promotes it to `> young tree > tree`.
- ```GROW_VINE``` Grows creeper vines.
- ```HOWL``` "an ear-piercing howl!"
- ```JACKSON``` Converts zombies into zombie dancers.
- ```LASER``` Laser turret fires.
- ```LEAP``` leap away to an unobstructed tile.
- ```LONGSWIPE``` Does high damage claw attack, which can even hit some distance away.
- ```LUNGE``` Perform a jumping attack from some distance away, which can down the target.
- ```MULTI_ROBOT``` Robot can attack with tazer, flamethrower, M4, MGL, or 120mm cannon depending on distance.
- ```PARA_STING``` Shoot a paralyzing dart at the player.
- ```PARROT``` Parrots the speech defined in `speech.json`, picks one of the lines randomly. "speaker" points to a monster id.
- ```PHOTOGRAPH``` Photograph the player. Causes a robot attack?
- ```PLANT``` Fungal spores take seed and grow into a fungaloid.
- ```PULL_METAL_WEAPON``` Pull weapon that's made of iron or steel from the player's hand.
- ```RANGED_PULL``` Pull targets towards attacker.
- ```RATKING``` Inflicts disease `rat`
- ```RATTLE``` "a sibilant rattling sound!"
- ```RESURRECT``` Revives the dead--again.
- ```RIFLE_TUR``` Rifle turret fires.
- ```RIOTBOT``` Sprays teargas or relaxation gas, can handcuff players, and can use a blinding flash.
- ```SCIENCE``` Various science/technology related attacks (e.g. manhacks, radioactive beams, etc.)
- ```SEARCHLIGHT``` Tracks targets with a searchlight.
- ```SHOCKSTORM``` Shoots bolts of lightning.
- ```SHRIEK``` "a terrible shriek!"
- ```SHRIEK_ALERT``` "a very terrible shriek!"
- ```SHRIEK_STUN``` "a stunning shriek!", causes a small bash, can cause a stun.
- ```SLIMESPRING``` Can provide a morale boost to the player, and cure bite and bleed effects.
- ```SMASH``` Smashes the target for massive damage, sending it flying.
- ```SMG``` SMG turret fires.
- ```SPIT_SAP``` Spit sap.
- ```STARE``` Stare at the player and inflict teleglow.
- ```STRETCH_ATTACK``` Long ranged piercing attack.
- ```STRETCH_BITE``` Long ranged bite attack.
- ```SUICIDE``` Dies after attacking.
- ```TAZER``` Shock the player.
- ```TENTACLE``` Lashes a tentacle at the player.
- ```TRIFFID_GROWTH``` Young triffid grows into an adult.
- ```TRIFFID_HEARTBEAT``` Grows and crumbles root walls around the player, and spawns more monsters.
- ```UPGRADE``` Upgrades a regular zombie into a special zombie.
- ```VINE``` Attacks with vine.
- ```VORTEX``` Forms a vortex/tornado that causes damage and throws creatures around.

### Anger, Fear & Placation Triggers

- ```NULL``` Source use only?
- ```FIRE``` There's a fire nearby.
- ```FRIEND_ATTACKED``` A monster of the same type was attacked.
- ```FRIEND_DIED``` A monster of the same type died.
- ```HURT``` The monster is hurt.
- ```MEAT``` Meat or a corpse is nearby.
- ```PLAYER_CLOSE``` The player gets within a few tiles distance.
- ```PLAYER_WEAK``` The player is hurt.
- ```STALK``` Increases when following the player.
- ```SOUND``` Heard a sound.

## Monster Groups

### Conditions

Limit when monsters can spawn.

#### Seasons

Multiple season conditions will be combined together so that any of those conditions become valid time of year spawn times.

- ```AUTUMN```
- ```SPRING```
- ```SUMMER```
- ```WINTER```

#### Time of day

Multiple time of day conditions will be combined together so that any of those conditions become valid time of day spawn times.

- ```DAWN```
- ```DAY```
- ```DUSK```
- ```NIGHT```

## Mutations

### Categories

These branches are also the valid entries for the categories of `dreams` in `dreams.json`

- ```MUTCAT_ALPHA``` "You feel...better. Somehow."
- ```MUTCAT_BEAST``` "Your heart races and you see blood for a moment."
- ```MUTCAT_BIRD``` "Your body lightens and you long for the sky."
- ```MUTCAT_CATTLE``` "Your mind and body slow down. You feel peaceful."
- ```MUTCAT_CEPHALOPOD``` "Your mind is overcome by images of eldritch horrors...and then they pass."
- ```MUTCAT_CHIMERA``` "You need to roar, bask, bite, and flap. NOW."
- ```MUTCAT_ELFA``` "Nature is becoming one with you..."
- ```MUTCAT_FISH``` "You are overcome by an overwhelming longing for the ocean."
- ```MUTCAT_INSECT``` "You hear buzzing, and feel your body harden."
- ```MUTCAT_LIZARD``` "For a heartbeat, your body cools down."
- ```MUTCAT_MEDICAL``` "Your can feel the blood rushing through your veins and a strange, medicated feeling washes over your senses."
- ```MUTCAT_PLANT``` "You feel much closer to nature."
- ```MUTCAT_RAPTOR``` "Mmm...sweet bloody flavor...tastes like victory."
- ```MUTCAT_RAT``` "You feel a momentary nausea."
- ```MUTCAT_SLIME``` "Your body loses all rigidity for a moment."
- ```MUTCAT_SPIDER``` "You feel insidious."
- ```MUTCAT_TROGLOBITE``` "You yearn for a cool, dark place to hide."

## Vehicle Parts

### Fuel types

- ```NULL``` None
- ```battery``` Electrifying.
- ```diesel``` Refined dino.
- ```gasoline``` Refined dino.
- ```plasma``` Superheated.
- ```plutonium``` 1.21 Gigawatts!
- ```water``` Clean.

### Flags

- ```ADVANCED_PLANTER``` This planter doesn't spill seeds and avoids damaging itself on non-diggable surfaces.
- ```AISLE``` Player can move over this part with less speed penalty than normal.
- ```AISLE_LIGHT```
- ```ALTERNATOR``` Recharges batteries installed on the vehicle.
- ```ANCHOR_POINT``` Allows secure seatbelt attachment.
- ```ARMOR``` Protects the other vehicle parts it's installed over during collisions.
- ```ATOMIC_LIGHT```
- ```BATTERY_MOUNT```
- ```BED``` A bed where the player can sleep.
- ```BEEPER``` Generates noise when the vehicle moves backward.
- ```BELTABLE``` Seatbelt can be attached to this part.
- ```BOARDABLE``` The player can safely move over or stand on this part while the vehicle is moving.
- ```CAMERA```
- ```CAMERA_CONTROL```
- ```CAPTURE_MOSNTER_VEH``` Can be used to capture monsters when mounted on a vehicle.
- ```CARGO``` Cargo holding area.
- ```CARGO_LOCKING``` This cargo area is inaccessible to NPCs.
- ```CHEMLAB``` Acts as a chemistry set for crafting.
- ```CHIMES``` Generates continuous noise when used.
- ```CIRCLE_LIGHT``` Projects a circular radius of light when turned on.
- ```CONE_LIGHT``` Projects a cone of light when turned on.
- ```CONTROLS``` Can be used to control the vehicle.
- ```COVERED``` Prevents items in cargo parts from emitting any light.
- ```CRAFTRIG``` Acts as a dehydrator, vacuum sealer and reloading press for crafting purposes. Potentially to include additional tools in the future.
- ```CTRL_ELECTRONIC``` Controls electrical and electronic systems of the vehicle.
- ```CURTAIN``` Can be installed over a part flagged with ```WINDOW```, and functions the same as blinds found on windows in buildings.
- ```DIFFICULTY_REMOVE```
- ```DOME_LIGHT```
- ```DOOR_MOTOR```
- ```ENGINE``` Is an engine and contributes towards vehicle mechanical power.
- ```EVENTURN``` Only on during even turns.
- ```EXTRA_DRAG``` tells the vehicle that the part exerts engine power reduction.
- ```FAUCET```
- ```FOLDABLE```
- ```FORGE``` Acts as a forge for crafting.
- ```FRIDGE``` Can refrigerate items.
- ```FUNNEL```
- ```HORN``` Generates noise when used.
- ```INITIAL_PART``` When starting a new vehicle via the construction menu, this vehicle part will be the initial part of the vehicle (if the used item matches the item required for this part). The items of parts with this flag are automatically added as component to the vehicle start construction.
- ```INTERNAL``` Must be mounted inside a cargo area.
- ```KITCHEN``` Acts as a kitchen unit and heat source for crafting.
- ```LOCKABLE_CARGO``` Cargo containers that are able to have a lock installed.
- ```MUFFLER``` Muffles the noise a vehicle makes while running.
- ```MULTISQUARE``` Causes this part and any adjacent parts with the same ID to act as a singular part.
- ```MUSCLE_ARMS``` Power of the engine with such flag depends on player's strength (it's less effective than ```MUSCLE_LEGS```).
- ```MUSCLE_LEGS``` Power of the engine with such flag depends on player's strength.
- ```NAILABLE``` Attached with nails
- ```NEEDS_BATTERY_MOUNT```
- ```NOINSTALL``` Cannot be installed.
- ```NO_JACK```
- ```OBSTACLE``` Cannot walk through part, unless the part is also ```OPENABLE```.
- ```ODDTURN``` Only on during odd turns.
- ```ON_CONTROLS```
- ```OPAQUE``` Cannot be seen through.
- ```OPENABLE``` Can be opened or closed.
- ```OPENCLOSE_INSIDE```  Can be opened or closed, but only from inside the vehicle.
- ```OVER``` Can be mounted over other parts.
- ```PLANTER``` Plants seeds into tilled dirt, spilling them when the terrain underneath is unsuitable. It is damaged by running it over non-```DIGGABLE``` surfaces.
- ```PLOW``` Tills the soil underneath the part while active. Takes damage from unsuitable terrain at a level proportional to the speed of the vehicle.
- ```POWER_TRANSFER``` Transmits power to and from an attached thingy (probably a vehicle).
- ```PROTRUSION``` Part sticks out so no other parts can be installed over it.
- ```REACTOR```
- ```REAPER``` Cuts down mature crops, depositing them on the square.
- ```RECHARGE``` Recharge items with the same flag. ( Currently only the rechargeable battery mod. )
- ```REMOTE_CONTROLS```
- ```REVERSIBLE``` Removal has identical requirements to installation but is twice as quick
- ```ROOF``` Covers a section of the vehicle. Areas of the vehicle that have a roof and roofs on surrounding sections, are considered inside. Otherwise they're outside.
- ```SCOOP``` Pulls items from underneath the vehicle to the cargo space of the part. Also mops up liquids.
- ```SEAT``` A seat where the player can sit or sleep.
- ```SEATBELT``` Helps prevent the player from being ejected from the vehicle during an accident.
- ```SECURITY```
- ```SHARP``` Striking a monster with this part does cutting damage instead of bashing damage, and prevents stunning the monster.
- ```SOLAR_PANEL``` Recharges vehicle batteries when exposed to sunlight. Has a 1 in 4 chance of being broken on car generation.
- ```STABLE``` Similar to `WHEEL`, but if the vehicle is only a 1x1 section, this single wheel counts as enough wheels.
- ```STEERABLE``` This wheel is steerable.
- ```STEREO```
- ```TOOL_NONE``` Can be removed/installed without any tools
- ```TOOL_SCREWDRIVER``` Attached with screws, can be removed/installed with a screwdriver
- ```TOOL_WRENCH``` Attached with bolts, can be removed/installed with a wrench
- ```TRACK``` Allows the vehicle installed on, to be marked and tracked on map.
- ```TRACKED``` Contributes to steering effectiveness but doesn't count as a steering axle for install difficulty and still contributes to drag for the center of steering calculation.
- ```TURRET``` Is a weapon turret.
- ```UNMOUNT_ON_DAMAGE``` Part breaks off the vehicle when destroyed by damage.
- ```UNMOUNT_ON_MOVE``` Dismount this part when the vehicle moves. Doesn't drop the part, unless you give it special handling.
- ```VARIABLE_SIZE``` Has 'bigness' for power, wheel radius, etc.
- ```VISION```
- ```WELDRIG``` Acts as a welder for crafting.
- ```WHEEL``` Counts as a wheel in wheel calculations.
- ```WASHING_MACHINE``` Can be used to wash filthy clothes en masse.
- ```WINDOW``` Can see through this part and can install curtains over it.

## Ammo

### Ammo type

These are handled through ammo_types.json.  You can tag a weapon with these to have it chamber existing ammo, or make your own ammo there.  The first column in this list is the tag's "id", the internal identifier DDA uses to track the tag, and the second is a brief description of the ammo tagged.  Use the id to search for ammo listings, as ids are constant throughout DDA's code.  Happy chambering!  :-)

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
- ```9mm``` 9x19mm Luger (and relatives)
- ```9x18``` 9x18mm
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

- ```ACIDBOMB``` Leaves a pool of acid on detonation.
- ```BEANBAG``` Stuns the target.
- ```BLINDS_EYES``` Blinds the target if it hits the head (ranged projectiles can't actually hit the eyes at the moment).
- ```BOUNCE``` Inflicts target with `bounced` effect and rebounds to a nearby target without this effect.
- ```COOKOFF``` Explodes when lit on fire.
- ```CUSTOM_EXPLOSION``` Explosion as specified in ```"explosion"``` field of used ammo. See JSON_INFO.md
- ```DRAW_AS_LINE``` Doesn't go through regular bullet animation, instead draws a line and the bullet on its end for one frame.
- ```EXPLOSIVE``` Explodes without any shrapnel.
- ```EXPLOSIVE_BIG``` Large explosion without any shrapnel.
- ```EXPLOSIVE_HUGE``` Huge explosion without any shrapnel.
- ```FLAME``` Very small explosion that lights fires.
- ```FLARE``` Lights the target on fire.
- ```FLASHBANG``` Blinds and deafens nearby targets.
- ```FRAG``` Small explosion that spreads shrapnel.
- ```INCENDIARY``` Lights target on fire.
- ```LARGE_BEANBAG``` Heavily stuns the target.
- ```LASER``` Creates a trail of laser (the field type)
- ```LIGHTNING``` Creates a trail of lightning.
- ```MININUKE_MOD``` Small thermo-nuclear detonation that leaves behind radioactive fallout.
- ```MUZZLE_SMOKE``` Generate a small cloud of smoke at the source.
- ```NAPALM``` Explosion that spreads fire.
- ```NOGIB``` Prevents overkill damage on the target (target won't explode into gibs, see also the monster flag NO_GIBS).
- ```NO_EMBED``` When an item would be spawned from the projectile, it will always be spawned on the ground rather than in monster's inventory. Implied for active thrown items. Doesn't do anything on projectiles that do not drop items.
- ```NO_ITEM_DAMAGE``` Will not damage items on the map even when it otherwise would try to.
- ```NEVER_MISFIRES``` Firing ammo without this flag may trigger a misfiring, this is independent of the weapon flags.
- ```PLASMA``` Creates a trail of superheated plasma.
- ```RECOVER_[X]``` Has a (X-1/X) chance to create a single charge of the used ammo at the point of impact.
- ```RECYCLED``` (For handmade ammo) causes the gun to misfire sometimes, this independent of the weapon flags.
- ```SHOT``` Multiple smaller pellets; less effective against armor but increases chance to hit and no point-blank penalty
- ```SMOKE``` Generates a cloud of smoke at the target.
- ```SMOKE_BIG``` Generates a large cloud of smoke at the target.
- ```STREAM``` Leaves a trail of fire fields.
- ```STREAM_BIG``` Leaves a trail of intense fire fields.
- ```TRAIL``` Creates a trail of smoke.
- ```WIDE``` Prevents `HARDTOSHOOT` monster flag from having any effect. Implied by ```SHOT``` or liquid ammo.

## Techniques

Techniques may be used by tools, armors, weapons and anything else that can be wielded.

- see contents of `data/json/techniques.json`
- techniques are also used with martial arts styles, see `data/json/martialarts.json`

## Armor

### Covers

- ```ARMS``` ... same ```ARM_L``` and ```ARM_R```
- ```ARM_L```
- ```ARM_R```
- ```EYES```
- ```FEET``` ... same ```FOOT_L``` and ```FOOT_R```
- ```FOOT_L```
- ```FOOT_R```
- ```HANDS``` ... same ```HAND_L``` and ```HAND_R```
- ```HAND_L```
- ```HAND_R```
- ```HEAD```
- ```LEGS``` ... same ```LEG_L``` and ```LEG_R```
- ```LEG_L```
- ```LEG_R```
- ```MOUTH```
- ```TORSO```

### Flags

Some armor flags, such as `WATCH` and `ALARMCLOCK` are compatible with other item types. Experiment to find which flags work elsewhere.

- ```ALARMCLOCK``` Has an alarm-clock feature.
- ```ALLOWS_NATURAL_ATTACKS``` Doesn't prevent any natural attacks or similar benefits from mutations, fingertip razors, etc., like most items covering the relevant body part would.
- ```BAROMETER``` This gear is equipped with an accurate barometer (which is used to measure atmospheric pressure).
- ```BELTED``` Layer for backpacks and things worn over outerwear.
- ```BLIND``` Blinds the wearer while worn, and provides nominal protection v. flashbang flashes.
- ```BLOCK_WHILE_WORN``` Allows worn armor or shields to be used for blocking attacks.
- ```CLIMATE_CONTROL``` This piece of clothing has climate control of some sort, keeping you warmer or cooler depending on ambient and bodily temperature.
- ```COLLAR``` This piece of clothing has a wide collar that can keep your mouth warm.
- ```DEAF``` Makes the player deaf.
- ```ELECTRIC_IMMUNE``` This gear completely protects you from electric discharges.
- ```ONLY_ONE``` You can wear only one.
- ```FANCY``` Wearing this clothing gives a morale bonus if the player has the `Stylish` trait.
- ```FIX_FARSIGHT``` This gear corrects farsightedness.
- ```FIX_NEARSIGHT``` This gear corrects nearsightedness.
- ```FLOTATION``` Prevents the player from drowning in deep water. Also prevents diving underwater.
- ```FRAGILE``` This gear is less resistant to damage than normal.
- ```HELMET_COMPAT``` Items that are not SKINTIGHT or OVERSIZE but can be worn with a helmet.
- ```HOOD``` Allow this clothing to conditionally cover the head, for additional warmth or water protection., if the player's head isn't encumbered
- ```HYGROMETER``` This gear is equipped with an accurate hygrometer (which is used to measure humidity).
- ```NO_QUICKDRAW``` Don't offer to draw items from this holster when the fire key is pressed whilst the players hands are empty
- ```OUTER```  Outer garment layer.
- ```OVERSIZE``` Can always be worn no matter encumbrance/mutations/bionics/etc., but prevents any other clothing being worn over this.
- ```PARTIAL_DEAF``` Reduces the volume of sounds to a safe level.
- ```POCKETS``` Increases warmth for hands if the player's hands are cold and the player is wielding nothing.
- ```PSYSHIELD_PARTIAL``` 25% chance to protect against fear_paralyze monster attack when worn.
- ```RAD_PROOF``` This piece of clothing completely protects you from radiation.
- ```RAD_RESIST``` This piece of clothing partially protects you from radiation.
- ```RAINPROOF``` Prevents the covered body-part(s) from getting wet in the rain.
- ```RESTRICT_HANDS``` Prevents the player from wielding a weapon two-handed, forcing one-handed use if the weapon permits it.
- ```SKINTIGHT``` Undergarment layer.
- ```SLOWS_MOVEMENT``` This piece of clothing multiplies move cost by 1.1.
- ```SLOWS_THIRST``` This piece of clothing multiplies the rate at which the player grows thirsty by 0.70.
- ```STURDY``` This clothing is a lot more resistant to damage than normal.
- ```SUN_GLASSES``` Prevents glaring when in sunlight.
- ```SUPER_FANCY``` Gives an additional moral bonus over `FANCY` if the player has the `Stylish` trait.
- ```SWIM_GOGGLES``` Allows you to see much further under water.
- ```THERMOMETER``` This gear is equipped with an accurate thermometer (which is used to measure temperature).
- ```VARSIZE``` Can be made to fit via tailoring.
- ```WAIST``` Layer for belts other things worn on the waist.
- ```WATCH``` Acts as a watch and allows the player to see actual time.
- ```WATER_FRIENDLY``` Prevents the item from making the body part count as unfriendly to water and thus causing negative morale from being wet.
- ```WATERPROOF``` Prevents the covered body-part(s) from getting wet in any circumstance.

## Comestibles

### Comestible type

- ```DRINK```
- ```FOOD```
- ```MED```

### Addiction type

- ```alcohol```
- ```amphetamine```
- ```caffeine```
- ```cocaine```
- ```crack```
- ```nicotine```
- ```opiate```
- ```sleeping pill```

### Use action

- ```NONE``` "You can't do anything of interest with your [x]."
- ```ALCOHOL``` Increases drunkenness. Adds disease `drunk`.
- ```ALCOHOL_STRONG``` Greatly increases drunkenness. Adds disease `drunk`.
- ```ALCOHOL_WEAK``` Slightly increases drunkenness. Adds disease `drunk`
- ```ANTIBIOTIC``` Helps fight infections. Removes disease `infected` and adds disease `recover`.
- ```ATOMIC_CAFF``` Greatly reduces fatigue and increases radiation dosage.
- ```BANDAGE``` Stop bleeding.
- ```BIRDFOOD``` Makes a small bird friendly.
- ```BLECH``` Causes vomiting.
- ```CAFF``` Reduces fatigue.
- ```CATFOOD``` Makes a cat friendly.
- ```CATTLEFODDER``` Makes a large herbivore friendly.
- ```CHEW``` Displays message "You chew your %s", but otherwise does nothing.
- ```CIG``` Alleviates nicotine cravings. Adds disease `cig`.
- ```COKE``` Decreases hunger. Adds disease `high`.
- ```CRACK``` Decreases hunger. Adds disease `high`.
- ```DISINFECTANT``` Prevents infections.
- ```DOGFOOD``` Makes a dog friendly.
- ```FIRSTAID``` Heal.
- ```FLUMED``` Adds disease `took_flumed`.
- ```FLUSLEEP``` Adds disease `took_flumed` and increases fatigue.
- ```FUNGICIDE``` Kills fungus and spores. Removes diseases `fungus` and `spores`.
- ```SEWAGE``` Causes vomiting and a chance to mutate.
- ```HALLU``` Adds disease `hallu`.
- ```HONEYCOMB``` Spawns wax.
- ```INHALER``` Removes disease `asthma`.
- ```IODINE``` Adds disease `iodine`.
- ```MARLOSS``` "As you eat the berry, you have a near-religious experience, feeling at one with your surroundings..."
- ```METH``` Adds disease `meth`
- ```PKILL``` Reduces pain. Adds disease `pkill[n]` where `[n]` is the level of flag `PKILL_[n]` used on this comestible.
- ```PLANTBLECH``` Causes vomiting if player does not contain plant mutations
- ```POISON``` Adds diseases `poison` and `foodpoison`.
- ```PROZAC``` Adds disease `took_prozac` if not currently present, otherwise acts as a minor stimulant. Rarely has the `took_prozac_bad` adverse effect.
- ```PURIFIER``` Removes negative mutations.
- ```ROYAL_JELLY``` Alleviates many negative conditions and diseases.
- ```SLEEP``` Greatly increases fatigue.
- ```THORAZINE``` Removes diseases `hallu`, `visuals`, `high`. Additionally removes disease `formication` if disease `dermatik` isn't also present. Has a chance of a negative reaction which increases fatigue.
- ```VACCINE``` Greatly increases health.
- ```VITAMINS``` Increases healthiness (not to be confused with HP)
- ```WEED``` Makes you roll with Cheech & Chong. Adds disease `weed_high`.
- ```XANAX``` Alleviates anxiety. Adds disease `took_xanax`.

### Flags

- ```ACID``` when consumed using the BLECH function, penalties are reduced if acidproof.
- ```CARNIVORE_OK``` Can be eaten by characters with the Carnivore mutation.
- ```EATEN_HOT``` Morale bonus for eating hot.
- ```EATEN_COLD``` Morale bonus for eating cold.
- ```FERTILIZER``` Works as fertilizer for farming, of if this consumed with the PLANTBLECH function penalties will be reversed for plants.
- ```HIDDEN_POISON``` ... Food is poisonous, visible only with a certain survival skill level.
- ```HIDDEN_HALLU``` ... Food causes hallucinations, visible only with a certain survival skill level.
- ```MYCUS_OK``` Can be eaten by post-threshold Mycus characters. Only applies to mycus fruits by default.
- ```PKILL_1``` Minor painkiller.
- ```PKILL_2``` Moderate painkiller.
- ```PKILL_3``` Heavy painkiller.
- ```PKILL_4``` "You shoot up."
- ```PKILL_L``` Slow-release painkiller.
- ```USE_EAT_VERB``` "You drink your %s." or "You eat your %s."
- ```USE_ON_NPC``` Can be used on NPCs (not necessarily by them).
- ```ZOOM``` Zoom items can increase your overmap sight range.

## Melee

### Flags

- ```ALWAYS_TWOHAND``` Item is always wielded with two hands. Without this, the items volume and weight are used to calculate this.
- ```DIAMOND``` Diamond coating adds 30% bonus to cutting and piercing damage
- ```MESSY``` Creates more mess when pulping
- ```NO_CVD``` Item can never be used with a CVD machine
- ```NO_RELOAD``` Item can never be reloaded (even if has a valid ammo type).
- ```NO_UNWIELD``` Cannot unwield this item.
- ```REACH_ATTACK``` Allows to perform reach attack.
- ```SHEATH_KNIFE``` Item can be sheathed in a knife sheath, it applicable to small/medium knives (with volume not bigger than 2)
- ```SHEATH_SWORD``` Item can be sheathed in a sword scabbard
- ```SPEAR``` When making reach attacks intervening THIN_OBSTACLE terrain is not an obstacle
- ```UNARMED_WEAPON``` Wielding this item still counts as unarmed combat.
- ```WHIP``` Has a chance of disarming the opponent.

## Guns

- ```BACKBLAST``` Causes a small explosion behind the person firing the weapon. Currently not implemented?
- ```BIPOD``` Handling bonus only applies on MOUNTABLE map/vehicle tiles. Does not include wield time penalty (see SLOW_WIELD).
- ```CHARGE``` Has to be charged to fire. Higher charges do more damage.
- ```COLLAPSIBLE_STOCK``` Reduces weapon volume proportional to the base size of the gun (excluding any mods). Does not include wield time penalty (see NEEDS_UNFOLD).
- ```DISABLE_SIGHTS``` Prevents use of the base weapon sights
- ```FIRE_50``` Uses 50 shots per firing.
- ```FIRE_100``` Uses 100 shots per firing.
- ```FIRE_TWOHAND``` Gun can only be fired if player has two free hands.
- ```IRREMOVABLE``` Makes so that the gunmod cannot be removed.
- ```MOUNTED_GUN``` Gun can only be used on terrain / furniture with the "MOUNTABLE" flag.
- ```NEVER_JAMS``` Never malfunctions.
- ```NO_BOOM``` Cancels the ammo effect "FLAME".
- ```NO_UNLOAD``` Cannot be unloaded.
- ```PRIMITIVE_RANGED_WEAPON``` Allows using non-gunsmith tools to repair it (but not reinforce).
- ```PUMP_ACTION``` Gun has a rails on its pump action, allowing to install only mods with PUMP_RAIL_COMPATIBLE flag on underbarrel slot.
- ```PUMP_RAIL_COMPATIBLE``` Mod can be installed on underbarrel slot of guns with rails on their pump action.
- ```RAPIDFIRE``` Increases rate of fire by 50% for AUTO firing mode
- ```RELOAD_AND_SHOOT``` Firing automatically reloads and then shoots.
- ```RELOAD_EJECT``` Ejects shell from gun on reload instead of when fired.
- ```RELOAD_ONE``` Only reloads one round at a time.
- ```STR_DRAW``` Range with this weapon is reduced unless character has at least twice the required minimum strength
- ```STR_RELOAD``` Reload speed is affected by strength.
- ```UNDERWATER_GUN``` Gun is optimized for usage underwater, does perform badly outside of water.
- ```WATERPROOF_GUN``` Gun does not rust and can be used underwater.

### Firing modes
- ```MELEE``` Melee attack using properties of the gun or auxiliary gunmod
- ```NPC_AVOID``` NPC's will not attempt to use this mode
- ```SIMULTANEOUS``` All rounds fired concurrently (not sequentially) with recoil added only once (at the end)

## Magazines

- ```MAG_BULKY``` Can be stashed in an appropriate oversize ammo pouch (intended for bulky or awkwardly shaped magazines)
- ```MAG_COMPACT``` Can be stashed in an appropriate ammo pouch (intended for compact magazines)
- ```MAG_DESTROY``` Magazine is destroyed when the last round is consumed (intended for ammo belts). Has precedence over MAG_EJECT.
- ```MAG_EJECT``` Magazine is ejected from the gun/tool when the last round is consumed
- ```SPEEDLOADER``` Acts like a magazine, except it transfers rounds to the target gun instead of being inserted into it.

## Tools

### Flags

Melee flags are fully compatible with tool flags, and vice versa.

- ```ACT_ON_RANGED_HIT```  The item should activate when thrown or fired, then immediately get processed if it spawns on the ground.
- ```BELT_CLIP``` The item can be clipped or hooked on to a belt loop of the appropriate size (belt loops are limited by their max_volume and max_weight properties)
- ```BOMB``` It can be a remote controlled bomb.
- ```CABLE_SPOOL``` This item is a cable spool and must be processed as such. It has an internal "state" variable which may be in the states "attach_first" or "pay_out_cable" -- in the latter case, set its charges to `max_charges - dist(here, point(vars["source_x"], vars["source_y"]))`. If this results in 0 or a negative number, set its state back to "attach_first".
- ```CHARGEDIM``` If illuminated, light intensity fades with charge, starting at 20% charge left.
- ```DIG_TOOL``` If wielded, item is automatically activated when player tries to move onto map tile with ```MINEABLE``` flag.
- ```FIRE``` Counts as a fire for crafting purposes.
- ```FISH_GOOD``` When used for fishing, it's a good tool (requires that the matching use_action has been set).
- ```FISH_POOR``` When used for fishing, it's a poor tool (requires that the matching use_action has been set).
- ```HAS_RECIPE``` Used by the E-Ink tablet to indicates it's currently showing a recipe.
- ```LIGHT_[X]``` Illuminates the area with light intensity `[X]` where `[X]` is an intensity value. (e.x. `LIGHT_4` or `LIGHT_100`).
- ```MC_MOBILE```, ```MC_RANDOM_STUFF```, ```MC_SCIENCE_STUFF```, ```MC_USED```, ```MC_HAS_DATA``` Memory card related flags, see `iuse.cpp`
- ```NO_DROP``` Item should never exist on map tile as a discrete item (must be contained by another item)
- ```NO_UNLOAD``` Cannot be unloaded.
- ```RADIOCARITEM``` Item can be put into a remote controlled car.
- ```RADIOSIGNAL_1``` Activated per radios signal 1.
- ```RADIOSIGNAL_2``` Activated per radios signal 2.
- ```RADIOSIGNAL_3``` Activated per radios signal 3.
- ```RADIO_ACTIVATION``` It is activated by a remote control (also requires RADIOSIGNAL*).
- ```RADIO_CONTAINER``` It's a container of something that is radio controlled.
- ```RADIO_MOD``` The item has been made into a radio-activated item.
- ```RADIO_MODABLE``` Indicates the item can be made into a radio-activated item.
- ```RECHARGE``` Gain charges when placed in a cargo area with a recharge station.
- ```USE_UPS``` Item is charges from an UPS / it uses the charges of an UPS instead of its own.
- ```WET``` Item is wet and will slowly dry off (e.g. towel).

### Flags that apply to items, not to item types.

Those flags are added by the game code to specific items (that specific welder, not *all* welders).

- ```COLD``` Item is cold (see EATEN_COLD).
- ```FIT``` Reduces encumbrance by one.
- ```HOT``` Item is hot (see EATEN_HOT).
- ```LITCIG``` Marks a lit smoking item (cigarette, joint etc.).
- ```REVIVE_SPECIAL``` ... Corpses revives when the player is nearby.
- ```USE_UPS``` The tool has the UPS mod and is charged from an UPS.
- ```WET``` Item is wet and will slowly dry off (e.g. towel).

## Bionics

- ```BIONIC_FAULTY``` This bionic is a "faulty" bionic.
- ```BIONIC_POWER_SOURCE``` This bionic is a power source bionic.
- ```BIONIC_TOGGLED``` This bionic only has a function when activated, else it causes it's effect every turn.
- ```BIONIC_GUN``` This bionic is a gun bionic and activating it will fire it.  Prevents all other activation effects.
- ```BIONIC_WEAPON``` This bionic is a weapon bionic and activating it will create (or destroy) bionic's fake_item in user's hands.  Prevents all other activation effects.
- ```BIONIC_ARMOR_INTERFACE``` This bionic can provide power to powered armor.
- ```BIONIC_NPC_USABLE``` The NPC AI knows how to use this CBM and it can be installed on an NPC.

## Books

- ```INSPIRATIONAL``` Reading this book grants bonus morale to characters with the SPIRITUAL trait.

### Use actions

- ```NONE``` Do nothing.
- ```ACIDBOMB``` Pull the pin on an acid bomb.
- ```ACIDBOMB_ACT``` Get rid of it or you'll end up like that guy in Robocop.
- ```ARROW_FLAMABLE``` Light your arrow and let fly.
- ```BATTLETORCH``` Light the battle torch.
- ```BATTLETORCH_LIT``` Extinguish the battle torch.
- ```BELL``` Ring the bell.
- ```BOLTCUTTERS``` Use your town key to gain access anywhere.
- ```C4``` Arm the C4.
- ```CABLE_ATTACH``` This item is a cable spool. Use it to try to attach to a vehicle.
- ```CAN_GOO``` Release a little blob buddy.
- ```CAPTURE_MONSTER_ACT``` Capture and encapsulate a monster. The associated action is also used for releasing it.
- ```CARVER_OFF``` Turn the carver on.
- ```CARVER_ON``` Turn the carver off.
- ```CHAINSAW_OFF``` Turn the chainsaw on.
- ```CHAINSAW_ON``` Turn the chainsaw off.
- ```COMBATSAW_OFF``` Turn the combat-saw on.
- ```COMBATSAW_ON``` Turn the combat-saw off
- ```CROWBAR``` Pry open doors, windows, man-hole covers and many other things that need prying.
- ```DIG``` Clear rubble.
- ```DIRECTIONAL_ANTENNA``` Find the source of a signal with your radio.
- ```DOG_WHISTLE``` Dogs hate this thing; your dog seems pretty cool with it though.
- ```DOLLCHAT``` That creepy doll just keeps on talking.
- ```ELEC_CHAINSAW_OFF``` Turn the electric chainsaw on.
- ```ELEC_CHAINSAW_ON``` Turn the electric chainsaw off.
- ```EXTINGUISHER``` Put out fires.
- ```FIRECRACKER``` Light a singular firecracker.
- ```FIRECRACKER_ACT``` The saddest Fourth of July.
- ```FIRECRACKER_PACK``` Light an entire packet of firecrackers.
- ```FIRECRACKER_PACK_ACT``` Keep the change you filthy animal.
- ```FLASHBANG``` Pull the pin on a flashbang.
- ```GEIGER``` Detect local radiation levels.
- ```GRANADE``` Pull the pin on Granade.
- ```GRANADE_ACT``` Assaults enemies with source code fixes?
- ```GRENADE``` Pull the pin on a grenade.
- ```HACKSAW``` Cut metal into chunks.
- ```HAMMER``` Pry boards off of windows, doors and fences.
- ```HEATPACK``` Activate the heatpack and get warm.
- ```HEAT_FOOD``` Heat food around fires.
- ```HOTPLATE``` Use the hotplate.
- ```JACKHAMMER``` Bust down walls and other constructions.
- ```JET_INJECTOR``` Inject some jet drugs right into your veins.
- ```LAW``` Unpack the LAW for firing.
- ```LIGHTSTRIP``` Activates the lightstrip.
- ```LUMBER``` Cut logs into planks.
- ```MAKEMOUND``` Make a mound of dirt.
- ```MANHACK``` Activate a manhack.
- ```MATCHBOMB``` Light the matchbomb.
- ```MILITARYMAP``` Learn of local military installations, and show roads.
- ```MININUKE``` Set the timer and run. Or hit with a hammer (not really).
- ```MOLOTOV``` Light the Molotov cocktail.
- ```MOLOTOV_LIT``` Throw it, but don't drop it.
- ```MOP``` Mop up the mess.
- ```MP3``` Turn the mp3 player on.
- ```MP3_ON``` Turn the mp3 player off.
- ```SOLARPACK``` Unfold solar backpack array.
- ```SOLARPACK_OFF``` Fold solar backpack array.
- ```NOISE_EMITTER_OFF``` Turn the noise emitter on.
- ```NOISE_EMITTER_ON``` Turn the noise emitter off.
- ```PHEROMONE``` Makes zombies love you.
- ```PICKAXE``` Does nothing but berate you for having it (I'm serious).
- ```PIPEBOMB``` Light a pipebomb.
- ```PIPEBOMB_ACT``` Let's hope it doesn't fizzle out.
- ```PLACE_RANDOMLY``` This is very much like the flag in the manhack iuse, it prevents the item from querying the player as to where they want the monster unloaded to, and instead choses randomly.
- ```PORTABLE_GAME``` Play games.
- ```PORTAL``` Create portal traps.
- ```RADIO_OFF``` Turn the radio on.
- ```RADIO_ON``` Turn the radio off.
- ```RAG``` Stop the bleeding.
- ```RESTAURANTMAP``` Learn of local eateries, and show roads.
- ```ROADMAP``` Learn of local common points-of-interest and show roads.
- ```SCISSORS``` Cut up clothing.
- ```SEED``` Asks if you are sure that you want to eat the seed. As it is better to plant seeds.
- ```SEW``` Sew clothing.
- ```SHELTER``` Put up a full-blown shelter.
- ```SHOCKTONFA_OFF``` Turn the shocktonfa on.
- ```SHOCKTONFA_ON``` Turn the shocktonfa off.
- ```SIPHON``` Siphon liquids out of vehicle.
- ```SMOKEBOMB``` Pull the pin on a smoke bomb.
- ```SMOKEBOMB_ACT``` This may be a good way to hide as a smoker.
- ```SOLDER_WELD``` Solder or weld items, or cauterize wounds.
- ```SPRAY_CAN``` Graffiti the town.
- ```SURVIVORMAP``` Learn of local points-of-interest that can help you survive, and show roads.
- ```TAZER``` Shock someone or something.
- ```TELEPORT``` Teleport.
- ```TENT``` Pitch a tent.
- ```TORCH``` Light a torch.
- ```TORCH_LIT``` Extinguish the torch.
- ```TOURISTMAP``` Learn of local points-of-interest that a tourist would like to visit, and show roads.
- ```TOWEL``` Dry your character using the item as towel.
- ```TURRET``` Activate a turret.
- ```WASHCLOTHES``` Wash clothes with FILTHY flag.
- ```WATER_PURIFIER``` Purify water.
- ```BREAK_STICK``` Breaks long stick into two.

## Generic

### Flags

- ```ANESTHESIA``` ... Item is considered anesthesia for the purpose of installing or uninstalling bionics.
- ```DURABLE_MELEE``` ... Item is made to hit stuff and it does it well, so it's considered to be a lot tougher than other weapons made of the same materials.
- ```FRAGILE_MELEE``` ... Fragile items that fall apart easily when used as a weapon due to poor construction quality and will break into components when broken.
- ```GAS_DISCOUNT``` ... Discount cards for the automated gas stations.
- ```LEAK_ALWAYS``` ... Leaks (may be combined with "RADIOACTIVE").
- ```LEAK_DAM``` ... Leaks when damaged (may be combined with "RADIOACTIVE").
- ```NEEDS_UNFOLD``` ... Has an additional time penalty upon wielding. For melee weapons and guns this is offset by the relevant skill. Stacks with "SLOW_WIELD".
- ```NO_PICKUP``` ... Character can not pickup anything while wielding this item (e.g. bionic claws).
- ```NO_SALVAGE``` Item cannot be broken down through a salvage process. Best used when something should not be able to be broken down (i.e. base components like leather patches).
- ```PSEUDO``` ... Used internally to mark items that are referred to in the crafting inventory but are not actually items. They can be used as tools, but not as components. Implies "TRADER_AVOID".
- ```RADIOACTIVE``` ... Is radioactive (can be used with LEAK_*).
- ```RAIN_PROTECT``` ... Protects from sunlight and from rain, when wielded.
- ```REDUCED_BASHING``` ... Gunmod flag; reduces the item's bashing damage by 50%.
- ```REDUCED_WEIGHT``` ... Gunmod flag; reduces the item's base weight by 25%.
- ```SLOW_WIELD``` ... Has an additional time penalty upon wielding. For melee weapons and guns this is offset by the relevant skill. Stacks with "NEEDS_UNFOLD".
- ```TRADER_AVOID``` ... NPCs will not start with this item. Use this for active items (e.g. flashlight (on)), dangerous items (e.g. active bomb), fake item or unusual items (e.g. unique quest item).
- ```UNBREAKABLE_MELEE``` ... Does never get damaged when used as melee weapon.
- ```UNRECOVERABLE``` Cannot be recovered from a disassembly.
- ```DANGEROUS``` ... NPCs will not accept this item. Explosion iuse actor implies this flag. Implies "NPC_THROW_NOW".
- ```NO_REPAIR``` ... Prevents repairing of this item even if otherwise suitable tools exist.
- ```NPC_THROW_NOW``` ... NPCs will try to throw this item away, preferably at enemies. Implies "TRADER_AVOID" and "NPC_THROWN".
- ```NPC_ACTIVATE``` ... NPCs can activate this item as an alternative attack. Currently by throwing it right after activation. Implied by "BOMB".
- ```NPC_THROWN``` ... NPCs will throw this item (without activating it first) as an alternative attack.
- ```NPC_ALT_ATTACK``` ... Shouldn't be set directly. Implied by "NPC_ACTIVATE" and "NPC_THROWN".

## Skills

### Tags

- ```combat_skill``` The skill is considered a combat skill. It's affected by "PACIFIST", "PRED1", "PRED2", "PRED3", and "PRED4" traits.
- ```contextual_skill``` The skill is abstract, it depends on context (an indirect item to which it's applied). Neither player nor NPCs can possess it.

## Scenarios

### Flags

- ```SCEN_ONLY``` Profession can be chosen only as part of the appropriate scenario.

- ```ALLOW_OUTSIDE``` Allows placing player outside of building, useful for outdoor start
- ```BOARDED``` Start in boarded building (windows and doors are boarded, movable furniture is moved to windows and doors)
- ```SUR_START``` ... surrounded start, zombies outside the starting shelter.

- ```WIN_START``` ... start in winter.
- ```SPR_START``` ... start in spring.
- ```SUM_START``` ... start in summer.
- ```SUM_ADV_START``` ... start second summer after Cataclysm.
- ```AUT_START``` ... start in autumn.

## Overmap terrains

### Flags

- ```KNOWN_DOWN``` There's a known way down.
- ```KNOWN_UP``` There's a known way up.
- ```LINEAR``` For roads etc, which use ID_straight, ID_curved, ID_tee, ID_four_way.
- ```NO_ROTATE``` The terrain can't be rotated (ID_north, ID_east, ID_south, and ID_west instances will NOT be generated, just ID).
- ```RIVER``` It's a river tile.
- ```SIDEWALK``` Has sidewalks on the sides adjacent to roads.

## Overmap connections

### Flags

- ```ORTHOGONAL``` The connection generally prefers straight lines, avoids turning wherever possible.

## TODO

- Descriptions for `Special attacks` under `Monsters` could stand to be more descriptive of exactly what the attack does.
- `Ammo effects` under `Ammo` need more descriptive details, and some need to be double-checked for accuracy.

## MAP SPECIALS

- ```mx_null``` ... No special at all.
- ```mx_anomaly``` ...  Natural anomaly (crater + artifact).
- ```mx_collegekids``` ... Corpses and items.
- ```mx_crater``` ... Crater with rubble (and radioactivity).
- ```mx_drugdeal``` ... Corpses and some drugs.
- ```mx_fumarole``` ... A lava rift.
- ```mx_helicopter``` ... Metal wreckage and some items.
- ```mx_military``` ... Corpses and some military items.
- ```mx_minefield``` ... Landmines, a field of them.
- ```mx_portal``` ... Portal to neither space.
- ```mx_portal_in``` ... Another portal to neither space.
- ```mx_roadblock``` ... Roadblock furniture with turrets and some cars.
- ```mx_science``` ... Corpses and some scientist items.
- ```mx_supplydrop``` ... Crates with some military items in it.

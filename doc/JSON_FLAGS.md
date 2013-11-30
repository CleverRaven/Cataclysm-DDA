# JSON Flags

## Material Phases

- ```NULL```
- ```SOLID```
- ```LIQUID```
- ```GAS```
- ```PLASMA```

## Body Parts

- ```TORSO```
- ```HEAD```
- ```EYES```
- ```MOUTH```
- ```ARMS```
- ```HANDS```
- ```LEGS```
- ```FEET```

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

- ```TRANSPARENT``` Players and monsters can see thorugh/past it. Also sets ter_t.transparent.
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

### Death functions

- ```NORMAL``` Drop a body.
- ```ACID``` Acid instead of a body.
- ```BOOMER``` Explodes in vomit.
- ```KILL_VINES``` Kill all nearby vines.
- ```VINE_CUT``` Kill adjacent vine if it's cut.
- ```TRIFFID_HEART``` Destroys all roots.
- ```FUNGUS``` Explodes in spores.
- ```DISINTEGRATE``` Falls apart.
- ```WORM``` Spawns 2 half-worms
- ```DISAPPEAR``` Hallucination disappears.
- ```GUILT``` Moral penalty.
- ```BLOBSPLIT``` Creates more blobs.
- ```MELT``` Normal death, but melts.
- ```AMIGARA``` Removes hypnosis if the last one.
- ```THING``` Turn into a full thing.
- ```EXPLODE``` Damaging explosion.
- ```BROKEN``` Spawns a broken robot.
- ```RATKING``` Cure verminitis.
- ```KILL_BREATHERS``` All breathers die.
- ```SMOKEBURST``` Explode like a huge smoke bomb.
- ```ZOMBIE``` Generate proper clothing for zombies.
- ```GAMEOVER``` Game over man! Game over! Defense mode.

### Flags

- ```NULL``` Source use only.
- ```SEES``` It can see you (and will run/follow).
- ```VIS50``` Vision -10
- ```VIS40``` Vision -20
- ```VIS30``` Vision -30
- ```VIS20``` Vision -40
- ```VIS10``` Vision -50
- ```HEARS``` It can hear you.
- ```GOODHEARING``` Pursues sounds more than most monsters.
- ```SMELLS``` It can smell you.
- ```KEENNOSE``` Keen sense of smell.
- ```STUMBLES``` Stumbles in its movement.
- ```WARM``` Warm blooded.
- ```NOHEAD``` Headshots not allowed!
- ```HARDTOSHOOT``` Some shots are actually misses.
- ```GRABS``` Its attacks may grab you!
- ```BASHES``` Bashes down doors.
- ```DESTROYS``` Bashes down walls and more.
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
- ```CBM``` May produce a cbm or two when butchered.
- ```BONES``` May produce bones and sinews when butchered.
- ```IMMOBILE``` Doesn't move (e.g. turrets)
- ```FRIENDLY_SPECIAL``` Use our special attack, even if friendly.
- ```HIT_AND_RUN``` Flee for several turns after a melee atack.
- ```GUILT``` You feel guilty for killing it.
- ```HUMAN``` It's a live human, as long as it's alive.
- ```NO_BREATHE``` Creature can't drown and is unharmed by gas, smoke or poison.
- ```REGENERATES_50``` Monster regenerates very quickly over time.
- ```REGENERATES_10``` Monster regenerates very quickly over time.
- ```FLAMMABLE``` Monster catches fire, burns, and spreads fire to nearby objects.
- ```REVIVES``` Monster corpse will revive after a short period of time.
- ```CHITIN``` May produce chitin when butchered.
- ```VERMIN``` Creature is too small for normal combat, butchering etc.
- ```HUNTS_VERMIN``` Creature uses vermin as a food source.
- ```SMALL_BITER``` Creature can cause a painful, non-damaging bite.

### Special attacks

- ```NONE``` No special attack.
- ```ANTQUEEN``` Hatches/grows: `egg > ant > soldier`.
- ```SHRIEK``` "a terrible shriek!"
- ```RATTLE``` "a sibilant rattling sound!"
- ```ACID``` Spit acid.
- ```SHOCKSTORM``` Shoots bolts of lightning.
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
- ```FLAMETHROWER``` Shoots a stream fire.
- ```COPBOT``` Cop-bot alerts and then tazes the player.
- ```MULTI_ROBOT``` Robot can attack with tazer, flamethrower or SMG depending on distance.
- ```RATKING``` Inflicts disease `rat`
- ```GENERATOR``` Regenerates health.
- ```UPGRADE``` Upgrades a regular zombie into a special zombie.
- ```BREATHE``` Spawns a `breather`
- ```BITE``` Bites the player.
- ```BRANDISH``` Brandish a knife at the player.
- ```FLESH_GOLEM``` Attack the player with claw, and inflict disease `downed` if the attack connects.
- ```PARROT``` Parrots the speech defined in `migo_speech.json`

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
- ```MUTCAT_TROGLO``` "You yearn for a cool, dark place to hide."
- ```MUTCAT_ALPHA``` "You feel...better. Somehow."
- ```MUTCAT_MEDICAL``` "Your can feel the blood rushing through your veins and a strange, medicated feeling washes over your senses."
- ```MUTCAT_CHIMERA``` "You need to roar, bask, bite, and flap. NOW."
- ```MUTCAT_ELFA``` "Nature is becoming one with you..."
- ```MUTCAT_RAPTOR``` "Mmm...sweet bloody flavor...tastes like victory."

## Vehicle Parts

### Fuel types

- ```NULL``` None
- ```gasoline``` Refined dino.
- ```battery``` Electrifying.
- ```plutonium``` 1.21 Gigawatts!
- ```plasma``` Superheated.
- ```water``` Clean.

### Flags

- ```NOINSTALL``` Cannot be installed.
- ```INTERNAL``` Can be mounted inside other parts.
- ```ANCHOR_POINT``` Allows secure seatbelt attachment.
- ```OVER``` Can be mounted over other parts.
- ```VARIABLE_SIZE``` Has 'bigness' for power, wheel radius, etc.
- ```BOARDABLE``` The player can safely move over or stand on this part while the vehicle is moving.
- ```CARGO``` Cargo holding area.
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
- ```PEDALS``` Similar to 'ENGINE', but requires the player to manually power it.
- ```FUEL_TANK``` Storage device for a fuel type.
- ```FRIDGE``` Can refrigerate items.
- ```CONTROLS``` Can be used to control the vehicle.
- ```MUFFLER``` Muffles the noise a vehicle makes while running.
- ```CURTAIN``` Can be installed over a part flagged with `WINDOW`, and functions the same as blinds found on windows in buildings.
- ```SOLAR_PANEL``` Recharges vehicle batteries when exposed to sunlight.
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

## Skills

### Tags

- ```"gun_types"``` Define gun related skills?

## TODO

- Descriptions for `Special attacks` under `Monsters` could stand to be more descriptive of exactly what the attack does.
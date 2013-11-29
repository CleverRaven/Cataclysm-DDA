# JSON Flags

## Material Phases

- ```NULL```
- ```SOLID```
- ```LIQUID```
- ```GAS```
- ```PLASMA```

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
- ```ANTQUEEN```
- ```SHRIEK```
- ```RATTLE```
- ```ACID```
- ```SHOCKSTORM```
- ```SMOKECLOUD```
- ```BOOMER```
- ```RESURRECT```
- ```SCIENCE```
- ```GROWPLANTS```
- ```GROW_VINE```
- ```VINE```
- ```SPIT_SAP```
- ```TRIFFID_HEARTBEAT```
- ```FUNGUS```
- ```FUNGUS_GROWTH```
- ```FUNGUS_SPROUT```
- ```LEAP```
- ```DERMATIK```
- ```DERMATIK_GROWTH```
- ```PLANT```
- ```DISAPPEAR```
- ```FORMBLOB```
- ```DOGTHING```
- ```TENTACLE```
- ```VORTEX```
- ```GENE_STING```
- ```PARA_STING```
- ```TRIFFID_GROWTH```
- ```STARE```
- ```FEAR_PARALYZE```
- ```PHOTOGRAPH```
- ```TAZER```
- ```SMG```
- ```LASER```
- ```FLAMETHROWER```
- ```COPBOT```
- ```MULTI_ROBOT```
- ```RATKING```
- ```GENERATOR```
- ```UPGRADE```
- ```BREATHE```
- ```BITE```
- ```BRANDISH```
- ```FLESH_GOLEM```
- ```PARROT```
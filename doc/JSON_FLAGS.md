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

### Examine actions

- ```none``` None
- ```gaspump``` Use the gas-pump.
- ```toilet``` Either drink or get water out of the toilet.
- ```elevator``` Use the elevator to change floors.
- ```controls_gate``` Controls the attached gate.
- ```cardreader``` Use the cardreader with a valid card, or attempt to hack.
- ```rubble``` Clear up the rubble if you have a shovel.
- ```chainfence``` Hop over the chain fence.
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
- ```tree_apple``` Pick an apple tree.
- ```shrub_blueberry``` Pick a blueberry bush.
- ```shrub_strawberry``` Pick a strawberry bush.
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
- ```FAT``` May produce fat when butchered.
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
Some special attacks are also valid use actions for tools and weapons.

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
- ```INTERNAL``` Must be mounted inside a cargo area.
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
- ```RECHARGE``` Recharge items with the same flag. ( Currently only the rechargeable battery mod. )

## Ammo

### Ammo type
The chambering of weapons that this ammo can be loaded into.

- ```nail``` Nail
- ```BB``` BB
- ```bolt``` Bolt
- ```arrow``` Arrow
- ```pebble``` Pebble
- ```shot``` Shotshell
- ```22``` .22LR
- ```9mm``` 9mm Luger
- ```762x25``` 7.62x25mm
- ```38``` .38 Special
- ```40``` 10mm
- ```44``` .44 Magnum
- ```45``` .45 ACP
- ```454``` .454 Casull
- ```500``` .500 Magnum
- ```57``` 57mm
- ```46``` 46mm
- ```762``` 7.62x39mm
- ```223``` .223 Remington
- ```308``` .308 Winchester
- ```3006``` 30.06
- ```40mm``` 40mm Grenade
- ```66mm``` 66mm HEAT
- ```84x246mm``` 84x246mm HE
- ```m235``` M235 TPA (66mm Incendiary Rocket)
- ```battery``` Battery
- ```fusion``` Laser Pack
- ```12mm``` 12mm
- ```plasma``` Plasma
- ```plutonium``` Plutonium Cell
- ```gasoline``` Gasoline
- ```thread``` Thread
- ```water``` Water
- ```charcoal``` Charcoal
- ```8x40mm``` 8mm Caseless
- ```20x66mm``` 20x66mm Shot
- ```5x50``` 5x50 Dart
- ```signal_flare``` Signal Flare
- ```mininuke_mod``` Mininuke
- ```metal_rail``` Rebar Rail
- ```UPS``` UPS
- ```components``` Components
- ```thrown``` Thrown
- ```ampoule``` Ampoule
- ```50``` .50 BMG

### Effects

- ```COOKOFF``` Explodes when lit on fire.
- ```SHOT``` Multiple smaller pellets instead of one singular bullet; less effective against armor but increases chance to hit.
- ```BOUNCE``` Inflicts target with `ME_BOUNCED` effect.
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
- ```STREAM``` No effect? Not currently used in source files.
- ```BEANBAG``` Stuns the target.
- ```LARGE_BEANBAG``` Heavily stuns the target.
- ```MININUKE_MOD``` Small thermo-nuclear detonation that leaves behind radioactive fallout.
- ```LIGHTNING``` Creates a trail of lightning.
- ```PLASMA``` Creates a trail of superheated plasma.
- ```LASER``` Creates a trail of laser (the field type)

## Techniques
Techniques may be used by tools, armors, weapons and anything else that can be wielded.

### Offensive

- ```SWEEP``` Criticals may make your enemy fall & miss a turn.
- ```PRECISE``` Criticals are painful and stun.
- ```BRUTAL``` Criticals knock the target back.
- ```GRAB``` Hit may allow a second unarmed attack attempt.
- ```WIDE``` Attacks adjacent opponents.
- ```RAPID``` Hits faster.
- ```FEINT``` Misses take less time.
- ```THROW``` Attacks may throw your opponent.
- ```DISARM``` Remove an NPC's weapon.
- ```FLAMING``` Sets the target on fire.

### Defensive

- ```BLOCK``` Block attacks, reducing them to 25% damage.
- ```BLOCK_LEGS``` Block attacks, but with your legs.
- ```WBLOCK_1``` Poor chance to block when wielding this item (e.g. pole).
- ```WBLOCK_2``` Moderate chance to block when wielding this item (e.g. a weapon made for blocking)
- ```WBLOCK_3``` Good chance to block when wielding this item (e.g. a shield).
- ```BREAK``` Break from a grab.
- ```DEF_THROW``` Throw an enemy that attacks you.
- ```DEF_DISARM``` Disarm an enemy.

## Qualities
Qualities, like techniques, may be used by tools, armors, weapons and anything else that can be wielded.

- ```CUT``` Can be used to cut objects.

## Armor

### Covers

- ```TORSO```
- ```HEAD```
- ```EYES```
- ```MOUTH```
- ```ARMS```
- ```HANDS```
- ```LEGS```
- ```FEET```

### Flags
Some armor flags, such as `WATCH` and `ALARMCLOCK` are compatible with other item types. Experiment to find which flags work elsewhere.

- ```FIT``` Reduces encumbrance by one.
- ```VARSIZE``` Can be made to fit via tailoring.
- ```SKINTIGHT``` Reduces clothing layering penalty.
- ```WATER_FRIENDLY``` Prevents the covered body part(s) from getting drenched with water.
- ```WATERPROOF``` Prevents the covered body-part(s) from getting wet in any circumstance.
- ```RAINPROOF``` Prevents the covered body-part(s) from getting wet in the rain.
- ```STURDY``` This clothing is a lot more resistant to damage than normal.
- ```FANCY``` Wearing this clothing gives a morale bonus if the player has the `Stylish` trait.
- ```SUPER_FANCY``` Gives an additional moral bonus over `FANCY` if the player has the `Stylish` trait.
- ```POCKETS``` Increases warmth for hands if the player's hands are cold and the player is wielding nothing.
- ```HOOD``` Allow this clothing to conditionally cover the head, for additional warmth or water protection., if the player's head isn't encumbered
- ```FLOATATION``` Prevents the player from drowning in deep water. Also prevents diving underwater.
- ```OVERSIZE``` Can always be worn no matter encumbrance/mutations/bionics/etc., but prevents any other clothing being worn over this.
- ```WATCH``` Acts as a watch and allows the player to see actual time.
- ```ALARMCLOCK``` Has an alarm-clock feature.
- ```DEAF``` Makes the player deaf.

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
- ```CHEW``` Displays message "You chew your %s", but otherwise does nothing.
- ```MUTAGEN``` Causes mutation.
- ```PURIFIER``` Removes negative mutations.
- ```MARLOSS``` "As you eat the berry, you have a near-religious experience, feeling at one with your surroundings..."
- ```DOGFOOD``` Makes a dog friendly.
- ```CATFOOD```Makes a cat friendly.

### Flags

- ```EATEN_HOT``` Morale bonus for eating hot.
- ```USE_EAT_VERB``` "You drink your %s." or "You eat your %s."
- ```FERTILIZER``` Works as fertilizer for farming.
- ```SEED``` Plantable seed for farming.
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

## Containers

- ```RIGID``` Unused?
- ```WATERTIGHT``` Can hold liquids.
- ```SEALS``` Can be resealed.

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

## Guns

- ```MODE_BURST``` Has a burst-fire mode.
- ```RELOAD_AND_SHOOT``` Firing automatically reloads and then shoots.
- ```RELOAD_ONE``` Only reloads one round at a time.
- ```NO_AMMO``` Does not directly have a loaded ammo type.
- ```BIO_WEAPON``` Weapon is a CBM weapon, uses power as ammo. (CBM weapons should get both NO_AMMO and BIO_WEAPON, to work correctly).
- ```USE_UPS``` Uses 5 UPS charges per shot, or 3 advanced UPS charges.
- ```USE_UPS_20``` Uses 20 UPS charges per shot, or 12 advanced UPS charges.
- ```USE_UPS_40``` Uses 40 UPS charges per shot, or 24 advanced UPS charges.
- ```CHARGE``` Has to be charged to fire. Higher charges do more damage.
- ```NO_UNLOAD``` Cannot be unloaded.
- ```FIRE_50``` Uses 50 shots per firing.
- ```FIRE_100``` Uses 100 shots per firing.
- ```BACKBLAST``` Causes a small explosion behind the person firing the weapon. Currently not implemented?
- ```STR_RELOAD``` Reload speed is affected by strength.

## Tools

### Flags
Melee flags are fully compatible with tool flags, and vice versa.

- ```LIGHT_[X]``` Illuminates the area with light intensity `[X]` where `[X]` is an intensity value. (e.x. `LIGHT_4` or `LIGHT_100`).
- ```CHARGEDIM``` If illuminated, light intensity fades with charge, starting at 20% charge left.
- ```FIRE``` Counts as a fire for crafting purposes.
- ```WRAP``` Unused?
- ```RECHARGE``` Gain charges when placed in a cargo area with a recharge station. 

### Use actions

- ```NONE``` Do nothing.
- ```LIGHTER``` Light a fire.
- ```PRIMITIVE_FIRE``` Attempt to light a fire with a high chance of failure.
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
- ```MOLOTOV``` Light the molotov cocktail.
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
- ```ABSORBENT```
- ```UNFOLD_BICYCLE``` Unfold the folding bicycle.
- ```MATCHBOMB``` Light the matchbomb.
- ```MATCHBOMB_ACT``` This thing is so hokey that you might actually be safe just holding onto it.
- ```HORN_BICYCLE``` Honk the horn.
- ```RAD_BADGE``` Take the radiation badge out of its protective case to start measuring absorbed dosage.
- ```AIRHORN``` Sound the horn.
- ```JET_INJECTOR``` Inject some jet drugs right into your veins.

## Skills

### Tags

- ```"gun_types"``` Define gun related skills?

## TODO

- Descriptions for `Special attacks` under `Monsters` could stand to be more descriptive of exactly what the attack does.
- `Ammo effects` under `Ammo` need more descriptive details, and some need to be double-checked for accuracy.
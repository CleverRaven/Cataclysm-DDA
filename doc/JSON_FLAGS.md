# JSON Flags

* [Notes](#notes)
* [TODO](#todo)
* [Ammo](#ammo)
  + [Ammo type](#ammo-type)
  + [Effects](#effects)
* [Armor](#armor)
  + [Covers](#covers)
  + [Flags](#flags)
* [Bionics](#bionics)
* [Books](#books)
  + [Use actions](#use-actions)
* [Comestibles](#comestibles)
  + [Comestible type](#comestible-type)
  + [Addiction type](#addiction-type)
  + [Use action](#use-action)
  + [Flags](#flags-1)
* [Furniture and Terrain](#furniture-and-terrain)
  + [Flags](#flags-2)
  + [Examine Actions](#examine-actions)
  + [Fungal Conversions Only](#fungal-conversions-only)
  + [Furniture Only](#furniture-only)
* [Generic](#generic)
  + [Flags](#flags-3)
* [Guns](#guns)
  + [Firing modes](#firing-modes)
* [Magazines](#magazines)
* [MAP SPECIALS](#map-specials)
* [Material Phases](#material-phases)
* [Melee](#melee)
  + [Flags](#flags-4)
* [Monster Groups](#monster-groups)
  + [Conditions](#conditions)
    - [Seasons](#seasons)
    - [Time of day](#time-of-day)
* [Monsters](#monsters)
  + [Anger, Fear and Placation Triggers](#anger--fear-and-placation-triggers)
  + [Categories](#categories)
  + [Death Functions](#death-functions)
  + [Flags](#flags-5)
  + [Monster Defense and Attacks](#monster-defense-and-attacks)
  + [Sizes](#sizes)
  + [Special attacks](#special-attacks)
* [Mutations](#mutations)
  + [Categories](#categories-1)
* [Overmap](#overmap)
  + [Overmap connections](#overmap-connections)
    - [Flags](#flags-6)
  + [Overmap specials](#overmap-specials)
    - [Flags](#flags-7)
  + [Overmap terrains](#overmap-terrains)
    - [Flags](#flags-8)
* [Recipes](#recipes)
  + [Categories](#categories-2)
  + [Flags](#flags-9)
* [Scenarios](#scenarios)
  + [Flags](#flags-10)
    - [Season Flags](#season-flags)
* [Skills](#skills)
  + [Tags](#tags)
* [Techniques](#techniques)
* [Tools](#tools)
  + [Flags](#flags-11)
  + [Flags that apply to items](#flags-that-apply-to-items)
* [Vehicle Parts](#vehicle-parts)
  + [Flags](#flags-12)
  + [Fuel types](#fuel-types)


## Notes

- Many of the flags intended for one category or item type, can be used in other categories or item types. Experiment to see where else flags can be used.
- Offensive and defensive flags can be used on any item type that can be wielded.


## TODO

- Descriptions for `Special attacks` under `Monsters` could stand to be more descriptive of exactly what the attack does.
- `Ammo effects` under `Ammo` need more descriptive details, and some need to be double-checked for accuracy.


## Ammo

### Ammo type

These are handled through `ammo_types.json`.  You can tag a weapon with these to have it chamber existing ammo, or make your own ammo there.  The first column in this list is the tag's "id", the internal identifier DDA uses to track the tag, and the second is a brief description of the ammo tagged.  Use the id to search for ammo listings, as ids are constant throughout DDA's code.  Happy chambering!  :-)

- ```120mm``` 120mm HEAT
- ```12mm``` 12mm
- ```20x66mm``` 20x66mm Shot (and relatives)
- ```223``` .223 Remington (and 5.56 NATO)
- ```22``` .22LR (and relatives)
- ```3006``` 30.06
- ```300``` .300 WinMag
- ```308``` .308 Winchester (and relatives)
- ```32``` .32 ACP
- ```36paper``` .36 cap & ball
- ```38``` .38 Special
- ```40``` 10mm
- ```40mm``` 40mm Grenade
- ```44``` .44 Magnum
- ```44paper``` .44 cap & ball
- ```454``` .454 Casull
- ```45``` .45 ACP (and relatives)
- ```46``` 46mm
- ```500``` .500 Magnum
- ```50``` .50 BMG
- ```57``` 57mm
- ```5x50``` 5x50 Dart
- ```66mm``` 66mm HEAT
- ```700nx``` .700 Nitro Express
- ```762R``` 7.62x54mm
- ```762``` 7.62x39mm
- ```762x25``` 7.62x25mm
- ```84x246mm``` 84x246mm HE
- ```8x40mm``` 8mm Caseless
- ```9mm``` 9x19mm Luger (and relatives)
- ```9x18``` 9x18mm
- ```BB``` BB
- ```RPG-7``` RPG-7
- ```UPS``` UPS charges
- ```ammo_flintlock``` Flintlock ammo
- ```ampoule``` Ampoule
- ```arrow``` Arrow
- ```battery``` Battery
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
- ```shot``` Shotshell
- ```signal_flare``` Signal Flare
- ```tape``` Duct tape
- ```thread``` Thread
- ```thrown``` Thrown
- ```unfinished_char``` Semi-charred fuel
- ```water``` Water

### Effects

- ```ACIDBOMB``` Leaves a pool of acid on detonation.
- ```BEANBAG``` Stuns the target.
- ```BLACKPOWDER``` May clog up the gun with blackpowder fouling, which will also cause rust.
- ```BLINDS_EYES``` Blinds the target if it hits the head (ranged projectiles can't actually hit the eyes at the moment).
- ```BOUNCE``` Inflicts target with `bounced` effect and rebounds to a nearby target without this effect.
- ```COOKOFF``` Explodes when lit on fire.
- ```CUSTOM_EXPLOSION``` Explosion as specified in ```"explosion"``` field of used ammo. See `JSON_INFO.md`.
- ```DRAW_AS_LINE``` Doesn't go through regular bullet animation, instead draws a line and the bullet on its end for one frame.
- ```EXPLOSIVE_BIG``` Large explosion without any shrapnel.
- ```EXPLOSIVE_HUGE``` Huge explosion without any shrapnel.
- ```EXPLOSIVE``` Explodes without any shrapnel.
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
- ```NEVER_MISFIRES``` Firing ammo without this flag may trigger a misfiring, this is independent of the weapon flags.
- ```NOGIB``` Prevents overkill damage on the target (target won't explode into gibs, see also the monster flag NO_GIBS).
- ```TANGLE``` When this projectile hits a target, it has a chance to tangle them up and immobilise them.
- ```NO_EMBED``` When an item would be spawned from the projectile, it will always be spawned on the ground rather than in monster's inventory. Implied for active thrown items. Doesn't do anything on projectiles that do not drop items.
- ```NO_ITEM_DAMAGE``` Will not damage items on the map even when it otherwise would try to.
- ```MODIFIER``` Internal flag, effected monsters search for certain bonuses in their effect data.
- ```PLASMA``` Creates a trail of superheated plasma.
- ```RECOVER_[X]``` Has a (X-1/X) chance to create a single charge of the used ammo at the point of impact.
- ```RECYCLED``` (For handmade ammo) causes the gun to misfire sometimes, this independent of the weapon flags.
- ```SHOT``` Multiple smaller pellets; less effective against armor but increases chance to hit and no point-blank penalty
- ```SMOKE_BIG``` Generates a large cloud of smoke at the target.
- ```SMOKE``` Generates a cloud of smoke at the target.
- ```STREAM_BIG``` Leaves a trail of intense fire fields.
- ```STREAM``` Leaves a trail of fire fields.
- ```TRAIL``` Creates a trail of smoke.
- ```WIDE``` Prevents `HARDTOSHOOT` monster flag from having any effect. Implied by ```SHOT``` or liquid ammo.


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

- ```ACTIVE_CLOAKING``` While active, drains UPS to provide invisibility.
- ```ALARMCLOCK``` Has an alarm-clock feature.
- ```ALLOWS_NATURAL_ATTACKS``` Doesn't prevent any natural attacks or similar benefits from mutations, fingertip razors, etc., like most items covering the relevant body part would.
- ```AURA``` This item goes in the outer aura layer, intended for metaphysical effects.
- ```BAROMETER``` This gear is equipped with an accurate barometer (which is used to measure atmospheric pressure).
- ```BELTED``` Layer for backpacks and things worn over outerwear.
- ```BLIND``` Blinds the wearer while worn, and provides nominal protection v. flashbang flashes.
- ```BLOCK_WHILE_WORN``` Allows worn armor or shields to be used for blocking attacks.
- ```CLIMATE_CONTROL``` This piece of clothing has climate control of some sort, keeping you warmer or cooler depending on ambient and bodily temperature.
- ```COLLAR``` This piece of clothing has a wide collar that can keep your mouth warm.
- ```DEAF``` Makes the player deaf.
- ```ELECTRIC_IMMUNE``` This gear completely protects you from electric discharges.
- ```FANCY``` Wearing this clothing gives a morale bonus if the player has the `Stylish` trait.
- ```FIX_FARSIGHT``` This gear corrects farsightedness.
- ```FIX_NEARSIGHT``` This gear corrects nearsightedness.
- ```FLOTATION``` Prevents the player from drowning in deep water. Also prevents diving underwater.
- ```FRAGILE``` This gear is less resistant to damage than normal.
- ```HELMET_COMPAT``` Items that are not SKINTIGHT or OVERSIZE but can be worn with a helmet.
- ```HOOD``` Allow this clothing to conditionally cover the head, for additional warmth or water protection., if the player's head isn't encumbered
- ```HYGROMETER``` This gear is equipped with an accurate hygrometer (which is used to measure humidity).
- ```NO_TAKEOFF``` Item with that flag can't be taken off.
- ```NO_QUICKDRAW``` Don't offer to draw items from this holster when the fire key is pressed whilst the players hands are empty
- ```ONLY_ONE``` You can wear only one.
- ```OUTER```  Outer garment layer.
- ```OVERSIZE``` Can always be worn no matter encumbrance/mutations/bionics/etc., but prevents any other clothing being worn over this.
- ```PARTIAL_DEAF``` Reduces the volume of sounds to a safe level.
- ```PERSONAL``` This item goes in the personal aura layer, intended for metaphysical effects.
- ```POCKETS``` Increases warmth for hands if the player's hands are cold and the player is wielding nothing.
- ```PSYSHIELD_PARTIAL``` 25% chance to protect against fear_paralyze monster attack when worn.
- ```RAD_PROOF``` This piece of clothing completely protects you from radiation.
- ```RAD_RESIST``` This piece of clothing partially protects you from radiation.
- ```RAINPROOF``` Prevents the covered body-part(s) from getting wet in the rain.
- ```REQUIRES_BALANCE``` Gear that requires a certain balance to be steady with. If the player is hit while wearing, they have a chance to be downed.
- ```RESTRICT_HANDS``` Prevents the player from wielding a weapon two-handed, forcing one-handed use if the weapon permits it.
- ```ROLLER_ONE``` A less stable and slower version of ROLLER_QUAD, still allows the player to move faster than walking speed.
- ```ROLLER_QUAD```The medium choice between ROLLER_INLINE and ROLLER_ONE, while it is more stable, and moves faster, it also has a harsher non-flat terrain penalty then ROLLER_ONE.
- ```ROLLER_INLINE``` Faster, but less stable overall, the penalty for non-flat terrain is even harsher.
- ```SEMITANGIBLE``` Prevents the item from participating in the encumbrance system when worn.
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
- ```WATERPROOF``` Prevents the covered body-part(s) from getting wet in any circumstance.
- ```WATER_FRIENDLY``` Prevents the item from making the body part count as unfriendly to water and thus causing negative morale from being wet.


## Bionics

- ```BIONIC_ARMOR_INTERFACE``` This bionic can provide power to powered armor.
- ```BIONIC_FAULTY``` This bionic is a "faulty" bionic.
- ```BIONIC_GUN``` This bionic is a gun bionic and activating it will fire it.  Prevents all other activation effects.
- ```BIONIC_NPC_USABLE``` The NPC AI knows how to use this CBM and it can be installed on an NPC.
- ```BIONIC_POWER_SOURCE``` This bionic is a power source bionic.
- ```BIONIC_SLEEP_FRIENDLY``` This bionic won't prompt the user to turn it off if they try to sleep while it's active.
- ```BIONIC_TOGGLED``` This bionic only has a function when activated, else it causes it's effect every turn.
- ```BIONIC_WEAPON``` This bionic is a weapon bionic and activating it will create (or destroy) bionic's fake_item in user's hands.  Prevents all other activation effects.
- ```BIONIC_SHOCKPROOF``` This bionic can't be incapacitated by electrical attacks.


## Books

- ```INSPIRATIONAL``` Reading this book grants bonus morale to characters with the SPIRITUAL trait.

### Use actions

- ```ACIDBOMB_ACT``` Get rid of it or you'll end up like that guy in Robocop.
- ```ACIDBOMB``` Pull the pin on an acid bomb.
- ```AUTOCLAVE``` Sterilize one CBM by autoclaving it.
- ```ARROW_FLAMABLE``` Light your arrow and let fly.
- ```BATTLETORCH``` Light the battle torch.
- ```BELL``` Ring the bell.
- ```BOLTCUTTERS``` Use your town key to gain access anywhere.
- ```BREAK_STICK``` Breaks long stick into two.
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
- ```DIVE_TANK``` Use compressed air tank to breathe.
- ```DOG_WHISTLE``` Dogs hate this thing; your dog seems pretty cool with it though.
- ```DOLLCHAT``` That creepy doll just keeps on talking.
- ```ELEC_CHAINSAW_OFF``` Turn the electric chainsaw on.
- ```ELEC_CHAINSAW_ON``` Turn the electric chainsaw off.
- ```EXTINGUISHER``` Put out fires.
- ```FIRECRACKER_ACT``` The saddest Fourth of July.
- ```FIRECRACKER_PACK_ACT``` Keep the change you filthy animal.
- ```FIRECRACKER_PACK``` Light an entire packet of firecrackers.
- ```FIRECRACKER``` Light a singular firecracker.
- ```FLASHBANG``` Pull the pin on a flashbang.
- ```GEIGER``` Detect local radiation levels.
- ```GRANADE_ACT``` Assaults enemies with source code fixes?
- ```GRANADE``` Pull the pin on Granade.
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
- ```MOLOTOV_LIT``` Throw it, but don't drop it.
- ```MOLOTOV``` Light the Molotov cocktail.
- ```MOP``` Mop up the mess.
- ```MP3_ON``` Turn the mp3 player off.
- ```MP3``` Turn the mp3 player on.
- ```NOISE_EMITTER_OFF``` Turn the noise emitter on.
- ```NOISE_EMITTER_ON``` Turn the noise emitter off.
- ```NONE``` Do nothing.
- ```PACK_CBM``` Put CBM in special autoclave pouch so that they stay sterile once sterilized.
- ```PHEROMONE``` Makes zombies ignore you.
- ```PICKAXE``` Does nothing but berate you for having it (I'm serious).
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
- ```SMOKEBOMB_ACT``` This may be a good way to hide as a smoker.
- ```SMOKEBOMB``` Pull the pin on a smoke bomb.
- ```SOLARPACK_OFF``` Fold solar backpack array.
- ```SOLARPACK``` Unfold solar backpack array.
- ```SOLDER_WELD``` Solder or weld items, or cauterize wounds.
- ```SPRAY_CAN``` Graffiti the town.
- ```SURVIVORMAP``` Learn of local points-of-interest that can help you survive, and show roads.
- ```TAZER``` Shock someone or something.
- ```TELEPORT``` Teleport.
- ```TORCH``` Light a torch.
- ```TOURISTMAP``` Learn of local points-of-interest that a tourist would like to visit, and show roads.
- ```TOWEL``` Dry your character using the item as towel.
- ```TURRET``` Activate a turret.
- ```WASH_ALL_ITEMS``` Wash items with FILTHY flag.
- ```WASH_HARD_ITEMS``` Wash hard items with FILTHY flag.
- ```WASH_SOFT_ITEMS``` Wash soft items with FILTHY flag.
- ```WATER_PURIFIER``` Purify water.


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

- ```ALCOHOL_STRONG``` Greatly increases drunkenness. Adds disease `drunk`.
- ```ALCOHOL_WEAK``` Slightly increases drunkenness. Adds disease `drunk`
- ```ALCOHOL``` Increases drunkenness. Adds disease `drunk`.
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
- ```HALLU``` Adds disease `hallu`.
- ```HONEYCOMB``` Spawns wax.
- ```INHALER``` Removes disease `asthma`.
- ```IODINE``` Adds disease `iodine`.
- ```MARLOSS``` "As you eat the berry, you have a near-religious experience, feeling at one with your surroundings..."
- ```METH``` Adds disease `meth`
- ```NONE``` "You can't do anything of interest with your [x]."
- ```PKILL``` Reduces pain. Adds disease `pkill[n]` where `[n]` is the level of flag `PKILL_[n]` used on this comestible.
- ```PLANTBLECH``` Causes vomiting if player does not contain plant mutations
- ```POISON``` Adds diseases `poison` and `foodpoison`.
- ```PROZAC``` Adds disease `took_prozac` if not currently present, otherwise acts as a minor stimulant. Rarely has the `took_prozac_bad` adverse effect.
- ```PURIFIER``` Removes negative mutations.
- ```ROYAL_JELLY``` Alleviates many negative conditions and diseases.
- ```SEWAGE``` Causes vomiting and a chance to mutate.
- ```SLEEP``` Greatly increases fatigue.
- ```THORAZINE``` Removes diseases `hallu`, `visuals`, `high`. Additionally removes disease `formication` if disease `dermatik` isn't also present. Has a chance of a negative reaction which increases fatigue.
- ```VACCINE``` Greatly increases health.
- ```VITAMINS``` Increases healthiness (not to be confused with HP)
- ```WEED``` Makes you roll with Cheech & Chong. Adds disease `weed_high`.
- ```XANAX``` Alleviates anxiety. Adds disease `took_xanax`.

### Flags

- ```ACID``` when consumed using the BLECH function, penalties are reduced if acidproof.
- ```CARNIVORE_OK``` Can be eaten by characters with the Carnivore mutation.
- ```EATEN_COLD``` Morale bonus for eating cold.
- ```EATEN_HOT``` Morale bonus for eating hot.
- ```EDIBLE_FROZEN``` Being frozen doesn't prevent eating it. No morale bonus.
- ```INEDIBLE``` Inedible by default, enabled to eat when in conjunction with (mutation threshold) flags: BIRD, CATTLE.
- ```FERTILIZER``` Works as fertilizer for farming, of if this consumed with the PLANTBLECH function penalties will be reversed for plants.
- ```FREEZERBURN``` First thaw is MUSHY, second is rotten
- ```FUNGAL_VECTOR``` Will give a fungal infection when consumed.
- ```HIDDEN_HALLU``` ... Food causes hallucinations, visible only with a certain survival skill level.
- ```HIDDEN_POISON``` ... Food is poisonous, visible only with a certain survival skill level.
- ```MELTS``` Provides half fun unless frozen. Edible when frozen.
- ```MILLABLE``` Can be placed inside a mill, to turn into flour.
- ```MYCUS_OK``` Can be eaten by post-threshold Mycus characters. Only applies to mycus fruits by default.
- ```NO_INGEST``` Administered by some means other than oral intake.
- ```PKILL_1``` Minor painkiller.
- ```PKILL_2``` Moderate painkiller.
- ```PKILL_3``` Heavy painkiller.
- ```PKILL_4``` "You shoot up."
- ```PKILL_L``` Slow-release painkiller.
- ```SMOKABLE``` Accepted by smoking rack.
- ```SMOKED``` Not accepted by smoking rack (product of smoking).
- ```USE_EAT_VERB``` "You drink your %s." or "You eat your %s."
- ```USE_ON_NPC``` Can be used on NPCs (not necessarily by them).
- ```ZOOM``` Zoom items can increase your overmap sight range.

## Furniture and Terrain

List of known flags, used in both `terrain.json` and `furniture.json`.

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

- ```BARRICADABLE_DOOR_DAMAGED```
- ```BARRICADABLE_DOOR_REINFORCED_DAMAGED```
- ```BARRICADABLE_DOOR_REINFORCED```
- ```BARRICADABLE_DOOR``` Door that can be barricaded.
- ```BARRICADABLE_WINDOW_CURTAINS```
- ```BARRICADABLE_WINDOW``` Window that can be barricaded.
- ```BASHABLE``` Players + Monsters can bash this.
- ```BLOCK_WIND``` This terrain will block the effects of wind.
- ```BURROWABLE``` Burrowing monsters can travel under this terrain, while most others can't (e.g. graboid will traverse under the chain link fence, while ordinary zombie will be stopped by it).
- ```BUTCHER_EQ``` Butcher's equipment - required for full butchery of corpses.
- ```CAN_SIT``` Furniture the player can sit on. Player sitting near furniture with the "FLAT_SURF" tag will get mood bonus for eating.
- ```CHIP``` Used in construction menu to determine if wall can have paint chipped off.
- ```COLLAPSES``` Has a roof that can collapse.
- ```CONNECT_TO_WALL``` (only for terrain) This flag has been superseded by the JSON entry `connects_to`, but is retained for backward compatibility.
- ```CONSOLE``` Used as a computer.
- ```CONTAINER``` Items on this square are hidden until looted by the player.
- ```DECONSTRUCT``` Can be deconstructed.
- ```DEEP_WATER```
- ```DESTROY_ITEM``` Items that land here are destroyed. See also `NOITEM`
- ```DIGGABLE_CAN_DEEPEN``` Diggable location can be dug again to make deeper (e.g. shallow pit to deep pit).
- ```DIGGABLE``` Digging monsters, seeding monster, digging with shovel, etc.
- ```DOOR``` Can be opened (used for NPC path-finding).
- ```EASY_DECONSTRUCT``` Player can deconstruct this without tools.
- ```EMITTER``` This furniture will emit fields automatically as defined by its emissions entry
- ```EXPLODES``` Explodes when on fire.
- ```FIRE_CONTAINER``` Stops fire from spreading (brazier, wood stove, etc.)
- ```FLAMMABLE_ASH``` Burns to ash rather than rubble.
- ```FLAMMABLE_HARD``` Harder to light on fire, but still possible.
- ```FLAMMABLE``` Can be lit on fire.
- ```FLAT_SURF``` Furniture or terrain with a flat hard surface (e.g. table, but not chair; tree stump, etc.).
- ```FLAT``` Player can build and move furniture on.
- ```FORAGE_HALLU``` This item can be found with the `HIDDEN_HALLU` flag when found through foraging.
- ```FORAGE_POISION``` This item can be found with the `HIDDEN_POISON` flag when found through foraging.
- ```GOES_DOWN``` Can use <kbd>></kbd> to go down a level.
- ```GOES_UP``` Can use <kbd><</kbd> to go up a level.
- ```GROWTH_SEED``` This plant is in its seed stage of growth.
- ```GROWTH_SEEDLING``` This plant is in its seedling stage of growth.
- ```GROWTH_MATURE``` This plant is in a mature stage of a growth.
- ```GROWTH_HARVEST``` This plant is ready for harvest.
- ```HARVESTED``` Marks the harvested version of a terrain type (e.g. harvesting an apple tree turns it into a harvested tree, which later becomes an apple tree again).
- ```HIDE_PLACE``` Creatures on this tile can't be seen by creatures not standing on adjacent tiles
- ```INDOORS``` Has a roof over it; blocks rain, sunlight, etc.
- ```LADDER``` This piece of furniture that makes climbing easy (works only with z-level mode).
- ```LIQUIDCONT``` Furniture that contains liquid, allows for contents to be accessed in some checks even if `SEALED`.
- ```LIQUID``` Blocks movement, but isn't a wall (lava, water, etc.)
- ```MINEABLE``` Can be mined with a pickaxe/jackhammer.
- ```MOUNTABLE``` Suitable for guns with the `MOUNTED_GUN` flag.
- ```NOCOLLIDE``` Feature that simply doesn't collide with vehicles at all.
- ```NOITEM``` Items cannot be added here but may overflow to adjacent tiles. See also `DESTROY_ITEM`
- ```NO_FLOOR``` Things should fall when placed on this tile
- ```NO_SIGHT``` Creature on this tile have their sight reduced to one tile
- ```OPENCLOSE_INSIDE``` If it's a door (with an 'open' or 'close' field), it can only be opened or closed if you're inside.
- ```PAINFUL``` May cause a small amount of pain.
- ```PERMEABLE``` Permeable for gases.
- ```PLACE_ITEM``` Valid terrain for `place_item()` to put items on.
- ```PLANT``` A 'furniture' that grows and fruits.
- ```PLANTABLE``` This terrain or furniture can have seeds planted in it.
- ```PLOWABLE``` Terrain can be plowed.
- ```RAMP_END```
- ```RAMP``` Can be used to move up a z-level
- ```REDUCE_SCENT``` Reduces scent even more; only works if also bashable.
- ```ROAD``` Flat and hard enough to drive or skate (with rollerblades) on.
- ```ROUGH``` May hurt the player's feet.
- ```RUG``` Enables the `Remove Carpet` Construction entry.
- ```SALT_WATER``` Source of salt water (works for terrains with examine action "water_source").
- ```SEALED``` Can't use <kbd>e</kbd> to retrieve items; must smash them open first.
- ```SEEN_FROM_ABOVE``` Visible from a higher level (provided the tile above has no floor)
- ```SHARP``` May do minor damage to players/monsters passing through it.
- ```SHORT``` Feature too short to collide with vehicle protrusions. (mirrors, blades).
- ```SIGN``` Show written message on examine.
- ```SUPPORTS_ROOF``` Used as a boundary for roof construction.
- ```SUPPRESS_SMOKE``` Prevents smoke from fires; used by ventilated wood stoves, etc.
- ```SWIMMABLE``` Player and monsters can swim through it.
- ```THIN_OBSTACLE``` Passable by players and monsters; vehicles destroy it.
- ```TINY``` Feature too short to collide with vehicle undercarriage. Vehicles drive over them with no damage, unless a wheel hits them.
- ```TRANSPARENT``` Players and monsters can see through/past it. Also sets ter_t.transparent.
- ```UNSTABLE``` Walking here cause the bouldering effect on the character.
- ```USABLE_FIRE``` This terrain or furniture counts as a nearby fire for crafting.
- ```WALL``` This terrain is an upright obstacle. Used for fungal conversion, and also implies `CONNECT_TO_WALL`.

### Examine Actions

- ```aggie_plant``` Harvest plants.
- ```autoclave_empty``` Start the autoclave cycle if it contains filthy CBM, and the player has enough water.
- ```autoclave_full``` Check on the progress of the cycle, and collect sterile CBM once cycle is completed.
- ```bars``` Take advantage of AMORPHOUS and slip through the bars.
- ```bulletin_board``` Use this to arrange tasks for your faction camp.
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
- ```none``` None
- ```pedestal_temple``` Opens the temple if you have a petrified eye.
- ```pedestal_wyrm``` Spawn wyrms.
- ```pit_covered``` Uncover the pit.
- ```pit``` Cover the pit if you have some planks of wood.
- ```portable_structure``` Take down a tent or similar portable structure.
- ```recycle_compactor``` Compress pure metal objects into basic shapes.
- ```rubble``` Clear up the rubble if you have a shovel.
- ```safe``` Attempt to crack the safe.
- ```shelter``` Take down the shelter.
- ```shrub_marloss``` Pick a marloss bush.
- ```shrub_wildveggies``` Pick a wild veggies shrub.
- ```slot_machine``` Gamble.
- ```toilet``` Either drink or get water out of the toilet.
- ```trap``` Interact with a trap.
- ```water_source``` Drink or get water from a water source.

### Fungal Conversions Only

- ```FLOWER``` This furniture is a flower.
- ```FUNGUS``` Fungal covered.
- ```ORGANIC``` This furniture is partly organic.
- ```SHRUB``` This terrain is a shrub.
- ```TREE``` This terrain is a tree.
- ```YOUNG``` This terrain is a young tree.

### Furniture Only

- ```BLOCKSDOOR``` This will boost map terrain's resistance to bashing if `str_*_blocked` is set (see `map_bash_info`)


## Generic

### Flags

- ```ANESTHESIA``` ... Item is considered anesthesia for the purpose of installing or uninstalling bionics.
- ```BIONIC_NPC_USABLE``` ... Safe CBMs that NPCs can use without extensive NPC rewrites to utilize toggle CBMs.
- ```CORPSE``` ... Flag used to spawn various human corpses during the mapgen.
- ```DANGEROUS``` ... NPCs will not accept this item. Explosion iuse actor implies this flag. Implies "NPC_THROW_NOW".
- ```DURABLE_MELEE``` ... Item is made to hit stuff and it does it well, so it's considered to be a lot tougher than other weapons made of the same materials.
- ```FAKE_MILL``` ... Item is a fake item, to denote a partially milled product by @ref Item::process_fake_mill, where conditions for its removal are set.
- ```FAKE_SMOKE``` ... Item is a fake item generating smoke, recognizable by @ref item::process_fake_smoke, where conditions for its removal are set.
- ```FIREWOOD``` ... This item can serve as a firewood. Items with this flag are sorted out to "Loot: Wood" zone
- ```FRAGILE_MELEE``` ... Fragile items that fall apart easily when used as a weapon due to poor construction quality and will break into components when broken.
- ```GAS_DISCOUNT``` ... Discount cards for the automated gas stations.
- ```IS_PET_ARMOR``` ... Is armor for a pet monster, not armor for a person
- ```LEAK_ALWAYS``` ... Leaks (may be combined with "RADIOACTIVE").
- ```LEAK_DAM``` ... Leaks when damaged (may be combined with "RADIOACTIVE").
- ```NEEDS_UNFOLD``` ... Has an additional time penalty upon wielding. For melee weapons and guns this is offset by the relevant skill. Stacks with "SLOW_WIELD".
- ```NO_PACKED``` ... This item is not protected against contamination and won't stay sterile.  Only applies to CBMs.
- ```NO_PICKUP``` ... Character can not pickup anything while wielding this item (e.g. bionic claws).
- ```NO_REPAIR``` ... Prevents repairing of this item even if otherwise suitable tools exist.
- ```NO_SALVAGE``` ... Item cannot be broken down through a salvage process. Best used when something should not be able to be broken down (i.e. base components like leather patches).
- ```NO_STERILE``` ... This item is not sterile.  Only applies to CBMs.
- ```NPC_ACTIVATE``` ... NPCs can activate this item as an alternative attack. Currently by throwing it right after activation. Implied by "BOMB".
- ```NPC_ALT_ATTACK``` ... Shouldn't be set directly. Implied by "NPC_ACTIVATE" and "NPC_THROWN".
- ```NPC_THROWN``` ... NPCs will throw this item (without activating it first) as an alternative attack.
- ```NPC_THROW_NOW``` ... NPCs will try to throw this item away, preferably at enemies. Implies "TRADER_AVOID" and "NPC_THROWN".
- ```PSEUDO``` ... Used internally to mark items that are referred to in the crafting inventory but are not actually items. They can be used as tools, but not as components. Implies "TRADER_AVOID".
- ```RADIOACTIVE``` ... Is radioactive (can be used with LEAK_*).
- ```RAIN_PROTECT``` ... Protects from sunlight and from rain, when wielded.
- ```REDUCED_BASHING``` ... Gunmod flag; reduces the item's bashing damage by 50%.
- ```REDUCED_WEIGHT``` ... Gunmod flag; reduces the item's base weight by 25%.
- ```REQUIRES_TINDER``` ... Requires tinder to be present on the tile this item tries to start a fire on.
- ```SLEEP_AID``` ... This item helps in sleeping.
- ```SLOW_WIELD``` ... Has an additional time penalty upon wielding. For melee weapons and guns this is offset by the relevant skill. Stacks with "NEEDS_UNFOLD".
- ```TIE_UP``` ... Item can be used to tie up a creature.
- ```TINDER``` ... This item can be used as tinder for lighting a fire with a REQUIRES_TINDER flagged firestarter.
- ```TRADER_AVOID``` ... NPCs will not start with this item. Use this for active items (e.g. flashlight (on)), dangerous items (e.g. active bomb), fake item or unusual items (e.g. unique quest item).
- ```UNBREAKABLE_MELEE``` ... Does never get damaged when used as melee weapon.
- ```UNRECOVERABLE``` ... Cannot be recovered from a disassembly.


## Guns

- ```BACKBLAST``` Causes a small explosion behind the person firing the weapon. Currently not implemented?
- ```BIPOD``` Handling bonus only applies on MOUNTABLE map/vehicle tiles. Does not include wield time penalty (see SLOW_WIELD).
- ```CHARGE``` Has to be charged to fire. Higher charges do more damage.
- ```COLLAPSIBLE_STOCK``` Reduces weapon volume proportional to the base size of the gun (excluding any mods). Does not include wield time penalty (see NEEDS_UNFOLD).
- ```CONSUMABLE``` Makes a gunpart have a chance to get damaged depending on ammo fired, and definable fields 'consume_chance' and 'consume_divisor'.
- ```DISABLE_SIGHTS``` Prevents use of the base weapon sights
- ```FIRE_100``` Uses 100 shots per firing.
- ```FIRE_50``` Uses 50 shots per firing.
- ```FIRE_TWOHAND``` Gun can only be fired if player has two free hands.
- ```IRREMOVABLE``` Makes so that the gunmod cannot be removed.
- ```MECH_BAT```    This is an exotic battery designed to power military mechs.
- ```MECH_WEAPON``` A built-in mech weapon, cannot be removed or have mods added / removed.
- ```MOUNTED_GUN``` Gun can only be used on terrain / furniture with the "MOUNTABLE" flag.
- ```NEVER_JAMS``` Never malfunctions.
- ```NO_BOOM``` Cancels the ammo effect "FLAME".
- ```NO_UNLOAD``` Cannot be unloaded.
- ```PRIMITIVE_RANGED_WEAPON``` Allows using non-gunsmith tools to repair it (but not reinforce).
- ```PUMP_ACTION``` Gun has a rails on its pump action, allowing to install only mods with PUMP_RAIL_COMPATIBLE flag on underbarrel slot.
- ```PUMP_RAIL_COMPATIBLE``` Mod can be installed on underbarrel slot of guns with rails on their pump action.
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


## MAP SPECIALS

- ```mx_anomaly``` ...  Natural anomaly (crater + artifact).
- ```mx_bandits_block``` ...  Road block made by bandits from tree logs.
- ```mx_burned_ground``` ... Fire has ravaged this place.
- ```mx_point_burned_ground``` ... Fire has ravaged this place. (partial application)
- ```mx_clay_deposit``` ... A small surface clay deposit.
- ```mx_clearcut``` ... All trees become stumps.
- ```mx_collegekids``` ... Corpses and items.
- ```mx_crater``` ... Crater with rubble (and radioactivity).
- ```mx_drugdeal``` ... Corpses and some drugs.
- ```mx_dead_vegetation``` ... Kills all plants. (aftermath of acid rain etc.)
- ```mx_point_dead_vegetation``` ... Kills all plants. (aftermath of acid rain etc.) (partial application)
- ```mx_fumarole``` ... A lava rift.
- ```mx_grove``` ... All trees and shrubs become a single species of tree.
- ```mx_helicopter``` ... Metal wreckage and some items.
- ```mx_jabberwock``` ... A *chance* of a jabberwock.
- ```mx_military``` ... Corpses and some military items.
- ```mx_minefield``` ... Landmines, a field of them.
- ```mx_null``` ... No special at all.
- ```mx_pond``` ... A small pond.
- ```mx_portal_in``` ... Another portal to neither space.
- ```mx_portal``` ... Portal to neither space.
- ```mx_roadblock``` ... Roadblock furniture with turrets and some cars.
- ```mx_roadworks``` ... Partialy closed damaged road with chance of work equipment and utility vehicles.
- ```mx_science``` ... Corpses and some scientist items.
- ```mx_shia``` ... A *chance* of Shia, if Crazy Catalcysm is enabled.
- ```mx_shrubbery``` ... All trees and shrubs become a single species of shrub.
- ```mx_spider``` ... A big spider web, complete with spiders and eggs.
- ```mx_supplydrop``` ... Crates with some military items in it.


## Material Phases

- ```GAS```
- ```LIQUID```
- ```NULL```
- ```PLASMA```
- ```SOLID```


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


## Monsters

Flags used to describe monsters and define their properties and abilities.

### Anger, Fear and Placation Triggers

- ```FIRE``` There's a fire nearby.
- ```FRIEND_ATTACKED``` A monster of the same type was attacked.
- ```FRIEND_DIED``` A monster of the same type died.
- ```HURT``` The monster is hurt.
- ```MEAT``` Meat or a corpse is nearby.
- ```NULL``` Source use only?
- ```PLAYER_CLOSE``` The player gets within a few tiles distance.
- ```PLAYER_WEAK``` The player is hurt.
- ```SOUND``` Heard a sound.
- ```STALK``` Increases when following the player.

### Categories

- ```CLASSIC``` Only monsters we expect in a classic zombie movie.
- ```NULL``` No category.
- ```WILDLIFE``` Natural animals.

### Death Functions

Multiple death functions can be used. Not all combinations make sense.

- ```ACID``` Acid instead of a body. not the same as the ACID_BLOOD flag. In most cases you want both.
- ```AMIGARA``` Removes hypnosis if the last one.
- ```BLOBSPLIT``` Creates more blobs.
- ```BOOMER``` Explodes in vomit.
- ```BROKEN``` Spawns a broken robot item, its id calculated like this: the prefix "mon_" is removed from the monster id, than the prefix "broken_" is added. Example: mon_eyebot -> broken_eyebot
- ```DISAPPEAR``` Hallucination disappears.
- ```DISINTEGRATE``` Falls apart.
- ```EXPLODE``` Damaging explosion.
- ```FIREBALL``` 10 percent chance to explode in a fireball.
- ```FLAME_EXPLOSION``` guaranteed to explode and starts fires.
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

- ```ABSORBS_SPLITS``` Consumes objects it moves over, and if it absorbs enough it will split into a copy.
- ```ABSORBS``` Consumes objects it moves over. (Modders use this).
- ```ACIDPROOF``` Immune to acid.
- ```ACIDTRAIL``` Leaves a trail of acid.
- ```ACID_BLOOD``` Makes monster bleed acid. Fun stuff! Does not automatically dissolve in a pool of acid on death.
- ```ANIMAL``` Is an _animal_ for purposes of the `Animal Empathy` trait.
- ```AQUATIC``` Confined to water.
- ```ARTHROPOD_BLOOD``` Forces monster to bleed hemolymph.
- ```ATTACKMON``` Attacks other monsters.
- ```BADVENOM``` Attack may **severely** poison the player.
- ```BASHES``` Bashes down doors.
- ```BILE_BLOOD``` Makes monster bleed bile.
- ```BIRDFOOD``` Becomes friendly / tamed with bird food.
- ```BLEED``` Causes the player to bleed.
- ```BONES``` May produce bones and sinews when butchered.
- ```BORES``` Tunnels through just about anything (15x bash multiplier: dark wyrms' bash skill 12->180)
- ```CAN_DIG``` Can dig _and_ walk.
- ```CAN_OPEN_DOORS``` Can open doors on its path.
- ```CANPLAY``` This creature can be played with if it's a pet.
- ```CATFOOD``` Becomes friendly / tamed with cat food.
- ```CATTLEFODDER``` Becomes friendly / tamed with cattle fodder.
- ```CBM_CIV``` May produce a common CBM a power CBM when butchered.
- ```CBM_OP``` May produce a CBM or two from 'bionics_op' item group when butchered.
- ```CBM_POWER``` May produce a power CBM when butchered, independent of CBM.
- ```CBM_SCI``` May produce a CBM from 'bionics_sci' item group when butchered.
- ```CBM_SUBS``` May produce a CBM or two from bionics_subs and a power CBM when butchered.
- ```CBM_TECH``` May produce a CBM or two from 'bionics_tech' item group and a power CBM when butchered.
- ```CHITIN``` May produce chitin when butchered.
- ```CLIMBS``` Can climb.
- ```CURRENT``` this water is flowing.
- ```DESTROYS``` Bashes down walls and more. (2.5x bash multiplier, where base is the critter's max melee bashing)
- ```DIGS``` Digs through the ground.
- ```DOGFOOD``` Becomes friendly / tamed with dog food.
- ```DRIPS_GASOLINE``` Occasionally drips gasoline on move.
- ```DRIPS_NAPALM``` Occasionally drips napalm on move.
- ```ELECTRIC``` Shocks unarmed attackers.
- ```ELECTRONIC``` e.g. A Robot; affected by emp blasts and other stuff.
- ```FAT``` May produce fat when butchered.
- ```FEATHER``` May produce feathers when butchered.
- ```FILTHY``` Any clothing it drops will be filthy.
- ```FIREPROOF``` Immune to fire.
- ```FIREY``` Burns stuff and is immune to fire.
- ```FISHABLE``` It is fishable.
- ```FLAMMABLE``` Monster catches fire, burns, and spreads fire to nearby objects.
- ```FLIES``` Can fly (over water, etc.)
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
- ```LOUDMOVES``` Mkes move noises as if ~2 sizes louder, even if flying.
- ```MECH_RECON_VISION``` This mech grants you night-vision and enhanced overmap sight radius when piloted.
- ```MECH_DEFENSIVE``` This mech can protect you thoroughly when piloted.
- ```MILITARY_MECH``` Is a military-grade mech.
- ```MILKABLE``` Produces milk when milked.
- ```NIGHT_INVISIBILITY``` Monster becomes invisible if it's more than one tile away and the lighting on its tile is LL_LOW or less. Visibility is not affected by night vision.
- ```NOGIB``` Does not leave gibs / meat chunks when killed with huge damage.
- ```NOHEAD``` Headshots not allowed!
- ```NO_BREATHE``` Creature can't drown and is unharmed by gas, smoke or poison.
- ```NO_BREED``` Creature doesn't reproduce even though it has reproduction data - useful when using copy-from to make child versions of adult creatures
- ```NULL``` Source use only.
- ```PACIFIST``` That monster will never do melee attacks.
- ```PARALYZE``` Attack may paralyze the player with venom.
- ```PLASTIC``` Absorbs physical damage to a great degree.
- ```POISON``` Poisonous to eat.
- ```PUSH_MON``` Can push creatures out of its way.
- ```QUEEN``` When it dies, local populations start to die off too.
- ```REGENERATES_1``` Monster regenerates slowly over time.
- ```REGENERATES_10``` Monster regenerates quickly over time.
- ```REGENERATES_50``` Monster regenerates very quickly over time.
- ```REGENERATES_IN_DARK``` Monster regenerates very quickly in poorly lit tiles.
- ```REGEN_MORALE``` Will stop fleeing if at max hp, and regen anger and morale.
- ```REVIVES``` Monster corpse will revive after a short period of time.
- ```RIDEABLE_MECH``` This monster is a mech suit that can be piloted.
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

### Monster Defense and Attacks

- ```ACIDSPLASH``` Splash acid on the attacker
- ```NONE``` No special attack-back
- ```ZAPBACK``` Shock attacker on hit

### Sizes

- ```HUGE``` Tank
- ```LARGE``` Cow
- ```MEDIUM``` Human
- ```SMALL``` Dog
- ```TINY``` Squirrel

### Special attacks

Some special attacks are also valid use actions for tools and weapons.
See `monsters.json` for examples on how to use these attacks.
Also see `monster_attacks.json` for more special attacks, for example, impale and scratch.

- ```ACID_ACCURATE``` Shoots acid that is accurate at long ranges, but less so up close.
- ```ACID_BARF``` Barfs corroding, blinding acid.
- ```ACID``` Spit acid.
- ```ANTQUEEN``` Hatches/grows: `egg > ant > soldier`.
- ```BIO_OP_TAKEDOWN``` Attack with special martial art takedown maneuverer.
- ```BITE``` Bite attack that can cause deep infected wounds.
- ```BMG_TUR``` Barrett .50BMG rifle fires.
- ```BOOMER_GLOW``` Spit glowing bile.
- ```BOOMER``` Spit bile.
- ```BRANDISH``` Brandish a knife at the player.
- ```BREATHE``` Spawns a `breather`
- ```CALLBLOBS``` Calls 2/3 of nearby blobs to defend this monster, and sends 1/3 of nearby blobs after the player.
- ```CHICKENBOT``` Robot can attack with tazer, M4, or MGL depending on distance.
- ```COPBOT``` Cop-bot alerts and then tazes the player.
- ```DANCE``` Monster dances.
- ```DARKMAN``` Can cause darkness and wraiths to spawn.
- ```DERMATIK_GROWTH``` Dermatik larva grows into an adult.
- ```DERMATIK``` Attempts to lay dermatik eggs in the player.
- ```DISAPPEAR``` Hallucination disappears.
- ```DOGTHING``` The dog _thing_ spawns into a tentacle dog.
- ```FEAR_PARALYZE``` Paralyze the player with fear.
- ```FLAMETHROWER``` Shoots a stream of fire.
- ```FLESH_GOLEM``` Attack the player with claw, and inflict disease `downed` if the attack connects.
- ```FORMBLOB``` Spawns blobs?
- ```FRAG_TUR``` MGL fires frag rounds.
- ```FUNGUS_BIG_BLOSSOM``` Spreads fire suppressing fungal haze.
- ```FUNGUS_BRISTLE``` Perform barbed tendril attack that can cause fungal infections.
- ```FUNGUS_CORPORATE``` Used solely by Crazy Cataclysm. This will cause runtime errors if used without out, and spawns SpOreos on top of the creature.
- ```FUNGUS_FORTIFY``` Grows Fungal hedgerows, and advances player on the mycus threshold path.
- ```FUNGUS_GROWTH``` Grows a young fungaloid into an adult.
- ```FUNGUS_HAZE``` Spawns fungal fields.
- ```FUNGUS_INJECT``` Perform needle attack that can cause fungal infections.
- ```FUNGUS_SPROUT``` Grows a fungal wall.
- ```FUNGUS``` Releases fungal spores and attempts to infect the player.
- ```GENERATOR``` Regenerates health.
- ```GENE_STING``` Shoot a dart at the player that causes a mutation if it connects.
- ```GRAB``` GRAB the target, and drag it around.
- ```GRAB``` Grabs the player, slowing on hit, making movement and dodging impossible and blocking harder.
- ```GROWPLANTS``` Spawns underbrush, or promotes it to `> young tree > tree`.
- ```GROW_VINE``` Grows creeper vines.
- ```HOWL``` "an ear-piercing howl!"
- ```JACKSON``` Converts zombies into zombie dancers.
- ```LASER``` Laser turret fires.
- ```LEAP``` leap away to an unobstructed tile.
- ```LONGSWIPE``` Does high damage claw attack, which can even hit some distance away.
- ```LUNGE``` Perform a jumping attack from some distance away, which can down the target.
- ```MULTI_ROBOT``` Robot can attack with tazer, flamethrower, M4, MGL, or 120mm cannon depending on distance.
- ```NONE``` No special attack.
- ```PARA_STING``` Shoot a paralyzing dart at the player.
- ```PARROT``` Parrots the speech defined in `speech.json`, picks one of the lines randomly. "speaker" points to a monster id.
- ```PARROT_AT_DANGER``` Performs the same function as PARROT, but only if the creature sees an angry monster from a hostile faction.
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
- ```SHOCKING_REVEAL``` Shoots bolts of lightning, and reveals a SHOCKING FACT! Very fourth-wall breaking. Used solely by Crazy Cataclysm.
- ```SHOCKSTORM``` Shoots bolts of lightning.
- ```SHRIEK_ALERT``` "a very terrible shriek!"
- ```SHRIEK_STUN``` "a stunning shriek!", causes a small bash, can cause a stun.
- ```SHRIEK``` "a terrible shriek!"
- ```SLIMESPRING``` Can provide a morale boost to the player, and cure bite and bleed effects.
- ```SMASH``` Smashes the target for massive damage, sending it flying for a number of tiles equal to `("melee_dice" * "melee_dice_sides" * 3) / 10`.
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


## Overmap

### Overmap connections

- ```ORTHOGONAL``` The connection generally prefers straight lines, avoids turning wherever possible.

### Overmap specials

#### Flags

- ```BEE``` Location is related to bees. Used to classify location.
- ```BLOB``` Location should "blob" outward from the defined location with a chance to be placed in adjacent locations.
- ```CLASSIC``` Location is allowed when classic zombies are enabled.
- ```FUNGAL``` Location is related to fungi. Used to classify location.
- ```TRIFFID``` Location is related to triffids. Used to classify location.
- ```LAKE``` Location is is placed on a lake and will be ignored for placement if the overmap doesn't contain any lake terrain.
- ```UNIQUE``` Location is unique and will only occur once per overmap. `occurrences` is overridden to define a percent chance (e.g. `"occurrences" : [75, 100]` is 75%)

### Overmap terrains

#### Flags

- ```KNOWN_DOWN``` There's a known way down.
- ```KNOWN_UP``` There's a known way up.
- ```LINEAR``` For roads etc, which use ID_straight, ID_curved, ID_tee, ID_four_way.
- ```NO_ROTATE``` The terrain can't be rotated (ID_north, ID_east, ID_south, and ID_west instances will NOT be generated, just ID).
- ```RIVER``` It's a river tile.
- ```SIDEWALK``` Has sidewalks on the sides adjacent to roads.
- ```LAKE``` Consider this location to be a valid lake terrain for mapgen purposes.
- ```LAKE_SHORE``` Consider this location to be a valid lake shore terrain for mapgen purposes.


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

- ```ALLOW_ROTTEN``` Explicitly allow rotten components when crafting non-perishables.
- ```BLIND_EASY``` Easy to craft with little to no light.
- ```BLIND_HARD``` Possible to craft with little to no light, but difficult.
- ```SECRET``` Not automatically learned at character creation time based on high skill levels.
- ```UNCRAFT_LIQUIDS_CONTAINED``` Spawn liquid items in its default container.
- ```UNCRAFT_SINGLE_CHARGE``` Lists returned amounts for one charge of an item that is counted by charges.
- ```FULL_MAGAZINE``` If this recipe requires magazines, it needs one that is full.  For deconstruction recipes, it will spawn a full magazine when deconstructed.


## Scenarios

### Flags

- ```ALLOW_OUTSIDE``` Allows placing player outside of building, useful for outdoor start.
- ```BAD_DAY``` Player starts the game drunk, depressed and sick with the flu.
- ```BOARDED``` Start in boarded building (windows and doors are boarded, movable furniture is moved to windows and doors).
- ```CHALLENGE``` Game won't choose this scenario in random game types.
- ```CITY_START``` Scenario is available only when city size value in world options is more than 0.
- ```FIRE_START``` Player starts the game with fire nearby.
- ```HELI_CRASH``` Player starts the game with various limbs wounds.
- ```INFECTED``` Player starts the game infected.
- ```LONE_START``` If starting NPC spawn option is switched to "Scenario-based", this scenario won't spawn a fellow NPC on game start.
- ```SCEN_ONLY``` Profession can be chosen only as part of the appropriate scenario.
- ```SUR_START``` Surrounded start, zombies outside the starting location.

#### Season Flags

- ```AUT_START``` ... start in autumn.
- ```SPR_START``` ... start in spring.
- ```SUM_ADV_START``` ... start second summer after Cataclysm.
- ```SUM_START``` ... start in summer.
- ```WIN_START``` ... start in winter.


## Skills

### Tags

- ```combat_skill``` The skill is considered a combat skill. It's affected by "PACIFIST", "PRED1", "PRED2", "PRED3", and "PRED4" traits.
- ```contextual_skill``` The skill is abstract, it depends on context (an indirect item to which it's applied). Neither player nor NPCs can possess it.


## Techniques

Techniques may be used by tools, armors, weapons and anything else that can be wielded.

- See contents of `data/json/techniques.json`.
- Techniques are also used with martial arts styles, see `data/json/martialarts.json`.


## Tools

### Flags

Melee flags are fully compatible with tool flags, and vice versa.

- ```ACT_ON_RANGED_HIT```  The item should activate when thrown or fired, then immediately get processed if it spawns on the ground.
- ```ALLOWS_REMOTE_USE``` This item can be activated or reloaded from adjacent tile without picking it up.
- ```BELT_CLIP``` The item can be clipped or hooked on to a belt loop of the appropriate size (belt loops are limited by their max_volume and max_weight properties)
- ```BOMB``` It can be a remote controlled bomb.
- ```CABLE_SPOOL``` This item is a cable spool and must be processed as such. It has an internal "state" variable which may be in the states "attach_first" or "pay_out_cable" -- in the latter case, set its charges to `max_charges - dist(here, point(vars["source_x"], vars["source_y"]))`. If this results in 0 or a negative number, set its state back to "attach_first".
- ```CHARGEDIM``` If illuminated, light intensity fades with charge, starting at 20% charge left.
- ```DIG_TOOL``` If wielded, digs thorough terrain like rock and walls, as player walks into them. If item also has ```POWERED``` flag, then it digs faster, but uses up the item's ammo as if activating it.
- ```FIRESTARTER``` Item will start fire with some difficulty.
- ```FIRE``` Item will start a fire immediately.
- ```FISH_GOOD``` When used for fishing, it's a good tool (requires that the matching use_action has been set).
- ```FISH_POOR``` When used for fishing, it's a poor tool (requires that the matching use_action has been set).
- ```HAS_RECIPE``` Used by the E-Ink tablet to indicates it's currently showing a recipe.
- ```IS_UPS``` Item is Unified Power Supply. Used in active item processing
- ```LIGHT_[X]``` Illuminates the area with light intensity `[X]` where `[X]` is an intensity value. (e.x. `LIGHT_4` or `LIGHT_100`).
- ```MC_MOBILE```, ```MC_RANDOM_STUFF```, ```MC_SCIENCE_STUFF```, ```MC_USED```, ```MC_HAS_DATA``` Memory card related flags, see `iuse.cpp`
- ```NO_DROP``` Item should never exist on map tile as a discrete item (must be contained by another item)
- ```NO_UNLOAD``` Cannot be unloaded.
- ```POWERED``` If turned ON, item uses its own source of power, instead of relying on power of the user
- ```RADIOCARITEM``` Item can be put into a remote controlled car.
- ```RADIOSIGNAL_1``` Activated per radios signal 1.
- ```RADIOSIGNAL_2``` Activated per radios signal 2.
- ```RADIOSIGNAL_3``` Activated per radios signal 3.
- ```RADIO_ACTIVATION``` It is activated by a remote control (also requires RADIOSIGNAL*).
- ```RADIO_CONTAINER``` It's a container of something that is radio controlled.
- ```RADIO_MODABLE``` Indicates the item can be made into a radio-activated item.
- ```RADIO_MOD``` The item has been made into a radio-activated item.
- ```RECHARGE``` Gain charges when placed in a cargo area with a recharge station.
- ```SAFECRACK``` This item can be used to unlock safes.
- ```USES_BIONIC_POWER``` The item has no charges of its own, and runs off of the player's bionic power.
- ```USE_UPS``` Item is charges from an UPS / it uses the charges of an UPS instead of its own.
- ```WATER_EXTINGUISH``` Is extinguishable in water or under precipitation. Converts items (requires "reverts_to" or use_action "transform" to be set).
- ```WET``` Item is wet and will slowly dry off (e.g. towel).
- ```WIND_EXTINGUISH``` This item will be extinguished by the wind.
- ```WRITE_MESSAGE``` This item could be used to write messages on signs.

### Flags that apply to items

These flags **do not apply to item types**.

Those flags are added by the game code to specific items (that specific welder, not *all* welders).

- ```COLD``` Item is cold (see EATEN_COLD).
- ```DIRTY``` Item (liquid) was dropped on the ground and is now irreparably dirty.
- ```FIELD_DRESS_FAILED``` Corpse was damaged by unskillful field dressing. Affects butcher results.
- ```FIELD_DRESS``` Corpse was field dressed. Affects butcher results.
- ```FIT``` Reduces encumbrance by one.
- ```FROZEN``` Item is frozen solid (used by freezer).
- ```HOT``` Item is hot (see EATEN_HOT).
- ```LITCIG``` Marks a lit smoking item (cigarette, joint etc.).
- ```MUSHY``` FREEZERBURN item was frozen and is now mushy and tasteless and will go bad after freezing again.
- ```NO_PARASITES``` Invalidates parasites count set in food->type->comestible->parasites
- ```QUARTERED``` Corpse was quartered into parts. Affects butcher results, weight, volume.
- ```REVIVE_SPECIAL``` ... Corpses revives when the player is nearby.
- ```USE_UPS``` The tool has the UPS mod and is charged from an UPS.
- ```WARM``` A hidden flag used to track an item's journey to/from hot, buffers between HOT and cold.
- ```WET``` Item is wet and will slowly dry off (e.g. towel).


## Vehicle Parts

### Flags

- ```ADVANCED_PLANTER``` This planter doesn't spill seeds and avoids damaging itself on non-diggable surfaces.
- ```AISLE_LIGHT```
- ```AISLE``` Player can move over this part with less speed penalty than normal.
- ```ALTERNATOR``` Recharges batteries installed on the vehicle.
- ```ANCHOR_POINT``` Allows secure seatbelt attachment.
- ```ARMOR``` Protects the other vehicle parts it's installed over during collisions.
- ```ATOMIC_LIGHT```
- ```BATTERY_MOUNT```
- ```BED``` A bed where the player can sleep.
- ```BEEPER``` Generates noise when the vehicle moves backward.
- ```BELTABLE``` Seatbelt can be attached to this part.
- ```BIKE_RACK_VEH``` Can be used to merge an adjacent single tile wide vehicle, or split a single tile wide vehicle off into its own vehicle.
- ```BOARDABLE``` The player can safely move over or stand on this part while the vehicle is moving.
- ```CAMERA_CONTROL```
- ```CAMERA```
- ```CAPTURE_MOSNTER_VEH``` Can be used to capture monsters when mounted on a vehicle.
- ```CARGO_LOCKING``` This cargo area is inaccessible to NPCs.
- ```CARGO``` Cargo holding area.
- ```CHEMLAB``` Acts as a chemistry set for crafting.
- ```CHIMES``` Generates continuous noise when used.
- ```CIRCLE_LIGHT``` Projects a circular radius of light when turned on.
- ```CONE_LIGHT``` Projects a cone of light when turned on.
- ```CONTROLS``` Can be used to control the vehicle.
- ```COOLER``` There is separate command to toggle this part.
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
- ```E_ALTERNATOR``` Is an engine that can power an alternator.
- ```E_COLD_START``` Is an engine that starts much slower in cold weather.
- ```E_COMBUSTION``` Is an engine that burns its fuel and can backfire or explode when damaged.
- ```E_HEATER``` Is an engine and has a heater to warm internal vehicle items when on.
- ```E_HIGHER_SKILL``` Is an engine that is more difficult to install as more engines are installed.
- ```E_STARTS_INSTANTLY``` Is an engine that starts instantly, like food pedals.
- ```FAUCET```
- ```EMITTER``` Is a part has emission (defined in ```emissions```).
- ```FLAT_SURF``` Part with a flat hard surface (e.g. table).
- ```FOLDABLE```
- ```FORGE``` Acts as a forge for crafting.
- ```FREEZER``` Can freeze items in below zero degrees Celsius temperature.
- ```FRIDGE``` Can refrigerate items.
- ```FUNNEL```
- ```HALF_CIRCLE_LIGHT``` Projects a directed half-circular radius of light when turned on.
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
- ```ON_ROOF``` - Parts with this flag could only be installed on a roof (parts with ```ROOF``` flag).
- ```OPAQUE``` Cannot be seen through.
- ```OPENABLE``` Can be opened or closed.
- ```OPENCLOSE_INSIDE```  Can be opened or closed, but only from inside the vehicle.
- ```OVER``` Can be mounted over other parts.
- ```PERPETUAL``` If paired with REACTOR, part produces electrical power without consuming fuel.
- ```PLANTER``` Plants seeds into tilled dirt, spilling them when the terrain underneath is unsuitable. It is damaged by running it over non-```DIGGABLE``` surfaces.
- ```PLOW``` Tills the soil underneath the part while active. Takes damage from unsuitable terrain at a level proportional to the speed of the vehicle.
- ```POWER_TRANSFER``` Transmits power to and from an attached thingy (probably a vehicle).
- ```PROTRUSION``` Part sticks out so no other parts can be installed over it.
- ```REACTOR``` When enabled, part consumes fuel to generate epower.
- ```REAPER``` Cuts down mature crops, depositing them on the square.
- ```RECHARGE``` Recharge items with the same flag. ( Currently only the rechargeable battery mod. )
- ```REMOTE_CONTROLS```
- ```REVERSIBLE``` Removal has identical requirements to installation but is twice as quick
- ```ROOF``` Covers a section of the vehicle. Areas of the vehicle that have a roof and roofs on surrounding sections, are considered inside. Otherwise they're outside.
- ```SCOOP``` Pulls items from underneath the vehicle to the cargo space of the part. Also mops up liquids.
- ```SEATBELT``` Helps prevent the player from being ejected from the vehicle during an accident.
- ```SEAT``` A seat where the player can sit or sleep.
- ```SECURITY```
- ```SHARP``` Striking a monster with this part does cutting damage instead of bashing damage, and prevents stunning the monster.
- ```SOLAR_PANEL``` Recharges vehicle batteries when exposed to sunlight. Has a 1 in 4 chance of being broken on car generation.
- ```SPACE_HEATER``` There is separate command to toggle this part.
- ```STABLE``` Similar to `WHEEL`, but if the vehicle is only a 1x1 section, this single wheel counts as enough wheels.
- ```STEERABLE``` This wheel is steerable.
- ```STEREO```
- ```TRANSFORM_TERRAIN``` Transform terrain (using rules defined in ```transform_terrain```).
- ```TOOL_NONE``` Can be removed/installed without any tools
- ```TOOL_SCREWDRIVER``` Attached with screws, can be removed/installed with a screwdriver
- ```TOOL_WRENCH``` Attached with bolts, can be removed/installed with a wrench
- ```TOWEL``` Can be used to dry yourself up.
- ```TRACKED``` Contributes to steering effectiveness but doesn't count as a steering axle for install difficulty and still contributes to drag for the center of steering calculation.
- ```TRACK``` Allows the vehicle installed on, to be marked and tracked on map.
- ```TURRET_MOUNT``` Parts with this flag are suitable for installing turrets.
- ```TURRET``` Is a weapon turret. Can only be installed on a part with ```TURRET_MOUNT``` flag.
- ```UNMOUNT_ON_DAMAGE``` Part breaks off the vehicle when destroyed by damage.
- ```UNMOUNT_ON_MOVE``` Dismount this part when the vehicle moves. Doesn't drop the part, unless you give it special handling.
- ```VARIABLE_SIZE``` Has 'bigness' for power, wheel radius, etc.
- ```VISION```
- ```WASHING_MACHINE``` Can be used to wash filthy clothes en masse.
- ```DISHWASHER``` Can be used to wash filthy non-soft items en masse.
- ```WATER_WHEEL``` Recharges vehicle batteries when in flowing water.
- ```WATER_WHEEL``` Recharges vehicle batteries when submerged in moving water.
- ```WELDRIG``` Acts as a welder for crafting.
- ```WHEEL``` Counts as a wheel in wheel calculations.
- ```WIDE_CONE_LIGHT``` Projects a wide cone of light when turned on.
- ```WINDOW``` Can see through this part and can install curtains over it.
- ```WIND_POWERED``` This engine is powered by wind ( sails etc ).
- ```WIND_TURBINE``` Recharges vehicle batteries when exposed to wind.
- ```WORKBENCH``` Can craft at this part, must be paired with a workbench json entry.

### Fuel types

- ```NULL``` None
- ```battery``` Electrifying.
- ```diesel``` Refined dino.
- ```gasoline``` Refined dino.
- ```plasma``` Superheated.
- ```plutonium``` 1.21 Gigawatts!
- ```water``` Clean.
- ```wind``` Wind powered.
